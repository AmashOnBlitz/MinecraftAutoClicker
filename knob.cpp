#define NOMINMAX
#include "Knob.hpp"
#include <algorithm>
#include <cmath>
#include <cwchar>

static constexpr float kPi = 3.14159265358979323846f;

static float WrapAngle(float a)
{
    while (a > kPi) a -= 2.0f * kPi;
    while (a <= -kPi) a += 2.0f * kPi;
    return a;
}

Knob::Knob(HWND hwnd, Graphics* gfx,
           const std::wstring& label,
           float minVal, float maxVal, float defaultVal,
           D2D1::ColorF trackColor,
           D2D1::ColorF fillColor,
           D2D1::ColorF needleColor,
           D2D1::ColorF textColor)
    : mHwnd(hwnd), mGfx(gfx),
    mLabel(label),
    mMinVal(minVal), mMaxVal(maxVal),
    mValue(std::clamp(defaultVal, minVal, maxVal)),
    mTrackColor(trackColor),
    mFillColor(fillColor),
    mNeedleColor(needleColor),
    mTextColor(textColor)
{
}

void Knob::SetValue(float v)
{
    mValue = std::clamp(v, mMinVal, mMaxVal);
    InvalidateRect(mHwnd, nullptr, FALSE);
    if (mOnChange) mOnChange(mValue);
}

float Knob::NormalizedValue() const
{
    if (mMaxVal == mMinVal) return 0.0f;
    return (mValue - mMinVal) / (mMaxVal - mMinVal);
}

float Knob::AngleForNorm(float t) const
{
    float deg = kStartAngleDeg + t * kSweepDeg;
    return deg * kPi / 180.0f;
}

void Knob::DrawWidget(float cx, float cy, float radius)
{
    const float trackW = radius * 0.18f;
    const float arcR = radius - trackW * 0.5f;
    const float innerR = radius - trackW;
    const float needleR = innerR * 0.70f;

    mGfx->SetAliased(false);

    mGfx->DrawArc(cx, cy, arcR,
                  kStartAngleDeg, kSweepDeg,
                  mTrackColor, trackW);

    float filledSweep = NormalizedValue() * kSweepDeg;
    if (filledSweep > 0.5f)
        mGfx->DrawArc(cx, cy, arcR,
                      kStartAngleDeg, filledSweep,
                      mFillColor, trackW);

    mGfx->FillCircle(cx, cy, innerR,
                     D2D1::ColorF(0.22f, 0.22f, 0.24f));

    mGfx->drawCircle(cx, cy, innerR, innerR,
                     D2D1::ColorF(0.38f, 0.38f, 0.40f), 1.5f);

    float angle = AngleForNorm(NormalizedValue());
    float nx = cx + needleR * std::cosf(angle);
    float ny = cy + needleR * std::sinf(angle);
    mGfx->DrawLine(cx, cy, nx, ny, mNeedleColor, 2.0f);

    mGfx->SetAliased(true);

    float labelW = radius * 4.0f;
    float labelX = cx - labelW * 0.5f;
    float labelY = cy + radius + 4.0f;
    mGfx->DrawTextCentered(mLabel, labelX, labelY, labelW, 15.0f, mTextColor, 11.0f);

    wchar_t buf[16];
    swprintf_s(buf, L"%.1f", mValue);
    mGfx->DrawTextCentered(std::wstring(buf),
                           labelX, labelY + 15.0f,
                           labelW, 15.0f,
                           D2D1::ColorF(0.45f, 0.45f, 0.45f), 11.0f);
}

void Knob::OnMouseDown(float mx, float my)
{
    if (!HitTest(mx, my)) return;

    mDragging = true;
    mDragStartY = my;
    mDragStartVal = mValue;

    float cx, cy;
    GetCenter(cx, cy);
    mLastAngle = std::atan2f(my - cy, mx - cx);
    mAccumAngle = 0.0f;

    SetCapture(mHwnd);
}

void Knob::OnMouseMove(float mx, float my)
{
    if (!mDragging) return;

    float range = mMaxVal - mMinVal;

    float verticalDelta = (mDragStartY - my) / 150.0f * range;

    float cx, cy;
    GetCenter(cx, cy);

    float currentAngle = std::atan2f(my - cy, mx - cx);
    float dAngle = WrapAngle(currentAngle - mLastAngle);
    mAccumAngle += dAngle;
    mLastAngle = currentAngle;

    float sweepRad = kSweepDeg * kPi / 180.0f;
    float circularDelta = (mAccumAngle / sweepRad) * range;

    float delta = (std::fabsf(circularDelta) > std::fabsf(verticalDelta))
        ? circularDelta
        : verticalDelta;

    SetValue(mDragStartVal + delta);
}

void Knob::OnMouseUp(float /*mx*/, float /*my*/)
{
    if (mDragging) {
        mDragging = false;
        ReleaseCapture();
    }
}

void Knob::OnMouseLeave()
{
}

FixedKnob::FixedKnob(HWND hwnd, Graphics* gfx,
                     const std::wstring& label,
                     float cx, float cy, float radius,
                     float minVal, float maxVal, float defaultVal,
                     D2D1::ColorF trackColor,
                     D2D1::ColorF fillColor,
                     D2D1::ColorF needleColor,
                     D2D1::ColorF textColor)
    : Knob(hwnd, gfx, label, minVal, maxVal, defaultVal,
           trackColor, fillColor, needleColor, textColor),
    mCx(cx), mCy(cy), mRadius(radius)
{
}

void FixedKnob::GetCenter(float& cx, float& cy) const
{
    cx = mCx;
    cy = mCy;
}

bool FixedKnob::HitTest(float mx, float my)
{
    float dx = mx - mCx;
    float dy = my - mCy;
    return (dx * dx + dy * dy) <= (mRadius * mRadius);
}

void FixedKnob::render()
{
    DrawWidget(mCx, mCy, mRadius);
}


CpsKnob::CpsKnob(HWND hwnd, Graphics* gfx,
                 float cx, float cy, float radius)
    : FixedKnob(hwnd, gfx,
                L"CPS",
                cx, cy, radius,
                10.0f, 20.0f, 10.0f)
{
}


CooldownKnob::CooldownKnob(HWND hwnd, Graphics* gfx,
                           float cx, float cy, float radius)
    : FixedKnob(hwnd, gfx,
                L"Cooldown Time",
                cx, cy, radius,
                0.5f, 3.0f, 1.0f,
                D2D1::ColorF(0.70f, 0.70f, 0.72f),
                D2D1::ColorF(0.80f, 0.50f, 0.10f),          
                D2D1::ColorF(D2D1::ColorF::White),           
                D2D1::ColorF(D2D1::ColorF::Black))           
{
}


TriggerCooldownKnob::TriggerCooldownKnob(HWND hwnd, Graphics* gfx,
                                         float cx, float cy, float radius)
    : FixedKnob(hwnd, gfx,
                L"Cooldown After",
                cx, cy, radius,
                4.0f, 10.0f, 4.0f,
                D2D1::ColorF(0.70f, 0.70f, 0.72f),
                D2D1::ColorF(0.12f, 0.60f, 0.55f),          
                D2D1::ColorF(D2D1::ColorF::White),           
                D2D1::ColorF(D2D1::ColorF::Black))           
{
}