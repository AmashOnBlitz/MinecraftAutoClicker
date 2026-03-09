#pragma once
#ifndef PIDINPUT_CLASS_HEADER
#define PIDINPUT_CLASS_HEADER

#include "IntBox.hpp"

class PidInput {
public:
    PidInput(HWND hwnd, Graphics* gfx, float x, float y);
    ~PidInput();

    void Render();
    void OnMouseDown(float mx, float my);
    void OnChar(wchar_t ch);
    void OnKeyDown(int vk);

    int  GetPid() const;

private:
    void MoveFocus(int idx);

    static constexpr int   kCount = 6;
    static constexpr float kBoxSize = 28.0f;
    static constexpr float kGap = 6.0f;
    static constexpr float kLabelH = 18.0f;
    static constexpr float kLabelGap = 5.0f;

    HWND      mHwnd;
    Graphics* mGfx;
    float     mX, mY;
    int       mFocusIdx = -1;
    IntBox* mBoxes[kCount];
};

#endif