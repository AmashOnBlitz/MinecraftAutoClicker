#pragma once
#ifndef KNOB_CLASS_HEADER
#define KNOB_CLASS_HEADER

#include <string>
#include <functional>
#include "Graphics.hpp"

class Knob {
public:
    Knob(HWND hwnd, Graphics* gfx,
         const std::wstring& label,
         float minVal, float maxVal, float defaultVal,
         D2D1::ColorF trackColor = D2D1::ColorF(0.70f, 0.70f, 0.72f),
         D2D1::ColorF fillColor = D2D1::ColorF(0.20f, 0.47f, 0.75f),
         D2D1::ColorF needleColor = D2D1::ColorF(D2D1::ColorF::White),
         D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::Black));

    virtual ~Knob() = default;

    virtual void render() = 0;

    virtual void OnMouseMove(float mx, float my);
    virtual void OnMouseDown(float mx, float my);
    virtual void OnMouseUp(float mx, float my);
    virtual void OnMouseLeave();

    void  SetOnChange(std::function<void(float)> cb) { mOnChange = cb; }
    float GetValue()  const { return mValue; }
    void  SetValue(float v);

protected:
    virtual bool  HitTest(float mx, float my) = 0;
    virtual void  GetCenter(float& cx, float& cy) const = 0;
    virtual float GetRadius() const = 0;

    void DrawWidget(float cx, float cy, float radius);

    float NormalizedValue()     const;
    float AngleForNorm(float t) const;

    HWND          mHwnd;
    Graphics* mGfx;
    std::wstring  mLabel;
    float         mMinVal, mMaxVal, mValue;

    D2D1::ColorF  mTrackColor;
    D2D1::ColorF  mFillColor;
    D2D1::ColorF  mNeedleColor;
    D2D1::ColorF  mTextColor;

    bool  mDragging = false;
    float mDragStartY = 0.0f;
    float mDragStartVal = 0.0f;
    float mAccumAngle = 0.0f;
    float mLastAngle = 0.0f;

    static constexpr float kStartAngleDeg = 120.0f;
    static constexpr float kSweepDeg = 270.0f;

private:
    std::function<void(float)> mOnChange;
};


class FixedKnob : public Knob {
public:
    FixedKnob(HWND hwnd, Graphics* gfx,
              const std::wstring& label,
              float cx, float cy, float radius,
              float minVal, float maxVal, float defaultVal,
              D2D1::ColorF trackColor = D2D1::ColorF(0.70f, 0.70f, 0.72f),
              D2D1::ColorF fillColor = D2D1::ColorF(0.20f, 0.47f, 0.75f),
              D2D1::ColorF needleColor = D2D1::ColorF(D2D1::ColorF::White),
              D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::Black));

    void render() override;

    void SetCenter(float cx, float cy) { mCx = cx; mCy = cy; }

protected:
    bool  HitTest(float mx, float my) override;
    void  GetCenter(float& cx, float& cy) const override;
    float GetRadius() const override { return mRadius; }

private:
    float mCx, mCy, mRadius;
};


class CpsKnob : public FixedKnob {
public:
    CpsKnob(HWND hwnd, Graphics* gfx,
            float cx, float cy,
            float radius = 36.0f);

    int GetCps() const { return static_cast<int>(GetValue() + 0.5f); }
};


class CooldownKnob : public FixedKnob {
public:
    CooldownKnob(HWND hwnd, Graphics* gfx,
                 float cx, float cy,
                 float radius = 36.0f);

    float GetCooldown() const { return GetValue(); }
};


class TriggerCooldownKnob : public FixedKnob {
public:
    TriggerCooldownKnob(HWND hwnd, Graphics* gfx,
                        float cx, float cy,
                        float radius = 36.0f);

    float GetTriggerCooldown() const { return GetValue(); }
};


#endif //!KNOB_CLASS_HEADER