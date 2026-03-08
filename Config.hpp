// Config.hpp
#pragma once
#include <Windows.h>
#include <string>

struct AcConfig {
    float cps;
    float cooldown;
    float triggerCooldown;
    int   lClickVK;
    int   rClickVK;
    bool  debugPanel;
    bool  controlDialog;
};

inline std::wstring GetConfigPath() {
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    return std::wstring(tmp) + L"ac_config.bin";
}

inline bool SaveConfig(const AcConfig& cfg) {
    std::wstring path = GetConfigPath();
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0,
                               nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    WriteFile(hFile, &cfg, sizeof(cfg), &written, nullptr);
    CloseHandle(hFile);
    return written == sizeof(cfg);
}

inline bool LoadConfig(AcConfig& cfg) {
    std::wstring path = GetConfigPath();
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD read = 0;
    ReadFile(hFile, &cfg, sizeof(cfg), &read, nullptr);
    CloseHandle(hFile);
    return read == sizeof(cfg);
}