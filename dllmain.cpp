#include "pch.h"
#include <Windows.h>

HWND g_hwnd = nullptr;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == GetCurrentProcessId())
    {
        g_hwnd = hwnd;
        return FALSE; // stop once we found the window
    }

    return TRUE;
}

DWORD WINAPI MainThread(LPVOID)
{
    Sleep(2000); // allow window to initialize

    // find window belonging to this process
    EnumWindows(EnumWindowsProc, 0);

    if (!g_hwnd)
        return 0;

    while (true)
    {
        HDC hdc = GetDC(g_hwnd);

        if (hdc)
        {
            TextOutW(hdc, 250, 250, L"Injected Overlay", 16);
            ReleaseDC(g_hwnd, hdc);
        }

        Sleep(16); // ~60 FPS
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD reason,
                      LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule); // small optimization
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }

    return TRUE;
}
