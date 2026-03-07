#pragma once
#ifndef BUTTON_CLASS_HEADER
#define BUTTON_CLASS_HEADER

#include <string>
#include "Graphics.hpp"

class Button {
public:
    Button(HWND hwnd, Graphics* gfx,
           const std::wstring& label,
           D2D1::ColorF bgColor,
           D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::White));
    virtual ~Button() = default;
    virtual void render();

protected:
    HWND          mHwnd;
    Graphics* mGfx;
    std::wstring  mLabel;
    D2D1::ColorF  mBgColor;
    D2D1::ColorF  mTextColor;
};

// Stretches edge-to-edge with equal padding on left, right, and bottom
class BottomPaddedButton : public Button {
public:
    BottomPaddedButton(HWND hwnd, Graphics* gfx,
                       const std::wstring& label,
                       float padding = 16.0f,
                       float height = 40.0f,
                       D2D1::ColorF bgColor = D2D1::ColorF(0.20f, 0.47f, 0.75f),
                       D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::White));
    void render() override;

private:
    float mPadding;
    float mHeight;
};

#endif //!BUTTON_CLASS_HEADER