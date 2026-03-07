#define NOMINMAX
#include "Button.hpp"
#include <algorithm>


static D2D1::ColorF Lighten(D2D1::ColorF c, float amt) {
    return D2D1::ColorF(
        std::min(c.r + amt, 1.0f),
        std::min(c.g + amt, 1.0f),
        std::min(c.b + amt, 1.0f),
        c.a
    );
}
static D2D1::ColorF Darken(D2D1::ColorF c, float amt) {
    return D2D1::ColorF(
        std::max(c.r - amt, 0.0f),
        std::max(c.g - amt, 0.0f),
        std::max(c.b - amt, 0.0f),
        c.a
    );
}


Button::Button(HWND hwnd, Graphics* gfx,
               const std::wstring& label,
               D2D1::ColorF bgColor,
               D2D1::ColorF textColor)
    : mHwnd(hwnd), mGfx(gfx),
    mLabel(label), mBgColor(bgColor), mTextColor(textColor)
{
}

D2D1::ColorF Button::CurrentBgColor() const {
    switch (mState) {
    case State::Hover:      return Lighten(mBgColor, 0.10f);
    case State::Pressed:    return Darken(mBgColor, 0.12f);
    case State::Disabled:   return Darken(mBgColor, 0.12f);
    default:                return mBgColor;
    }
}

void Button::render() {}

void Button::OnMouseMove(float mx, float my) {
    if (HitTest(mx, my) && mState != State::Disabled) {
        if (mState == State::Normal) {
            mState = State::Hover;
            InvalidateRect(mHwnd, nullptr, FALSE);  
        }
    }
    else {
        if (mState == State::Hover && mState != State::Disabled) {
            mState = State::Normal;
            InvalidateRect(mHwnd, nullptr, FALSE);
        }
    }
}

void Button::OnMouseDown(float mx, float my) {
    if (HitTest(mx, my) && mState != State::Disabled) {
        mState = State::Pressed;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
}

void Button::OnMouseUp(float mx, float my) {
    if (mState == State::Pressed && mState != State::Disabled) {
        mState = HitTest(mx, my) ? State::Hover : State::Normal;
        InvalidateRect(mHwnd, nullptr, FALSE);
        if (mOnClick && HitTest(mx, my)) mOnClick();
    }
}

void Button::OnMouseLeave() {
    if (mState != State::Normal && mState != State::Disabled) {
        mState = State::Normal;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
}

void Button::SetDisabled(bool disabled)
{
    if (disabled && mState != State::Disabled) {
        mState = State::Disabled;
        InvalidateRect(mHwnd, nullptr, TRUE);
    }
}


BottomPaddedButton::BottomPaddedButton(HWND hwnd, Graphics* gfx,
                                       const std::wstring& label,
                                       float padding, float height,
                                       D2D1::ColorF bgColor,
                                       D2D1::ColorF textColor)
    : Button(hwnd, gfx, label, bgColor, textColor),
    mPadding(padding), mHeight(height)
{
}

void BottomPaddedButton::GetRect(float& x, float& y, float& w, float& h) const {
    RECT rc;
    GetClientRect(mHwnd, &rc);
    float winW = static_cast<float>(rc.right - rc.left);
    float winH = static_cast<float>(rc.bottom - rc.top);
    x = mPadding;
    y = winH - mHeight - mPadding;
    w = winW - 2.0f * mPadding;
    h = mHeight;
}

bool BottomPaddedButton::HitTest(float mx, float my) {
    float x, y, w, h;
    GetRect(x, y, w, h);
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

void BottomPaddedButton::render() {
    float x, y, w, h;
    GetRect(x, y, w, h);
    mGfx->SetAliased(false); // Dont remove -- this is for rounded corners
    mGfx->FillRoundedRect(x, y, w, h, 8.0f, CurrentBgColor());
    mGfx->DrawTextCentered(mLabel, x, y, w, h, mTextColor);
    mGfx->SetAliased(true); // restore anti alias
}