#pragma once
#ifndef SLIDER_CLASS_HEADER
#define SLIDER_CLASS_HEADER

#include <functional>
#include "Graphics.hpp"

class Slider {
public:
    Slider(HWND hwnd, Graphics* gfx,
           float x, float y, float w,
           float minVal, float maxVal, float defaultVal,
           D2D1::ColorF trackColor = D2D1::ColorF(0.78f, 0.78f, 0.80f),
           D2D1::ColorF fillColor = D2D1::ColorF(0.20f, 0.47f, 0.75f));

    void  Render();
    void  OnMouseDown(float mx, float my);
    void  OnMouseMove(float mx, float my);
    void  OnMouseUp(float mx, float my);
    void  OnMouseLeave();

    float GetValue()  const { return mValue; }
    void  SetValue(float v);
    void  SetOnChange(std::function<void(float)> cb) { mOnChange = cb; }

private:
    bool  HitTest(float mx, float my) const;
    float NormalizedValue() const;
    float ValueFromX(float mx) const;
    float ThumbX() const;

    HWND      mHwnd;
    Graphics* mGfx;
    float     mX, mY, mW;
    float     mMinVal, mMaxVal, mValue;
    bool      mDragging = false;
    bool      mHover = false;

    D2D1::ColorF mTrackColor;
    D2D1::ColorF mFillColor;

    std::function<void(float)> mOnChange;

    static constexpr float kTrackH = 4.0f;
    static constexpr float kThumbR = 9.0f;
    static constexpr float kHitPad = 14.0f;
};

#endif