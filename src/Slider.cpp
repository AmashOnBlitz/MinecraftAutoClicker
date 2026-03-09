#include "pch.h"
#include "Slider.hpp"
#include <algorithm>
#include <cmath>

Slider::Slider(HWND hwnd, Graphics* gfx,
               float x, float y, float w,
               float minVal, float maxVal, float defaultVal,
               D2D1::ColorF trackColor,
               D2D1::ColorF fillColor)
    : mHwnd(hwnd), mGfx(gfx),
    mX(x), mY(y), mW(w),
    mMinVal(minVal), mMaxVal(maxVal),
    mValue(std::clamp(defaultVal, minVal, maxVal)),
    mTrackColor(trackColor), mFillColor(fillColor)
{
}

float Slider::NormalizedValue() const {
    if (mMaxVal == mMinVal) return 0.0f;
    return (mValue - mMinVal) / (mMaxVal - mMinVal);
}

float Slider::ThumbX() const {
    return mX + NormalizedValue() * mW;
}

float Slider::ValueFromX(float mx) const {
    float t = std::clamp((mx - mX) / mW, 0.0f, 1.0f);
    return mMinVal + t * (mMaxVal - mMinVal);
}

bool Slider::HitTest(float mx, float my) const {
    float dx = mx - ThumbX();
    float dy = my - mY;
    return (dx * dx + dy * dy) <= (kHitPad * kHitPad);
}

void Slider::SetValue(float v) {
    v = std::clamp(v, mMinVal, mMaxVal);
    if (v == mValue) return;
    mValue = v;
    InvalidateRect(mHwnd, nullptr, FALSE);
    if (mOnChange) mOnChange(mValue);
}

void Slider::Render() {
    float tx = ThumbX();

    mGfx->SetAliased(false);

    mGfx->FillRoundedRect(mX, mY - kTrackH * 0.5f, mW, kTrackH, kTrackH * 0.5f, mTrackColor);

    float fillW = tx - mX;
    if (fillW > 0.0f)
        mGfx->FillRoundedRect(mX, mY - kTrackH * 0.5f, fillW, kTrackH, kTrackH * 0.5f, mFillColor);

    D2D1::ColorF thumbFill = (mDragging || mHover)
        ? D2D1::ColorF(mFillColor.r * 0.80f, mFillColor.g * 0.80f, mFillColor.b * 0.80f, 1.0f)
        : mFillColor;

    mGfx->FillCircle(tx, mY, kThumbR, thumbFill);
    mGfx->drawCircle(tx, mY, kThumbR, kThumbR, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.40f), 1.5f);

    mGfx->SetAliased(true);
}

void Slider::OnMouseDown(float mx, float my) {
    if (mx >= mX - kThumbR && mx <= mX + mW + kThumbR &&
        my >= mY - kHitPad && my <= mY + kHitPad)
    {
        mDragging = true;
        SetCapture(mHwnd);
        SetValue(ValueFromX(mx));
    }
}

void Slider::OnMouseMove(float mx, float my) {
    if (mDragging) {
        SetValue(ValueFromX(mx));
        return;
    }
    bool prev = mHover;
    mHover = HitTest(mx, my);
    if (mHover != prev) InvalidateRect(mHwnd, nullptr, FALSE);
}

void Slider::OnMouseUp(float mx, float my) {
    if (!mDragging) return;
    mDragging = false;
    ReleaseCapture();
    SetValue(ValueFromX(mx));
}

void Slider::OnMouseLeave() {
    if (!mHover) return;
    mHover = false;
    InvalidateRect(mHwnd, nullptr, FALSE);
}