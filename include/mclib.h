#pragma once
#include <Windows.h>
#ifdef __cplusplus
extern "C" {
#endif
    typedef enum MCResult {
        MC_OK = 0,
        MC_ERR_NO_JVM = 1,
        MC_ERR_NO_PLAYER = 2,
        MC_ERR_REFLECT = 3,
    } MCResult;

    MCResult MC_Init(void);
    void     MC_Shutdown(void);
    int      MC_IsInGame(void);

    MCResult MC_SetFly(int enabled);
    int      MC_GetFly(void);
    MCResult MC_SetFlySpeed(float speed);
    float    MC_GetFlySpeed(void);

    MCResult MC_SetXpLevel(int level);
    int      MC_GetXpLevel(void);
    MCResult MC_SetXpProgress(float progress);

    // Set count of item in currently selected hotbar slot (1-99)
    MCResult MC_SetSelectedItemCount(int count);
    // Get count of item in currently selected hotbar slot, -1 if slot empty
    int      MC_GetSelectedItemCount(void);

    const char* MC_GetError(void);
#ifdef __cplusplus
}
#endif

//#pragma once
//#include <Windows.h>
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//    typedef enum MCResult {
//        MC_OK = 0,
//        MC_ERR_NO_JVM = 1,
//        MC_ERR_NO_PLAYER = 2,
//        MC_ERR_REFLECT = 3,
//    } MCResult;
//
//    // Call once after injecting. Attaches to the JVM and resolves all handles.
//    MCResult MC_Init(void);
//
//    // Disable all active cheats and detach from the JVM.
//    void     MC_Shutdown(void);
//
//    // Returns 1 if the player is in a loaded singleplayer world, 0 otherwise.
//    // Always returns 0 on multiplayer servers (cheats are blocked there).
//    int      MC_IsInGame(void);
//
//    // Fly  ── toggle creative-style flight in survival
//    MCResult MC_SetFly(int enabled);          // 1 = on, 0 = off
//    int      MC_GetFly(void);
//
//    // Fly speed  ── 0.01 (crawl) … 2.0 (rocket).  Default Minecraft value is 0.05.
//    MCResult MC_SetFlySpeed(float speed);
//    float    MC_GetFlySpeed(void);
//
//    // XP  ── set exact experience level (0-9999) and optionally fill the bar
//    MCResult MC_SetXpLevel(int level);
//    int      MC_GetXpLevel(void);
//    MCResult MC_SetXpProgress(float progress); // 0.0 … 1.0
//
//    // Returns a human-readable description of the last error, or "" if none.
//    const char* MC_GetError(void);
//#ifdef __cplusplus
//}
//#endif