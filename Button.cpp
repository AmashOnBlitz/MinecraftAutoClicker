#include "Button.hpp"


Button::Button(HWND hwnd, Graphics* gfx,
               const std::wstring& label,
               D2D1::ColorF bgColor,
               D2D1::ColorF textColor)
    : mHwnd(hwnd), mGfx(gfx),
    mLabel(label), mBgColor(bgColor), mTextColor(textColor)
{
}

void Button::render() {}  

BottomPaddedButton::BottomPaddedButton(HWND hwnd, Graphics* gfx,
                                       const std::wstring& label,
                                       float padding, float height,
                                       D2D1::ColorF bgColor,
                                       D2D1::ColorF textColor)
    : Button(hwnd, gfx, label, bgColor, textColor),
    mPadding(padding), mHeight(height)
{
}

void BottomPaddedButton::render()
{
    RECT rc;
    GetClientRect(mHwnd, &rc);
    float winW = static_cast<float>(rc.right - rc.left);
    float winH = static_cast<float>(rc.bottom - rc.top);

    float x = mPadding;
    float y = winH - mHeight - mPadding;   
    float w = winW - 2.0f * mPadding;     

    mGfx->FillRoundedRect(x, y, w, mHeight, 8.0f, mBgColor);
    mGfx->DrawTextCentered(mLabel, x, y, w, mHeight, mTextColor);
}