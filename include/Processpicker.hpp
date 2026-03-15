#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct ProcessEntry {
    DWORD        pid;
    std::wstring name;
};

DWORD ShowProcessPicker(HWND hParent);