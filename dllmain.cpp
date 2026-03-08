#include "pch.h"
#include "Config.hpp"
#include <random>
#include <algorithm>
#include "Graphics.hpp"

struct ClickStats
{
    int clickCount = 0;
    int lastDelay = 0;
    float cps = 0.0f;
    float avgCps = 0.0f;
};

static ClickStats g_stats{};
CRITICAL_SECTION g_statsLock;
HWND g_hwnd = nullptr;
static AcConfig g_cfg{};
bool g_leftEnabled = false;
bool g_rightEnabled = false;
Graphics* Gfx = nullptr;


int GetHumanClickDelay(int minCPS = 15, int maxCPS = 20)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // --- smooth CPS drift ---
    static double currentCPS = (minCPS + maxCPS) / 2.0;
    std::normal_distribution<> cpsDrift(0.0, 0.2);
    currentCPS += cpsDrift(gen);
    currentCPS = std::clamp(currentCPS, (double)minCPS, (double)maxCPS);

    int baseDelay = static_cast<int>(1000.0 / currentCPS);

    // --- gaussian jitter ---
    double jitterFactor = 0.05 + (gen() % 5) * 0.01;
    std::normal_distribution<> jitterDist(0.0, baseDelay * jitterFactor);
    int delay = baseDelay + static_cast<int>(jitterDist(gen));

    // --- micro fatigue/fatigue bursts ---
    static int clickCounter = 0;
    clickCounter++;

    if (clickCounter % (10 + gen() % 30) == 0)
        delay += 5 + gen() % 12;

    // --- hesitation & burst  ---
    std::uniform_int_distribution<> chance(1, 100);
    int roll = chance(gen);

    if (roll <= 3)
        delay += 20 + gen() % 50;
    else if (roll <= 8)
        delay -= 3 + gen() % 6;

    // --- random drops  ---
    if (chance(gen) <= 2)
        delay += 50 + gen() % 100;

    // --- random spikes ---
    if (chance(gen) <= 2)
        delay = std::max<int>(1, delay - (30 + gen() % 50));

    return std::max(delay, 1);
}


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == GetCurrentProcessId())
    {
        g_hwnd = hwnd;
        return FALSE;
    }

    return TRUE;
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
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return 0;

    HWND hOverlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        L"AcDbgOverlay", nullptr,
        WS_CHILD,
        0, 0, wFact, hFact,
        g_hwnd, nullptr, GetModuleHandleW(nullptr), nullptr
    );

    if (!hOverlay) return 0;

    SetLayeredWindowAttributes(hOverlay, 0, 100, LWA_ALPHA);
    SetWindowPos(hOverlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(hOverlay, SW_SHOWNOACTIVATE);

    Gfx = new Graphics(hOverlay);
    if (!Gfx->Init()) { delete Gfx; Gfx = nullptr; return 0; }

    MSG msg{};
    RECT rect{};
    while (true) {
        while (PeekMessageW(&msg, hOverlay, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
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
        Gfx->FillRoundedRect(0, 0, wFact, hFact, 6, D2D1::ColorF(0.08f, 0.08f, 0.08f, 1.0f));

        EnterCriticalSection(&g_statsLock);
        float curCps = g_stats.cps;
        float avgCps = g_stats.avgCps;
        float expCps = g_cfg.cps;
        LeaveCriticalSection(&g_statsLock);

        wchar_t buf[64];
        float colW = wFact / 3.0f;
        float lastColW = wFact - colW * 2;

        int col0x = 0, col0w = wFact / 3;
        int col1x = col0w, col1w = wFact / 3;
        int col2x = col1x + col1w, col2w = wFact - col2x;

        swprintf(buf, 64, L"CUR  %.1f", curCps);
        Gfx->DrawTextCentered(buf, col0x, 0, col0w, hFact, D2D1::ColorF(0.4f, 1.0f, 0.5f, 1.0f), 11.0f);

        swprintf(buf, 64, L"AVG  %.1f", avgCps);
        Gfx->DrawTextCentered(buf, col1x, 0, col1w, hFact, D2D1::ColorF(0.4f, 0.7f, 1.0f, 1.0f), 11.0f);

        swprintf(buf, 64, L"EXP  %.1f", expCps);
        Gfx->DrawTextCentered(buf, col2x, 0, col2w, hFact, D2D1::ColorF(1.0f, 0.8f, 0.3f, 1.0f), 11.0f);

        Gfx->DrawLine(col1x, 4, col1x, hFact - 4, D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f), 1.0f);
        Gfx->DrawLine(col2x, 4, col2x, hFact - 4, D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f), 1.0f);

        Gfx->EndDraw();
        Sleep(1000 / 20);
    }
    return 0;
}

DWORD WINAPI ControlPanelThread(LPVOID) {
    return 0;
}

bool IsKeyPressedOnce(int vk)
{
    static SHORT lastState[256] = {};

    SHORT state = GetAsyncKeyState(vk);

    bool pressed = (state & 0x8000) && !(lastState[vk] & 0x8000);

    lastState[vk] = state;

    return pressed;
}


DWORD WINAPI CPSThread(LPVOID)
{
    while (true)
    {
        if (IsKeyPressedOnce(g_cfg.lClickVK)) {
            g_leftEnabled = !g_leftEnabled;
            if (g_leftEnabled && g_rightEnabled) {
                g_rightEnabled = !g_rightEnabled;
            }
        }

        if (IsKeyPressedOnce(g_cfg.rClickVK)) {
            g_rightEnabled = !g_rightEnabled;
            if (g_leftEnabled && g_rightEnabled) {
                g_leftEnabled = !g_leftEnabled;
            }
        }


        if (g_leftEnabled)
        {
            INPUT input{};
            input.type = INPUT_MOUSE;

            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &input, sizeof(INPUT));

            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(INPUT));

            int delay = GetHumanClickDelay(g_cfg.cps - 3, g_cfg.cps);

            EnterCriticalSection(&g_statsLock);
            g_stats.clickCount++;
            g_stats.lastDelay = delay;
            g_stats.cps = 1000.0f / delay;
            float clampedCps = std::clamp(g_stats.cps, g_cfg.cps - 4.0f, g_cfg.cps + 2.0f);
            g_stats.avgCps += (clampedCps - g_stats.avgCps) * 0.05f;
            LeaveCriticalSection(&g_statsLock);

            Sleep(delay);
        }
        if (g_rightEnabled)
        {
            INPUT input{};
            input.type = INPUT_MOUSE;

            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            SendInput(1, &input, sizeof(INPUT));

            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            SendInput(1, &input, sizeof(INPUT));

            int delay = GetHumanClickDelay(g_cfg.cps - 3, g_cfg.cps);

            EnterCriticalSection(&g_statsLock);
            g_stats.clickCount++;
            g_stats.lastDelay = delay;
            g_stats.cps = 1000.0f / delay;
            LeaveCriticalSection(&g_statsLock);

            Sleep(delay);
        }
        else
        {
            Sleep(1);
        }
    }

    return 0;
}

DWORD WINAPI MainThread(LPVOID)
{
    Sleep(2000);
    if (!LoadConfig(g_cfg)) {
        // fallback defaults
        g_cfg.cps = 18.0f;
        g_cfg.cooldown = 1.0f;
        g_cfg.triggerCooldown = 4.0f;
        g_cfg.lClickVK = VK_F9;
        g_cfg.rClickVK = VK_F10;
        g_cfg.debugPanel = true;
        g_cfg.controlDialog = true;
    }

    EnumWindows(EnumWindowsProc, 0);
    InitializeCriticalSection(&g_statsLock);
    g_stats.avgCps = g_cfg.cps;
    CreateThread(nullptr, 0, CPSThread, nullptr, 0, nullptr);
    if (g_cfg.debugPanel) CreateThread(nullptr, 0, DebugThread, nullptr, 0, nullptr);
    if (g_cfg.controlDialog) CreateThread(nullptr, 0, ControlPanelThread, nullptr, 0, nullptr);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD reason,
                      LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
