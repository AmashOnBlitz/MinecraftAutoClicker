#include "pch.h"
#include "mclib.h"
#include <Windows.h>
#include "jni/jni.h"
#include <string>
#include <iostream>

// ─── Error string ─────────────────────────────────────────────────────────────
static char g_err[256] = "";
static void set_err(const char* msg) { strncpy_s(g_err, sizeof(g_err), msg, _TRUNCATE); }
const char* MC_GetError() { return g_err; }

// ─── JVM ─────────────────────────────────────────────────────────────────────
static JavaVM* g_vm = nullptr;
static JNIEnv* g_env = nullptr;

static bool jvm_load()
{
    jsize count = 0;
    if (JNI_GetCreatedJavaVMs(&g_vm, 1, &count) != JNI_OK) {
        set_err("JNI_GetCreatedJavaVMs failed — is java.dll loaded?"); return false;
    }
    if (count == 0 || !g_vm) {
        set_err("No running JVM found — inject after Minecraft has started"); return false;
    }
    jint r = g_vm->GetEnv(reinterpret_cast<void**>(&g_env), JNI_VERSION_1_8);
    if (r == JNI_EDETACHED) {
        JavaVMAttachArgs args{}; args.version = JNI_VERSION_1_8;
        args.name = const_cast<char*>("mclib");
        r = g_vm->AttachCurrentThreadAsDaemon(reinterpret_cast<void**>(&g_env), &args);
        if (r != JNI_OK) { set_err("AttachCurrentThreadAsDaemon failed"); return false; }
    }
    if (r != JNI_OK || !g_env) { set_err("GetEnv failed"); return false; }
    return true;
}
static void jvm_unload() { if (g_vm) g_vm->DetachCurrentThread(); g_vm = nullptr; g_env = nullptr; }
static bool exc_clear(const char* = "") { if (!g_env->ExceptionCheck()) return false; g_env->ExceptionClear(); return true; }

// ─── Reflection helpers ───────────────────────────────────────────────────────
static jobject   g_loader = nullptr;
static jclass    g_classCls = nullptr;
static jmethodID g_forName3 = nullptr;
static jclass    g_fieldCls = nullptr;
static jmethodID g_fieldGet = nullptr;
static jmethodID g_fieldSet = nullptr;
static jmethodID g_fieldSetFl = nullptr;
static jmethodID g_fieldSetBo = nullptr;
static jmethodID g_fieldSetInt = nullptr;  // setInt primitive
static jmethodID g_setAccess = nullptr;

static jobject r_class(const char* name) {
    jstring s = g_env->NewStringUTF(name);
    jobject c = g_env->CallStaticObjectMethod(g_classCls, g_forName3, s, JNI_TRUE, g_loader);
    g_env->DeleteLocalRef(s); if (exc_clear(name)) return nullptr; return c;
}

static bool reflect_init()
{
    // ── Classloader ───────────────────────────────────────────────────────────
    jclass threadCls = g_env->FindClass("java/lang/Thread");
    if (!threadCls) { set_err("FindClass(Thread) failed"); return false; }
    jmethodID getAllStack = g_env->GetStaticMethodID(threadCls, "getAllStackTraces", "()Ljava/util/Map;");
    jmethodID getLoader = g_env->GetMethodID(threadCls, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    jmethodID setLoader = g_env->GetMethodID(threadCls, "setContextClassLoader", "(Ljava/lang/ClassLoader;)V");
    jmethodID curThread = g_env->GetStaticMethodID(threadCls, "currentThread", "()Ljava/lang/Thread;");
    jobject map = g_env->CallStaticObjectMethod(threadCls, getAllStack);
    jclass mapCls = g_env->FindClass("java/util/Map");
    jclass setCls = g_env->FindClass("java/util/Set");
    jclass iterCls = g_env->FindClass("java/util/Iterator");
    jobject keySet = g_env->CallObjectMethod(map, g_env->GetMethodID(mapCls, "keySet", "()Ljava/util/Set;"));
    jobject iter = g_env->CallObjectMethod(keySet, g_env->GetMethodID(setCls, "iterator", "()Ljava/util/Iterator;"));
    jmethodID hasNext = g_env->GetMethodID(iterCls, "hasNext", "()Z");
    jmethodID next = g_env->GetMethodID(iterCls, "next", "()Ljava/lang/Object;");
    while (g_env->CallBooleanMethod(iter, hasNext)) {
        jobject thread = g_env->CallObjectMethod(iter, next);
        jobject loader = g_env->CallObjectMethod(thread, getLoader); exc_clear();
        if (loader) { g_loader = g_env->NewGlobalRef(loader); g_env->DeleteLocalRef(loader); g_env->DeleteLocalRef(thread); break; }
        g_env->DeleteLocalRef(thread);
    }
    g_env->DeleteLocalRef(iter); g_env->DeleteLocalRef(keySet); g_env->DeleteLocalRef(map);
    if (!g_loader) { set_err("No classloader found"); return false; }
    jobject self = g_env->CallStaticObjectMethod(threadCls, curThread);
    g_env->CallVoidMethod(self, setLoader, g_loader);
    g_env->DeleteLocalRef(self); g_env->DeleteLocalRef(threadCls);

    // ── Bootstrap reflection handles ──────────────────────────────────────────
    auto global = [](jclass c)->jclass { jclass g = (jclass)g_env->NewGlobalRef(c); g_env->DeleteLocalRef(c); return g; };
    g_classCls = global(g_env->FindClass("java/lang/Class"));
    if (!g_classCls) { set_err("FindClass(Class) failed"); return false; }
    g_forName3 = g_env->GetStaticMethodID(g_classCls, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
    if (!g_forName3) { set_err("GetMethodID(Class.forName) failed"); return false; }
    g_fieldCls = global(g_env->FindClass("java/lang/reflect/Field"));
    if (!g_fieldCls) { set_err("FindClass(Field) failed"); return false; }
    g_fieldGet = g_env->GetMethodID(g_fieldCls, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
    g_fieldSet = g_env->GetMethodID(g_fieldCls, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    g_fieldSetFl = g_env->GetMethodID(g_fieldCls, "setFloat", "(Ljava/lang/Object;F)V");
    g_fieldSetBo = g_env->GetMethodID(g_fieldCls, "setBoolean", "(Ljava/lang/Object;Z)V");
    g_fieldSetInt = g_env->GetMethodID(g_fieldCls, "setInt", "(Ljava/lang/Object;I)V");
    if (!g_fieldGet || !g_fieldSet || !g_fieldSetFl || !g_fieldSetBo || !g_fieldSetInt) {
        set_err("GetMethodID on Field accessor methods failed"); return false;
    }
    jclass accCls = g_env->FindClass("java/lang/reflect/AccessibleObject");
    if (!accCls) { set_err("FindClass(AccessibleObject) failed"); return false; }
    g_setAccess = g_env->GetMethodID(accCls, "setAccessible", "(Z)V");
    g_env->DeleteLocalRef(accCls);
    if (!g_setAccess) { set_err("GetMethodID(setAccessible) failed"); return false; }

    // ── Verify MC class reachable ─────────────────────────────────────────────
    jobject testCls = r_class("gfj");
    if (!testCls) { set_err("Class 'gfj' (Minecraft) not found — wrong MC version"); return false; }
    g_env->DeleteLocalRef(testCls);
    return true;
}

// ─── Field helpers ────────────────────────────────────────────────────────────
static jobject r_field(jobject cls, const char* name) {
    jclass cc = g_env->FindClass("java/lang/Class");
    jmethodID m = g_env->GetMethodID(cc, "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
    g_env->DeleteLocalRef(cc);
    jstring s = g_env->NewStringUTF(name);
    jobject fld = g_env->CallObjectMethod(cls, m, s);
    g_env->DeleteLocalRef(s);
    if (exc_clear(name)) return nullptr;
    g_env->CallVoidMethod(fld, g_setAccess, JNI_TRUE); exc_clear();
    return fld;
}
static jobject r_get(jobject fld, jobject obj) { jobject r = g_env->CallObjectMethod(fld, g_fieldGet, obj); exc_clear(); return r; }
static void    r_set_bool(jobject fld, jobject obj, bool v) { g_env->CallVoidMethod(fld, g_fieldSetBo, obj, v ? JNI_TRUE : JNI_FALSE); exc_clear(); }
static void    r_set_float(jobject fld, jobject obj, float v) { g_env->CallVoidMethod(fld, g_fieldSetFl, obj, (jfloat)v); exc_clear(); }
static void    r_set_int(jobject fld, jobject obj, int v) { g_env->CallVoidMethod(fld, g_fieldSetInt, obj, (jint)v); exc_clear(); }
static void    r_set_boxed(jobject fld, jobject obj, int v) {
    jclass ic = g_env->FindClass("java/lang/Integer");
    jmethodID vo = g_env->GetStaticMethodID(ic, "valueOf", "(I)Ljava/lang/Integer;");
    jobject box = g_env->CallStaticObjectMethod(ic, vo, (jint)v);
    g_env->CallVoidMethod(fld, g_fieldSet, obj, box); exc_clear();
    g_env->DeleteLocalRef(box); g_env->DeleteLocalRef(ic);
}
// Call a no-arg boolean method on object (used for isAlive)
static bool r_call_bool(jobject obj, const char* clsName, const char* method, const char* sig) {
    jobject cls = r_class(clsName); if (!cls) return false;
    jclass  cc = g_env->FindClass("java/lang/Class");
    jmethodID m = g_env->GetMethodID(cc, "getMethod", "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
    g_env->DeleteLocalRef(cc);
    jstring nm = g_env->NewStringUTF(method);
    jobject methObj = g_env->CallObjectMethod(cls, m, nm, nullptr);
    g_env->DeleteLocalRef(nm); g_env->DeleteLocalRef(cls);
    if (exc_clear() || !methObj) return false;
    g_env->CallVoidMethod(methObj, g_setAccess, JNI_TRUE);
    jclass methCls = g_env->FindClass("java/lang/reflect/Method");
    jmethodID invoke = g_env->GetMethodID(methCls, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    g_env->DeleteLocalRef(methCls);
    jobject result = g_env->CallObjectMethod(methObj, invoke, obj, nullptr);
    g_env->DeleteLocalRef(methObj);
    if (exc_clear() || !result) return false;
    jclass boolCls = g_env->FindClass("java/lang/Boolean");
    jmethodID boolVal = g_env->GetMethodID(boolCls, "booleanValue", "()Z");
    jboolean bv = g_env->CallBooleanMethod(result, boolVal);
    g_env->DeleteLocalRef(boolCls); g_env->DeleteLocalRef(result);
    return (bool)bv;
}

// ─── Minecraft object access ──────────────────────────────────────────────────
//
//  1.21.11 obfuscated names (from Mojang client.txt / mappings.dev):
//    gfj  = net.minecraft.client.Minecraft
//      A  = static Minecraft instance
//      s  = LocalPlayer player
//      ab = IntegratedServer singleplayerServer
//    ddm  = net.minecraft.world.entity.player.Player
//      cG = Abilities abilities
//      cs = int experienceLevel
//      cu = float experienceProgress
//      cr = Inventory inventory
//    ddi  = net.minecraft.world.entity.player.Abilities
//      c  = boolean mayfly
//      b  = boolean flying
//      m  = float flyingSpeed
//    ddk  = net.minecraft.world.entity.player.Inventory
//      k  = int selected  (current hotbar slot)
//      h  = NonNullList<ItemStack> items  (main inventory)
//    cuq  = net.minecraft.world.item.ItemStack
//      s  = int count

static jobject get_mc_instance() {
    jobject cls = r_class("gfj"); if (!cls) return nullptr;
    jobject fld = r_field(cls, "A"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
    jobject mc = r_get(fld, nullptr); g_env->DeleteLocalRef(fld); return mc;
}
static jobject get_player(jobject mc) {
    jobject cls = r_class("gfj"); if (!cls) return nullptr;
    jobject fld = r_field(cls, "s"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
    jobject pl = r_get(fld, mc); g_env->DeleteLocalRef(fld); return pl;
}
static bool is_multiplayer(jobject mc) {
    jobject cls = r_class("gfj"); if (!cls) return false;
    jobject fld = r_field(cls, "ab"); g_env->DeleteLocalRef(cls); if (!fld) return false;
    jobject srv = r_get(fld, mc); g_env->DeleteLocalRef(fld);
    bool mp = (srv == nullptr); if (srv) g_env->DeleteLocalRef(srv); return mp;
}
static jobject get_abilities(jobject player) {
    jobject cls = r_class("ddm"); if (!cls) return nullptr;
    jobject fld = r_field(cls, "cG"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
    jobject ab = r_get(fld, player); g_env->DeleteLocalRef(fld); return ab;
}
// Returns the Inventory object from a player
static jobject get_inventory(jobject player) {
    jobject cls = r_class("ddm"); if (!cls) return nullptr;
    jobject fld = r_field(cls, "cr"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
    jobject inv = r_get(fld, player); g_env->DeleteLocalRef(fld); return inv;
}
// Returns integer value of the selected hotbar slot (0-8)
static int get_selected_slot(jobject inv) {
    jobject cls = r_class("ddk"); if (!cls) return 0;
    jobject fld = r_field(cls, "k"); g_env->DeleteLocalRef(cls); if (!fld) return 0;
    // Use reflection getInt
    jclass fc = g_env->FindClass("java/lang/reflect/Field");
    jmethodID getInt = g_env->GetMethodID(fc, "getInt", "(Ljava/lang/Object;)I");
    g_env->DeleteLocalRef(fc);
    jint v = g_env->CallIntMethod(fld, getInt, inv); exc_clear();
    g_env->DeleteLocalRef(fld); return (int)v;
}
// Returns ItemStack at given main-inventory slot index (0..35); caller must DeleteLocalRef
static jobject get_item_at(jobject inv, int slot) {
    jobject invCls = r_class("ddk"); if (!invCls) return nullptr;
    jobject fld = r_field(invCls, "h"); g_env->DeleteLocalRef(invCls); if (!fld) return nullptr;
    // h is a NonNullList<ItemStack>; NonNullList extends AbstractList, implements get(int)
    jobject list = r_get(fld, inv); g_env->DeleteLocalRef(fld); if (!list) return nullptr;
    jclass listCls = g_env->FindClass("java/util/List");
    jmethodID getM = g_env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
    g_env->DeleteLocalRef(listCls);
    jobject stack = g_env->CallObjectMethod(list, getM, (jint)slot); exc_clear();
    g_env->DeleteLocalRef(list); return stack;
}

// ─── Internal state ───────────────────────────────────────────────────────────
static bool  g_inited = false;
static bool  g_fly = false;
static float g_flySpeed = 0.05f;
static int   g_xpLevel = 0;
static float g_xpProg = 0.0f;
static int   g_itemCount = -1;   // -1 = don't touch

// Last known player pointer (as global ref) — used to detect player change/death
static jobject g_lastPlayer = nullptr;

static void apply()
{
    jobject mc = get_mc_instance(); if (!mc) return;
    if (is_multiplayer(mc)) { g_env->DeleteLocalRef(mc); return; }
    jobject player = get_player(mc); g_env->DeleteLocalRef(mc);
    if (!player) {
        // Player is null → dead or not loaded: release cached ref
        if (g_lastPlayer) { g_env->DeleteGlobalRef(g_lastPlayer); g_lastPlayer = nullptr; }
        return;
    }

    // ── Detect player change (respawn creates a new object) ──────────────────
    bool playerChanged = false;
    if (!g_lastPlayer) {
        g_lastPlayer = g_env->NewGlobalRef(player);
        playerChanged = true;
    }
    else if (!g_env->IsSameObject(g_lastPlayer, player)) {
        g_env->DeleteGlobalRef(g_lastPlayer);
        g_lastPlayer = g_env->NewGlobalRef(player);
        playerChanged = true;
    }
    (void)playerChanged; // re-apply everything regardless

    // ── Abilities ─────────────────────────────────────────────────────────────
    jobject ab = get_abilities(player);
    if (ab) {
        jobject abCls = r_class("ddi");
        if (abCls) {
            jobject fMayfly = r_field(abCls, "c");
            jobject fFlying = r_field(abCls, "b");
            jobject fSpeed = r_field(abCls, "m");
            if (fMayfly) { r_set_bool(fMayfly, ab, g_fly);       g_env->DeleteLocalRef(fMayfly); }
            if (fFlying) { r_set_bool(fFlying, ab, g_fly);       g_env->DeleteLocalRef(fFlying); }
            if (fSpeed) { r_set_float(fSpeed, ab, g_flySpeed); g_env->DeleteLocalRef(fSpeed); }
            g_env->DeleteLocalRef(abCls);
        }
        g_env->DeleteLocalRef(ab);
    }

    // ── XP ───────────────────────────────────────────────────────────────────
    jobject plCls = r_class("ddm");
    if (plCls) {
        jobject fLvl = r_field(plCls, "cs");
        jobject fProg = r_field(plCls, "cu");
        if (fLvl) { r_set_boxed(fLvl, player, g_xpLevel); g_env->DeleteLocalRef(fLvl); }
        if (fProg) { r_set_float(fProg, player, g_xpProg);  g_env->DeleteLocalRef(fProg); }
        g_env->DeleteLocalRef(plCls);
    }

    // ── Item count ────────────────────────────────────────────────────────────
    if (g_itemCount >= 0) {
        jobject inv = get_inventory(player);
        if (inv) {
            int slot = get_selected_slot(inv);
            jobject stack = get_item_at(inv, slot);
            if (stack) {
                // Check not empty: ItemStack.isEmpty()
                jobject stackCls = r_class("cuq");
                if (stackCls) {
                    // Use setInt on count field
                    jobject fCount = r_field(stackCls, "s");
                    if (fCount) {
                        r_set_int(fCount, stack, g_itemCount);
                        g_env->DeleteLocalRef(fCount);
                    }
                    g_env->DeleteLocalRef(stackCls);
                }
                g_env->DeleteLocalRef(stack);
            }
            g_env->DeleteLocalRef(inv);
        }
    }

    g_env->DeleteLocalRef(player);
}

// ─── Public API ───────────────────────────────────────────────────────────────
MCResult MC_Init() {
    if (g_inited) return MC_OK;
    set_err("");
    if (!jvm_load())     return MC_ERR_NO_JVM;
    if (!reflect_init()) return MC_ERR_REFLECT;
    g_inited = true;
    return MC_OK;
}

void MC_Shutdown() {
    if (!g_inited) return;
    g_fly = false; g_itemCount = -1; apply();
    if (g_lastPlayer) { g_env->DeleteGlobalRef(g_lastPlayer); g_lastPlayer = nullptr; }
    if (g_loader) { g_env->DeleteGlobalRef(g_loader);   g_loader = nullptr; }
    if (g_classCls) { g_env->DeleteGlobalRef(g_classCls); g_classCls = nullptr; }
    if (g_fieldCls) { g_env->DeleteGlobalRef(g_fieldCls); g_fieldCls = nullptr; }
    jvm_unload(); g_inited = false;
}

int MC_IsInGame() {
    if (!g_inited) return 0;
    jobject mc = get_mc_instance(); if (!mc) return 0;
    if (is_multiplayer(mc)) { g_env->DeleteLocalRef(mc); return 0; }
    jobject pl = get_player(mc); g_env->DeleteLocalRef(mc);
    int yes = pl != nullptr; if (pl) g_env->DeleteLocalRef(pl); return yes;
}

MCResult MC_SetFly(int enabled) { if (!g_inited) return MC_ERR_NO_JVM; g_fly = enabled != 0; apply(); return MC_OK; }
int      MC_GetFly() { return g_fly ? 1 : 0; }
MCResult MC_SetFlySpeed(float speed) { if (!g_inited) return MC_ERR_NO_JVM; if (speed < 0.01f)speed = 0.01f; if (speed > 2.f)speed = 2.f; g_flySpeed = speed; apply(); return MC_OK; }
float    MC_GetFlySpeed() { return g_flySpeed; }
MCResult MC_SetXpLevel(int level) { if (!g_inited) return MC_ERR_NO_JVM; if (level < 0)level = 0; if (level > 9999)level = 9999; g_xpLevel = level; apply(); return MC_OK; }
int      MC_GetXpLevel() { return g_xpLevel; }
MCResult MC_SetXpProgress(float p) { if (!g_inited) return MC_ERR_NO_JVM; if (p < 0)p = 0; if (p > 1)p = 1; g_xpProg = p; apply(); return MC_OK; }

MCResult MC_SetSelectedItemCount(int count) {
    if (!g_inited) return MC_ERR_NO_JVM;
    if (count < 1)  count = 1;
    if (count > 99) count = 99;
    g_itemCount = count; apply(); return MC_OK;
}
int MC_GetSelectedItemCount() {
    if (!g_inited || !MC_IsInGame()) return -1;
    jobject mc = get_mc_instance(); if (!mc) return -1;
    jobject pl = get_player(mc); g_env->DeleteLocalRef(mc); if (!pl) return -1;
    jobject inv = get_inventory(pl); g_env->DeleteLocalRef(pl); if (!inv) return -1;
    int slot = get_selected_slot(inv);
    jobject stack = get_item_at(inv, slot); g_env->DeleteLocalRef(inv);
    if (!stack) return 0;
    // Read count field
    jobject stackCls = r_class("cuq"); int cnt = 0;
    if (stackCls) {
        jobject fCount = r_field(stackCls, "s");
        if (fCount) {
            jclass fc = g_env->FindClass("java/lang/reflect/Field");
            jmethodID getInt = g_env->GetMethodID(fc, "getInt", "(Ljava/lang/Object;)I");
            g_env->DeleteLocalRef(fc);
            cnt = (int)g_env->CallIntMethod(fCount, getInt, stack); exc_clear();
            g_env->DeleteLocalRef(fCount);
        }
        g_env->DeleteLocalRef(stackCls);
    }
    g_env->DeleteLocalRef(stack); return cnt;
}
//#include "mclib.h"
//#include <Windows.h>
//#include "jni/jni.h"
//#include <string>
//#include <iostream>
//
//// ─── Error string ─────────────────────────────────────────────────────────────
//static char g_err[256] = "";
//static void set_err(const char* msg) { strncpy_s(g_err, sizeof(g_err), msg, _TRUNCATE); }
//const char* MC_GetError() { return g_err; }
//
//// ─── JVM ─────────────────────────────────────────────────────────────────────
//static JavaVM* g_vm = nullptr;
//static JNIEnv* g_env = nullptr;
//
//static bool jvm_load()
//{
//    jsize count = 0;
//    if (JNI_GetCreatedJavaVMs(&g_vm, 1, &count) != JNI_OK) {
//        set_err("JNI_GetCreatedJavaVMs failed — is java.dll loaded?"); return false;
//    }
//    if (count == 0 || !g_vm) {
//        set_err("No running JVM found — inject after Minecraft has started"); return false;
//    }
//    jint r = g_vm->GetEnv(reinterpret_cast<void**>(&g_env), JNI_VERSION_1_8);
//    if (r == JNI_EDETACHED) {
//        JavaVMAttachArgs args{}; args.version = JNI_VERSION_1_8;
//        args.name = const_cast<char*>("mclib");
//        r = g_vm->AttachCurrentThreadAsDaemon(reinterpret_cast<void**>(&g_env), &args);
//        if (r != JNI_OK) { set_err("AttachCurrentThreadAsDaemon failed"); return false; }
//    }
//    if (r != JNI_OK || !g_env) { set_err("GetEnv failed"); return false; }
//    return true;
//}
//static void jvm_unload() { if (g_vm) g_vm->DetachCurrentThread(); g_vm = nullptr; g_env = nullptr; }
//static bool exc_clear(const char* = "") { if (!g_env->ExceptionCheck()) return false; g_env->ExceptionClear(); return true; }
//
//// ─── Reflection helpers ───────────────────────────────────────────────────────
//static jobject   g_loader = nullptr;
//static jclass    g_classCls = nullptr;
//static jmethodID g_forName3 = nullptr;
//static jclass    g_fieldCls = nullptr;
//static jmethodID g_fieldGet = nullptr;
//static jmethodID g_fieldSet = nullptr;
//static jmethodID g_fieldSetFl = nullptr;
//static jmethodID g_fieldSetBo = nullptr;
//static jmethodID g_fieldSetInt = nullptr;  // setInt primitive
//static jmethodID g_setAccess = nullptr;
//
//static jobject r_class(const char* name) {
//    jstring s = g_env->NewStringUTF(name);
//    jobject c = g_env->CallStaticObjectMethod(g_classCls, g_forName3, s, JNI_TRUE, g_loader);
//    g_env->DeleteLocalRef(s); if (exc_clear(name)) return nullptr; return c;
//}
//
//static bool reflect_init()
//{
//    // ── Classloader ───────────────────────────────────────────────────────────
//    jclass threadCls = g_env->FindClass("java/lang/Thread");
//    if (!threadCls) { set_err("FindClass(Thread) failed"); return false; }
//    jmethodID getAllStack = g_env->GetStaticMethodID(threadCls, "getAllStackTraces", "()Ljava/util/Map;");
//    jmethodID getLoader = g_env->GetMethodID(threadCls, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
//    jmethodID setLoader = g_env->GetMethodID(threadCls, "setContextClassLoader", "(Ljava/lang/ClassLoader;)V");
//    jmethodID curThread = g_env->GetStaticMethodID(threadCls, "currentThread", "()Ljava/lang/Thread;");
//    jobject map = g_env->CallStaticObjectMethod(threadCls, getAllStack);
//    jclass mapCls = g_env->FindClass("java/util/Map");
//    jclass setCls = g_env->FindClass("java/util/Set");
//    jclass iterCls = g_env->FindClass("java/util/Iterator");
//    jobject keySet = g_env->CallObjectMethod(map, g_env->GetMethodID(mapCls, "keySet", "()Ljava/util/Set;"));
//    jobject iter = g_env->CallObjectMethod(keySet, g_env->GetMethodID(setCls, "iterator", "()Ljava/util/Iterator;"));
//    jmethodID hasNext = g_env->GetMethodID(iterCls, "hasNext", "()Z");
//    jmethodID next = g_env->GetMethodID(iterCls, "next", "()Ljava/lang/Object;");
//    while (g_env->CallBooleanMethod(iter, hasNext)) {
//        jobject thread = g_env->CallObjectMethod(iter, next);
//        jobject loader = g_env->CallObjectMethod(thread, getLoader); exc_clear();
//        if (loader) { g_loader = g_env->NewGlobalRef(loader); g_env->DeleteLocalRef(loader); g_env->DeleteLocalRef(thread); break; }
//        g_env->DeleteLocalRef(thread);
//    }
//    g_env->DeleteLocalRef(iter); g_env->DeleteLocalRef(keySet); g_env->DeleteLocalRef(map);
//    if (!g_loader) { set_err("No classloader found"); return false; }
//    jobject self = g_env->CallStaticObjectMethod(threadCls, curThread);
//    g_env->CallVoidMethod(self, setLoader, g_loader);
//    g_env->DeleteLocalRef(self); g_env->DeleteLocalRef(threadCls);
//
//    // ── Bootstrap reflection handles ──────────────────────────────────────────
//    auto global = [](jclass c)->jclass { jclass g = (jclass)g_env->NewGlobalRef(c); g_env->DeleteLocalRef(c); return g; };
//    g_classCls = global(g_env->FindClass("java/lang/Class"));
//    if (!g_classCls) { set_err("FindClass(Class) failed"); return false; }
//    g_forName3 = g_env->GetStaticMethodID(g_classCls, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
//    if (!g_forName3) { set_err("GetMethodID(Class.forName) failed"); return false; }
//    g_fieldCls = global(g_env->FindClass("java/lang/reflect/Field"));
//    if (!g_fieldCls) { set_err("FindClass(Field) failed"); return false; }
//    g_fieldGet = g_env->GetMethodID(g_fieldCls, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
//    g_fieldSet = g_env->GetMethodID(g_fieldCls, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V");
//    g_fieldSetFl = g_env->GetMethodID(g_fieldCls, "setFloat", "(Ljava/lang/Object;F)V");
//    g_fieldSetBo = g_env->GetMethodID(g_fieldCls, "setBoolean", "(Ljava/lang/Object;Z)V");
//    g_fieldSetInt = g_env->GetMethodID(g_fieldCls, "setInt", "(Ljava/lang/Object;I)V");
//    if (!g_fieldGet || !g_fieldSet || !g_fieldSetFl || !g_fieldSetBo || !g_fieldSetInt) {
//        set_err("GetMethodID on Field accessor methods failed"); return false;
//    }
//    jclass accCls = g_env->FindClass("java/lang/reflect/AccessibleObject");
//    if (!accCls) { set_err("FindClass(AccessibleObject) failed"); return false; }
//    g_setAccess = g_env->GetMethodID(accCls, "setAccessible", "(Z)V");
//    g_env->DeleteLocalRef(accCls);
//    if (!g_setAccess) { set_err("GetMethodID(setAccessible) failed"); return false; }
//
//    // ── Verify MC class reachable ─────────────────────────────────────────────
//    jobject testCls = r_class("gfj");
//    if (!testCls) { set_err("Class 'gfj' (Minecraft) not found — wrong MC version"); return false; }
//    g_env->DeleteLocalRef(testCls);
//    return true;
//}
//
//// ─── Field helpers ────────────────────────────────────────────────────────────
//static jobject r_field(jobject cls, const char* name) {
//    jclass cc = g_env->FindClass("java/lang/Class");
//    jmethodID m = g_env->GetMethodID(cc, "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
//    g_env->DeleteLocalRef(cc);
//    jstring s = g_env->NewStringUTF(name);
//    jobject fld = g_env->CallObjectMethod(cls, m, s);
//    g_env->DeleteLocalRef(s);
//    if (exc_clear(name)) return nullptr;
//    g_env->CallVoidMethod(fld, g_setAccess, JNI_TRUE); exc_clear();
//    return fld;
//}
//static jobject r_get(jobject fld, jobject obj) { jobject r = g_env->CallObjectMethod(fld, g_fieldGet, obj); exc_clear(); return r; }
//static void    r_set_bool(jobject fld, jobject obj, bool v) { g_env->CallVoidMethod(fld, g_fieldSetBo, obj, v ? JNI_TRUE : JNI_FALSE); exc_clear(); }
//static void    r_set_float(jobject fld, jobject obj, float v) { g_env->CallVoidMethod(fld, g_fieldSetFl, obj, (jfloat)v); exc_clear(); }
//static void    r_set_int(jobject fld, jobject obj, int v) { g_env->CallVoidMethod(fld, g_fieldSetInt, obj, (jint)v); exc_clear(); }
//static void    r_set_boxed(jobject fld, jobject obj, int v) {
//    jclass ic = g_env->FindClass("java/lang/Integer");
//    jmethodID vo = g_env->GetStaticMethodID(ic, "valueOf", "(I)Ljava/lang/Integer;");
//    jobject box = g_env->CallStaticObjectMethod(ic, vo, (jint)v);
//    g_env->CallVoidMethod(fld, g_fieldSet, obj, box); exc_clear();
//    g_env->DeleteLocalRef(box); g_env->DeleteLocalRef(ic);
//}
//// Call a no-arg boolean method on object (used for isAlive)
//static bool r_call_bool(jobject obj, const char* clsName, const char* method, const char* sig) {
//    jobject cls = r_class(clsName); if (!cls) return false;
//    jclass  cc = g_env->FindClass("java/lang/Class");
//    jmethodID m = g_env->GetMethodID(cc, "getMethod", "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
//    g_env->DeleteLocalRef(cc);
//    jstring nm = g_env->NewStringUTF(method);
//    jobject methObj = g_env->CallObjectMethod(cls, m, nm, nullptr);
//    g_env->DeleteLocalRef(nm); g_env->DeleteLocalRef(cls);
//    if (exc_clear() || !methObj) return false;
//    g_env->CallVoidMethod(methObj, g_setAccess, JNI_TRUE);
//    jclass methCls = g_env->FindClass("java/lang/reflect/Method");
//    jmethodID invoke = g_env->GetMethodID(methCls, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
//    g_env->DeleteLocalRef(methCls);
//    jobject result = g_env->CallObjectMethod(methObj, invoke, obj, nullptr);
//    g_env->DeleteLocalRef(methObj);
//    if (exc_clear() || !result) return false;
//    jclass boolCls = g_env->FindClass("java/lang/Boolean");
//    jmethodID boolVal = g_env->GetMethodID(boolCls, "booleanValue", "()Z");
//    jboolean bv = g_env->CallBooleanMethod(result, boolVal);
//    g_env->DeleteLocalRef(boolCls); g_env->DeleteLocalRef(result);
//    return (bool)bv;
//}
//
//// ─── Minecraft object access ──────────────────────────────────────────────────
////
////  1.21.11 obfuscated names (from Mojang client.txt / mappings.dev):
////    gfj  = net.minecraft.client.Minecraft
////      A  = static Minecraft instance
////      s  = LocalPlayer player
////      ab = IntegratedServer singleplayerServer
////    ddm  = net.minecraft.world.entity.player.Player
////      cG = Abilities abilities
////      cs = int experienceLevel
////      cu = float experienceProgress
////      cr = Inventory inventory
////    ddi  = net.minecraft.world.entity.player.Abilities
////      c  = boolean mayfly
////      b  = boolean flying
////      m  = float flyingSpeed
////    ddk  = net.minecraft.world.entity.player.Inventory
////      k  = int selected  (current hotbar slot)
////      h  = NonNullList<ItemStack> items  (main inventory)
////    cuq  = net.minecraft.world.item.ItemStack
////      s  = int count
//
//static jobject get_mc_instance() {
//    jobject cls = r_class("gfj"); if (!cls) return nullptr;
//    jobject fld = r_field(cls, "A"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
//    jobject mc = r_get(fld, nullptr); g_env->DeleteLocalRef(fld); return mc;
//}
//static jobject get_player(jobject mc) {
//    jobject cls = r_class("gfj"); if (!cls) return nullptr;
//    jobject fld = r_field(cls, "s"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
//    jobject pl = r_get(fld, mc); g_env->DeleteLocalRef(fld); return pl;
//}
//static bool is_multiplayer(jobject mc) {
//    jobject cls = r_class("gfj"); if (!cls) return false;
//    jobject fld = r_field(cls, "ab"); g_env->DeleteLocalRef(cls); if (!fld) return false;
//    jobject srv = r_get(fld, mc); g_env->DeleteLocalRef(fld);
//    bool mp = (srv == nullptr); if (srv) g_env->DeleteLocalRef(srv); return mp;
//}
//static jobject get_abilities(jobject player) {
//    jobject cls = r_class("ddm"); if (!cls) return nullptr;
//    jobject fld = r_field(cls, "cG"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
//    jobject ab = r_get(fld, player); g_env->DeleteLocalRef(fld); return ab;
//}
//// Returns the Inventory object from a player
//static jobject get_inventory(jobject player) {
//    jobject cls = r_class("ddm"); if (!cls) return nullptr;
//    jobject fld = r_field(cls, "cr"); g_env->DeleteLocalRef(cls); if (!fld) return nullptr;
//    jobject inv = r_get(fld, player); g_env->DeleteLocalRef(fld); return inv;
//}
//// Returns integer value of the selected hotbar slot (0-8)
//static int get_selected_slot(jobject inv) {
//    jobject cls = r_class("ddk"); if (!cls) return 0;
//    jobject fld = r_field(cls, "k"); g_env->DeleteLocalRef(cls); if (!fld) return 0;
//    // Use reflection getInt
//    jclass fc = g_env->FindClass("java/lang/reflect/Field");
//    jmethodID getInt = g_env->GetMethodID(fc, "getInt", "(Ljava/lang/Object;)I");
//    g_env->DeleteLocalRef(fc);
//    jint v = g_env->CallIntMethod(fld, getInt, inv); exc_clear();
//    g_env->DeleteLocalRef(fld); return (int)v;
//}
//// Returns ItemStack at given main-inventory slot index (0..35); caller must DeleteLocalRef
//static jobject get_item_at(jobject inv, int slot) {
//    jobject invCls = r_class("ddk"); if (!invCls) return nullptr;
//    jobject fld = r_field(invCls, "h"); g_env->DeleteLocalRef(invCls); if (!fld) return nullptr;
//    // h is a NonNullList<ItemStack>; NonNullList extends AbstractList, implements get(int)
//    jobject list = r_get(fld, inv); g_env->DeleteLocalRef(fld); if (!list) return nullptr;
//    jclass listCls = g_env->FindClass("java/util/List");
//    jmethodID getM = g_env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
//    g_env->DeleteLocalRef(listCls);
//    jobject stack = g_env->CallObjectMethod(list, getM, (jint)slot); exc_clear();
//    g_env->DeleteLocalRef(list); return stack;
//}
//
//// ─── Internal state ───────────────────────────────────────────────────────────
//static bool  g_inited = false;
//static bool  g_fly = false;
//static float g_flySpeed = 0.05f;
//static int   g_xpLevel = 0;
//static float g_xpProg = 0.0f;
//static int   g_itemCount = -1;   // -1 = don't touch
//
//// Last known player pointer (as global ref) — used to detect player change/death
//static jobject g_lastPlayer = nullptr;
//
//static void apply()
//{
//    jobject mc = get_mc_instance(); if (!mc) return;
//    if (is_multiplayer(mc)) { g_env->DeleteLocalRef(mc); return; }
//    jobject player = get_player(mc); g_env->DeleteLocalRef(mc);
//    if (!player) {
//        // Player is null → dead or not loaded: release cached ref
//        if (g_lastPlayer) { g_env->DeleteGlobalRef(g_lastPlayer); g_lastPlayer = nullptr; }
//        return;
//    }
//
//    // ── Detect player change (respawn creates a new object) ──────────────────
//    bool playerChanged = false;
//    if (!g_lastPlayer) {
//        g_lastPlayer = g_env->NewGlobalRef(player);
//        playerChanged = true;
//    }
//    else if (!g_env->IsSameObject(g_lastPlayer, player)) {
//        g_env->DeleteGlobalRef(g_lastPlayer);
//        g_lastPlayer = g_env->NewGlobalRef(player);
//        playerChanged = true;
//    }
//    (void)playerChanged; // re-apply everything regardless
//
//    // ── Abilities ─────────────────────────────────────────────────────────────
//    jobject ab = get_abilities(player);
//    if (ab) {
//        jobject abCls = r_class("ddi");
//        if (abCls) {
//            jobject fMayfly = r_field(abCls, "c");
//            jobject fFlying = r_field(abCls, "b");
//            jobject fSpeed = r_field(abCls, "m");
//            if (fMayfly) { r_set_bool(fMayfly, ab, g_fly);       g_env->DeleteLocalRef(fMayfly); }
//            if (fFlying) { r_set_bool(fFlying, ab, g_fly);       g_env->DeleteLocalRef(fFlying); }
//            if (fSpeed) { r_set_float(fSpeed, ab, g_flySpeed); g_env->DeleteLocalRef(fSpeed); }
//            g_env->DeleteLocalRef(abCls);
//        }
//        g_env->DeleteLocalRef(ab);
//    }
//
//    // ── XP ───────────────────────────────────────────────────────────────────
//    jobject plCls = r_class("ddm");
//    if (plCls) {
//        jobject fLvl = r_field(plCls, "cs");
//        jobject fProg = r_field(plCls, "cu");
//        if (fLvl) { r_set_boxed(fLvl, player, g_xpLevel); g_env->DeleteLocalRef(fLvl); }
//        if (fProg) { r_set_float(fProg, player, g_xpProg);  g_env->DeleteLocalRef(fProg); }
//        g_env->DeleteLocalRef(plCls);
//    }
//
//    // ── Item count ────────────────────────────────────────────────────────────
//    if (g_itemCount >= 0) {
//        jobject inv = get_inventory(player);
//        if (inv) {
//            int slot = get_selected_slot(inv);
//            jobject stack = get_item_at(inv, slot);
//            if (stack) {
//                // Check not empty: ItemStack.isEmpty()
//                jobject stackCls = r_class("cuq");
//                if (stackCls) {
//                    // Use setInt on count field
//                    jobject fCount = r_field(stackCls, "s");
//                    if (fCount) {
//                        r_set_int(fCount, stack, g_itemCount);
//                        g_env->DeleteLocalRef(fCount);
//                    }
//                    g_env->DeleteLocalRef(stackCls);
//                }
//                g_env->DeleteLocalRef(stack);
//            }
//            g_env->DeleteLocalRef(inv);
//        }
//    }
//
//    g_env->DeleteLocalRef(player);
//}
//
//// ─── Public API ───────────────────────────────────────────────────────────────
//MCResult MC_Init() {
//    if (g_inited) return MC_OK;
//    set_err("");
//    if (!jvm_load())     return MC_ERR_NO_JVM;
//    if (!reflect_init()) return MC_ERR_REFLECT;
//    g_inited = true;
//    return MC_OK;
//}
//
//void MC_Shutdown() {
//    if (!g_inited) return;
//    g_fly = false; g_itemCount = -1; apply();
//    if (g_lastPlayer) { g_env->DeleteGlobalRef(g_lastPlayer); g_lastPlayer = nullptr; }
//    if (g_loader) { g_env->DeleteGlobalRef(g_loader);   g_loader = nullptr; }
//    if (g_classCls) { g_env->DeleteGlobalRef(g_classCls); g_classCls = nullptr; }
//    if (g_fieldCls) { g_env->DeleteGlobalRef(g_fieldCls); g_fieldCls = nullptr; }
//    jvm_unload(); g_inited = false;
//}
//
//int MC_IsInGame() {
//    if (!g_inited) return 0;
//    jobject mc = get_mc_instance(); if (!mc) return 0;
//    if (is_multiplayer(mc)) { g_env->DeleteLocalRef(mc); return 0; }
//    jobject pl = get_player(mc); g_env->DeleteLocalRef(mc);
//    int yes = pl != nullptr; if (pl) g_env->DeleteLocalRef(pl); return yes;
//}
//
//MCResult MC_SetFly(int enabled) { if (!g_inited) return MC_ERR_NO_JVM; g_fly = enabled != 0; apply(); return MC_OK; }
//int      MC_GetFly() { return g_fly ? 1 : 0; }
//MCResult MC_SetFlySpeed(float speed) { if (!g_inited) return MC_ERR_NO_JVM; if (speed < 0.01f)speed = 0.01f; if (speed > 2.f)speed = 2.f; g_flySpeed = speed; apply(); return MC_OK; }
//float    MC_GetFlySpeed() { return g_flySpeed; }
//MCResult MC_SetXpLevel(int level) { if (!g_inited) return MC_ERR_NO_JVM; if (level < 0)level = 0; if (level > 9999)level = 9999; g_xpLevel = level; apply(); return MC_OK; }
//int      MC_GetXpLevel() { return g_xpLevel; }
//MCResult MC_SetXpProgress(float p) { if (!g_inited) return MC_ERR_NO_JVM; if (p < 0)p = 0; if (p > 1)p = 1; g_xpProg = p; apply(); return MC_OK; }
//
//MCResult MC_SetSelectedItemCount(int count) {
//    if (!g_inited) return MC_ERR_NO_JVM;
//    if (count < 1)  count = 1;
//    if (count > 99) count = 99;
//    g_itemCount = count; apply(); return MC_OK;
//}
//int MC_GetSelectedItemCount() {
//    if (!g_inited || !MC_IsInGame()) return -1;
//    jobject mc = get_mc_instance(); if (!mc) return -1;
//    jobject pl = get_player(mc); g_env->DeleteLocalRef(mc); if (!pl) return -1;
//    jobject inv = get_inventory(pl); g_env->DeleteLocalRef(pl); if (!inv) return -1;
//    int slot = get_selected_slot(inv);
//    jobject stack = get_item_at(inv, slot); g_env->DeleteLocalRef(inv);
//    if (!stack) return 0;
//    // Read count field
//    jobject stackCls = r_class("cuq"); int cnt = 0;
//    if (stackCls) {
//        jobject fCount = r_field(stackCls, "s");
//        if (fCount) {
//            jclass fc = g_env->FindClass("java/lang/reflect/Field");
//            jmethodID getInt = g_env->GetMethodID(fc, "getInt", "(Ljava/lang/Object;)I");
//            g_env->DeleteLocalRef(fc);
//            cnt = (int)g_env->CallIntMethod(fCount, getInt, stack); exc_clear();
//            g_env->DeleteLocalRef(fCount);
//        }
//        g_env->DeleteLocalRef(stackCls);
//    }
//    g_env->DeleteLocalRef(stack); return cnt;
//}