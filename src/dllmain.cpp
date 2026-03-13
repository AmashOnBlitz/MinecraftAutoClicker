#include "pch.h"
#include "Config.hpp"
#include "ControlPanel.hpp"
#include <random>
#include <algorithm>
#include "Graphics.hpp"

struct ClickStats {
    int   clickCount = 0;
    int   lastDelay = 0;
    float cps = 0.0f;
    float avgCps = 0.0f;
};

ULONGLONG g_triggerStart = 0;
ULONGLONG g_cooldownStart = 0;
bool g_triggerActive = false;
bool g_cooldownActive = false;

ULONGLONG g_rightTriggerStart = 0;
ULONGLONG g_rightCooldownStart = 0;
bool g_rightTriggerActive = false;
bool g_rightCooldownActive = false;

static ClickStats g_stats{};
CRITICAL_SECTION  g_statsLock;
HWND              g_hwnd = nullptr;
AcConfig          g_cfg{};
bool              g_leftEnabled = false;
bool              g_rightEnabled = false;
Graphics* Gfx = nullptr;

static HWND g_overlayHwnd = nullptr;
static HWND g_controlHwnd = nullptr;
static bool g_debugVisible = true;
static bool g_controlVisible = false;

bool IsAppFocused() { return GetForegroundWindow() == g_hwnd; }

int GetHumanClickDelay(int minCPS = 15, int maxCPS = 20)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static double currentCPS = (minCPS + maxCPS) / 2.0;
    std::normal_distribution<> cpsDrift(0.0, 0.2);
    currentCPS += cpsDrift(gen);
    currentCPS = std::clamp(currentCPS, (double)minCPS, (double)maxCPS);
    int baseDelay = (int)(1000.0 / currentCPS);
    double jf = 0.05 + (gen() % 5) * 0.01;
    std::normal_distribution<> jd(0.0, baseDelay * jf);
    int delay = baseDelay + (int)jd(gen);
    static int cc = 0; cc++;
    if (cc % (10 + gen() % 30) == 0) delay += 5 + gen() % 12;
    std::uniform_int_distribution<> chance(1, 100);
    int roll = chance(gen);
    if (roll <= 3)      delay += 20 + gen() % 50;
    else if (roll <= 8) delay -= 3 + gen() % 6;
    if (chance(gen) <= 2) delay += 50 + gen() % 100;
    if (chance(gen) <= 2) delay = std::max<int>(1, delay - (30 + gen() % 50));
    return std::max(delay, 1);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) { g_hwnd = hwnd; return FALSE; }
    return TRUE;
}

static void EnsurePanelVisible(HWND panel)
{
    RECT pc{};
    GetClientRect(g_hwnd, &pc);

    RECT wr{};
    GetWindowRect(panel, &wr);

    POINT tl{ wr.left, wr.top };
    ScreenToClient(g_hwnd, &tl);

    int pw = wr.right - wr.left;
    int ph = wr.bottom - wr.top;

    bool invalid =
        tl.x < 0 ||
        tl.y < 0 ||
        tl.x + pw > pc.right ||
        tl.y + ph > pc.bottom;

    if (invalid)
    {
        int cx = (pc.right - pw) / 2;
        int cy = (pc.bottom - ph) / 2;

        SetWindowPos(panel, nullptr, cx, cy, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

DWORD WINAPI DebugThread(LPVOID)
{
    int wFact = 500;
    const int hFact = 30;
    while (!g_hwnd) Sleep(100);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"AcDbgOverlay";
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 0;

    HWND hOverlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        L"AcDbgOverlay", nullptr, WS_CHILD,
        0, 0, wFact, hFact,
        g_hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!hOverlay) return 0;

    SetLayeredWindowAttributes(hOverlay, 0, 100, LWA_ALPHA);
    SetWindowPos(hOverlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(hOverlay, SW_SHOWNOACTIVATE);
    g_overlayHwnd = hOverlay;

    Gfx = new Graphics(hOverlay);
    if (!Gfx->Init(true)) { delete Gfx; Gfx = nullptr; return 0; }

    MSG msg{}; RECT rect{};
    while (true) {
        while (PeekMessageW(&msg, hOverlay, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (!g_hwnd) break;
        GetClientRect(g_hwnd, &rect);
        int newW = (rect.right - rect.left) * 2 / 3;
        if (newW != wFact) {
            wFact = newW;
            SetWindowPos(hOverlay, HWND_TOP, 0, 0, wFact, hFact, SWP_NOMOVE | SWP_NOACTIVATE);
            Gfx->Resize(wFact, hFact);
        }
        Gfx->BeginDraw();
        Gfx->FillRoundedRect(0, 0, (float)wFact, (float)hFact, 6,
                             D2D1::ColorF(0.08f, 0.08f, 0.08f, 1.0f));
        EnterCriticalSection(&g_statsLock);
        float curCps = g_stats.cps, avgCps = g_stats.avgCps, expCps = g_cfg.cps;
        LeaveCriticalSection(&g_statsLock);
        wchar_t buf[64];
        float fW = (float)wFact;
        float c0x = 0.0f, c0w = floorf(fW / 3.0f);
        float c1x = c0w, c1w = floorf(fW / 3.0f);
        float c2x = c0w + c1w, c2w = fW - c2x;
        swprintf(buf, 64, L"CUR  %.1f", curCps);
        Gfx->DrawTextCentered(buf, c0x, 0, c0w, (float)hFact, D2D1::ColorF(0.4f, 1.0f, 0.5f, 1.0f), 20.0f);
        swprintf(buf, 64, L"AVG  %.1f", avgCps);
        Gfx->DrawTextCentered(buf, c1x, 0, c1w, (float)hFact, D2D1::ColorF(0.4f, 0.7f, 1.0f, 1.0f), 20.0f);
        swprintf(buf, 64, L"EXP  %.1f", expCps);
        Gfx->DrawTextCentered(buf, c2x, 0, c2w, (float)hFact, D2D1::ColorF(1.0f, 0.8f, 0.3f, 1.0f), 20.0f);
        Gfx->DrawLine(c1x, 4, c1x, (float)(hFact - 4), D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f), 1.0f);
        Gfx->DrawLine(c2x, 4, c2x, (float)(hFact - 4), D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f), 1.0f);
        Gfx->EndDraw();
        Sleep(1000 / 20);
    }
    return 0;
}

static void ApplyRoundedRegion(HWND hwnd, int w, int h)
{
    HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, 24, 24);
    SetWindowRgn(hwnd, rgn, TRUE);
}

static void ClampToParent(HWND panel)
{
    RECT pc{};
    GetClientRect(g_hwnd, &pc);
    RECT wr{};
    GetWindowRect(panel, &wr);
    POINT tl = { wr.left, wr.top };
    ScreenToClient(g_hwnd, &tl);
    int pw = wr.right - wr.left, ph = wr.bottom - wr.top;
    int px = tl.x, py = tl.y;
    if (px < 0)              px = 0;
    if (py < 0)              py = 0;
    if (px + pw > pc.right)  px = pc.right - pw;
    if (py + ph > pc.bottom) py = pc.bottom - ph;
    SetWindowPos(panel, nullptr, px, py, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CALLBACK ControlPanelWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ControlPanel* cp =
        reinterpret_cast<ControlPanel*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        cp = reinterpret_cast<ControlPanel*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cp));
        cp->SetStage(hwnd);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (cp) cp->Render();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        if (cp) cp->Resize(w, h);
        ApplyRoundedRegion(hwnd, w, h);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        float mx = (float)(short)LOWORD(lParam);
        float my = (float)(short)HIWORD(lParam);
        SetCapture(hwnd);
        if (cp) cp->OnMouseDown(mx, my);
        return 0;
    }
    case WM_MOUSEMOVE: {
        float mx = (float)(short)LOWORD(lParam);
        float my = (float)(short)HIWORD(lParam);
        if (cp) cp->OnMouseMove(mx, my);
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_LBUTTONUP: {
        float mx = (float)(short)LOWORD(lParam);
        float my = (float)(short)HIWORD(lParam);
        ReleaseCapture();
        if (cp) cp->OnMouseUp(mx, my);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (cp) cp->OnMouseLeave();
        return 0;
    case WM_MOUSEWHEEL: {
        POINT pt = { (LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam) };
        ScreenToClient(hwnd, &pt);
        if (cp) cp->OnMouseWheel((float)pt.x, (float)pt.y, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    case WM_SHOWWINDOW:
        if (wParam && g_hwnd) EnsurePanelVisible(hwnd);
        return 0;
    case WM_MOVING: {
        RECT* proposed = reinterpret_cast<RECT*>(lParam);
        RECT pc{};
        GetClientRect(g_hwnd, &pc);
        POINT origin{};
        ClientToScreen(g_hwnd, &origin);
        int pw = proposed->right - proposed->left;
        int ph = proposed->bottom - proposed->top;
        if (proposed->left < origin.x)                proposed->left = origin.x, proposed->right = origin.x + pw;
        if (proposed->top < origin.y)                proposed->top = origin.y, proposed->bottom = origin.y + ph;
        if (proposed->right > origin.x + pc.right)    proposed->right = origin.x + pc.right, proposed->left = proposed->right - pw;
        if (proposed->bottom > origin.y + pc.bottom)   proposed->bottom = origin.y + pc.bottom, proposed->top = proposed->bottom - ph;
        return TRUE;
    }
    case WM_NCHITTEST: {
        POINT pt = { (LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam) };
        ScreenToClient(hwnd, &pt);
        if (cp && cp->IsDragArea((float)pt.x, (float)pt.y))
            return HTCAPTION;
        return HTCLIENT;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DWORD WINAPI ControlPanelThread(LPVOID)
{
    while (!g_hwnd) Sleep(100);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = ControlPanelWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"AcControlPanel";
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 0;

    ControlPanel* cp = new ControlPanel();

    RECT pr{};
    GetClientRect(g_hwnd, &pr);
    const int cpW = 400;
    const int cpH = 420;
    int cpX = (pr.right - cpW) / 2;
    int cpY = (pr.bottom - cpH) / 2;

    HWND hwnd = CreateWindowExW(
        0, L"AcControlPanel", nullptr,
        WS_CHILD,
        cpX, cpY, cpW, cpH,
        g_hwnd, nullptr, GetModuleHandleW(nullptr), cp);

    if (!hwnd) { delete cp; return 0; }

    ApplyRoundedRegion(hwnd, cpW, cpH);
    g_controlHwnd = hwnd;
    g_controlVisible = false;

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    delete cp;
    return 0;
}

bool IsKeyPressedOnce(int vk) {
    static SHORT last[256] = {};
    SHORT s = GetAsyncKeyState(vk);
    bool  p = (s & 0x8000) && !(last[vk] & 0x8000);
    last[vk] = s;
    return p;
}

DWORD WINAPI CPSThread(LPVOID)
{
    while (true) {

        ULONGLONG now = GetTickCount64();

        if (g_triggerActive) {
            if (now - g_triggerStart >= (ULONGLONG)(g_cfg.triggerCooldown * 1000)) {
                g_triggerActive = false;
                g_cooldownActive = true;
                g_cooldownStart = now;
            }
        }

        if (g_cooldownActive) {
            if (now - g_cooldownStart >= (ULONGLONG)(g_cfg.cooldown * 1000)) {
                g_cooldownActive = false;
                g_triggerStart = now;
                g_triggerActive = true;
            }
            else {
                Sleep(1);
                continue;
            }
        }

        if (g_rightTriggerActive) {
            if (now - g_rightTriggerStart >= (ULONGLONG)(g_cfg.triggerCooldown * 1000)) {
                g_rightTriggerActive = false;
                g_rightCooldownActive = true;
                g_rightCooldownStart = now;
            }
        }

        if (g_rightCooldownActive) {
            if (now - g_rightCooldownStart >= (ULONGLONG)(g_cfg.cooldown * 1000)) {
                g_rightCooldownActive = false;
                g_rightTriggerStart = now;
                g_rightTriggerActive = true;
            }
            else {
                Sleep(1);
                continue;
            }
        }

        if (IsKeyPressedOnce(g_cfg.lClickVK)) {
            g_leftEnabled = !g_leftEnabled;
            if (g_leftEnabled) {
                g_triggerStart = GetTickCount64();
                g_triggerActive = true;
            }
            if (g_leftEnabled && g_rightEnabled) g_rightEnabled = false;
        }
        if (IsKeyPressedOnce(g_cfg.rClickVK)) {
            g_rightEnabled = !g_rightEnabled;
            if (g_rightEnabled) {
                g_rightTriggerStart = GetTickCount64();
                g_rightTriggerActive = true;
            }
            if (g_leftEnabled && g_rightEnabled)
                g_leftEnabled = false;
        }
        if (g_cfg.debugToggleVK && IsKeyPressedOnce(g_cfg.debugToggleVK)) {
            if (g_overlayHwnd) {
                g_debugVisible = !g_debugVisible;
                ShowWindow(g_overlayHwnd, g_debugVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
            }
        }
        if (g_cfg.controlToggleVK && IsKeyPressedOnce(g_cfg.controlToggleVK)) {
            if (g_controlHwnd) {
                g_controlVisible = !g_controlVisible;
                ShowWindow(g_controlHwnd, g_controlVisible ? SW_SHOWNOACTIVATE : SW_HIDE);

                if (g_controlVisible)
                {
                    EnsurePanelVisible(g_controlHwnd);
                    SetWindowPos(g_controlHwnd, HWND_TOP, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                }
            }
        }
        if (!IsAppFocused()) { g_leftEnabled = false; g_rightEnabled = false; }

        if (g_leftEnabled) {
            INPUT in{}; in.type = INPUT_MOUSE;
            in.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &in, sizeof(in));
            in.mi.dwFlags = MOUSEEVENTF_LEFTUP;   SendInput(1, &in, sizeof(in));
            int delay = GetHumanClickDelay((int)(g_cfg.cps - 3), (int)g_cfg.cps);
            EnterCriticalSection(&g_statsLock);
            g_stats.clickCount++;
            g_stats.lastDelay = delay;
            g_stats.cps = 1000.0f / delay;
            float c = std::clamp(g_stats.cps, g_cfg.cps - 4.0f, g_cfg.cps + 2.0f);
            g_stats.avgCps += (c - g_stats.avgCps) * 0.05f;
            LeaveCriticalSection(&g_statsLock);
            Sleep(delay);
        }
        if (g_rightEnabled) {
            INPUT in{}; in.type = INPUT_MOUSE;
            in.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; SendInput(1, &in, sizeof(in));
            in.mi.dwFlags = MOUSEEVENTF_RIGHTUP;   SendInput(1, &in, sizeof(in));
            int delay = GetHumanClickDelay((int)(g_cfg.cps - 3), (int)g_cfg.cps);
            EnterCriticalSection(&g_statsLock);
            g_stats.clickCount++;
            g_stats.lastDelay = delay;
            g_stats.cps = 1000.0f / delay;
            LeaveCriticalSection(&g_statsLock);
            Sleep(delay);
        }
        else { Sleep(1); }
    }
    return 0;
}

DWORD WINAPI MainThread(LPVOID)
{
    Sleep(2000);
    if (!LoadConfig(g_cfg)) {
        g_cfg.cps = 18.0f;
        g_cfg.cooldown = 1.0f;
        g_cfg.triggerCooldown = 4.0f;
        g_cfg.lClickVK = VK_F9;
        g_cfg.rClickVK = VK_F10;
        g_cfg.debugPanel = true;
        g_cfg.controlDialog = true;
        g_cfg.debugToggleVK = VK_F11;
        g_cfg.controlToggleVK = VK_F12;
    }
    EnumWindows(EnumWindowsProc, 0);
    InitializeCriticalSection(&g_statsLock);
    g_stats.avgCps = g_cfg.cps;
    CreateThread(nullptr, 0, CPSThread, nullptr, 0, nullptr);
    if (g_cfg.debugPanel)    CreateThread(nullptr, 0, DebugThread, nullptr, 0, nullptr);
    if (g_cfg.controlDialog) CreateThread(nullptr, 0, ControlPanelThread, nullptr, 0, nullptr);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}