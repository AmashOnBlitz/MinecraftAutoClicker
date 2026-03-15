#pragma once
#include "Graphics.hpp"
#include "IntBox.hpp"
#include <windows.h>

class PidInput {
public:
    static constexpr int   kCount = 6;
    static constexpr float kBoxSize = 28.0f;
    static constexpr float kGap = 6.0f;
    static constexpr float kLabelH = 16.0f;
    static constexpr float kLabelGap = 4.0f;

    PidInput(HWND hwnd, Graphics* gfx, float x, float y);
    ~PidInput();

    void Render();
    void OnMouseDown(float mx, float my);
    void OnMouseMove(float mx, float my);
    void OnMouseLeave();
    void OnChar(wchar_t ch);
    void OnKeyDown(int vk);

    int  GetPid() const;
    void SetPid(DWORD pid);

private:
    void MoveFocus(int idx);
    bool BrowseHitTest(float mx, float my) const;
    void BrowseGetRect(float& bx, float& by) const;
    void OpenBrowse();

    HWND      mHwnd;
    Graphics* mGfx;
    float     mX, mY;
    int       mFocusIdx = -1;
    bool      mBrowseHover = false;

    IntBox* mBoxes[kCount]{};

    static constexpr float kBrowseSize = 22.0f;
    static constexpr float kBrowseGap = 8.0f;
};