#include "pch.h"
#define NOMINMAX
#include "Checkbox.hpp"
#include <algorithm>

Checkbox::Checkbox(HWND hwnd, Graphics* gfx, const std::wstring& label,
                   D2D1::ColorF accentColor, D2D1::ColorF textColor, D2D1::ColorF borderColor)
    : mHwnd(hwnd), mGfx(gfx),
    mLabel(label),
    mAccentColor(accentColor),
    mTextColor(textColor),
    mBorderColor(borderColor)
{
}

void Checkbox::OnMouseMove(float mx, float my) {
    if (HitTest(mx, my)) {
        if (mState == State::Normal) {
            mState = State::Hover;
            InvalidateRect(mHwnd, nullptr, FALSE);
        }
    }
    else {
        if (mState == State::Hover) {
            mState = State::Normal;
            InvalidateRect(mHwnd, nullptr, FALSE);
        }
    }
}

void Checkbox::OnMouseDown(float /*mx*/, float /*my*/) {
}

void Checkbox::OnMouseUp(float mx, float my) {
    if (HitTest(mx, my)) {
        mChecked = !mChecked;
        InvalidateRect(mHwnd, nullptr, FALSE);
        if (mOnChange) mOnChange(mChecked);
    }
}

void Checkbox::OnMouseLeave() {
    if (mState != State::Normal) {
        mState = State::Normal;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
}

void Checkbox::DrawWidget(float boxX, float boxY, float totalW) {

    mGfx->SetAliased(false); 
    D2D1::ColorF bgColor = D2D1::ColorF::Blue;
    if (mChecked) {
        bgColor = mAccentColor;                                
    }
    else if (mState == State::Hover) {
        bgColor = D2D1::ColorF(0.88f, 0.92f, 0.97f);          
    }
    else {
        bgColor = D2D1::ColorF(1.0f, 1.0f, 1.0f);          
    }
    mGfx->FillRoundedRect(boxX, boxY, kBoxSize, kBoxSize, kBoxRadius, bgColor);

    D2D1::ColorF borderCol = mChecked
        ? mAccentColor                                         
        : (mState == State::Hover
           ? D2D1::ColorF(mAccentColor.r, mAccentColor.g, mAccentColor.b, 0.80f)
           : mBorderColor);
    mGfx->DrawRoundedRect(boxX, boxY, kBoxSize, kBoxSize, kBoxRadius, borderCol, 1.5f);

    if (mChecked) {
        float s1x = boxX + kBoxSize * 0.20f;
        float s1y = boxY + kBoxSize * 0.52f;
        float knx = boxX + kBoxSize * 0.42f;
        float kny = boxY + kBoxSize * 0.76f;
        float s2x = boxX + kBoxSize * 0.80f;
        float s2y = boxY + kBoxSize * 0.24f;

        mGfx->DrawLine(s1x, s1y, knx, kny, D2D1::ColorF(D2D1::ColorF::White), kCheckStrokeW);
        mGfx->DrawLine(knx, kny, s2x, s2y, D2D1::ColorF(D2D1::ColorF::White), kCheckStrokeW);
    }

    mGfx->SetAliased(true); 

    float textX = boxX + kBoxSize + kTextGap;
    float textW = totalW - kBoxSize - kTextGap;
    mGfx->DrawTextLeft(mLabel, textX, boxY, textW, kBoxSize, mTextColor, kFontSize);
}



FixedCheckbox::FixedCheckbox(HWND hwnd, Graphics* gfx, const std::wstring& label,
                             float x, float y, float textWidth,
                             D2D1::ColorF accentColor, D2D1::ColorF textColor, D2D1::ColorF borderColor)
    : Checkbox(hwnd, gfx, label, accentColor, textColor, borderColor),
    mX(x), mY(y), mTextWidth(textWidth)
{
}

void FixedCheckbox::GetRect(float& x, float& y, float& w, float& h) const {
    x = mX;
    y = mY;
    w = kBoxSize + kTextGap + mTextWidth;
    h = kBoxSize;
}

bool FixedCheckbox::HitTest(float mx, float my) {
    float x, y, w, h;
    GetRect(x, y, w, h);
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

void FixedCheckbox::render() {
    DrawWidget(mX, mY, kBoxSize + kTextGap + mTextWidth);
}