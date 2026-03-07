#pragma once
#ifndef CHECKBOX_CLASS_HEADER
#define CHECKBOX_CLASS_HEADER

#include <string>
#include <functional>
#include "Graphics.hpp"

class Checkbox {
public:
    Checkbox(HWND hwnd, Graphics* gfx,
             const std::wstring& label,
             D2D1::ColorF accentColor = D2D1::ColorF(0.20f, 0.47f, 0.75f),
             D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::Black),
             D2D1::ColorF borderColor = D2D1::ColorF(0.55f, 0.55f, 0.55f));

    virtual ~Checkbox() = default;

    virtual void render() = 0;

    virtual void OnMouseMove(float mx, float my);
    virtual void OnMouseDown(float mx, float my);
    virtual void OnMouseUp(float mx, float my);
    virtual void OnMouseLeave();

    void SetOnChange(std::function<void(bool)> cb) { mOnChange = cb; }

    bool IsChecked()         const { return mChecked; }
    void SetChecked(bool v) { mChecked = v; InvalidateRect(mHwnd, nullptr, FALSE); }

    const std::wstring& GetLabel() const { return mLabel; }

protected:
    virtual bool HitTest(float mx, float my) = 0;
    virtual void GetRect(float& x, float& y, float& w, float& h) const = 0;
    void DrawWidget(float boxX, float boxY, float totalW);

    enum class State { Normal, Hover };
    State mState = State::Normal;
    bool  mChecked = false;

    HWND         mHwnd;
    Graphics* mGfx;
    std::wstring mLabel;
    D2D1::ColorF mAccentColor;   
    D2D1::ColorF mTextColor;
    D2D1::ColorF mBorderColor;  

    static constexpr float kBoxSize = 18.0f; 
    static constexpr float kBoxRadius = 4.0f; 
    static constexpr float kTextGap = 8.0f; 
    static constexpr float kFontSize = 13.0f;
    static constexpr float kCheckStrokeW = 2.0f;

private:
    std::function<void(bool)> mOnChange;
};

class FixedCheckbox : public Checkbox {
public:
    FixedCheckbox(HWND hwnd, Graphics* gfx,
                  const std::wstring& label,
                  float x, float y,
                  float textWidth = 160.0f,
                  D2D1::ColorF accentColor = D2D1::ColorF(0.20f, 0.47f, 0.75f),
                  D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::Black),
                  D2D1::ColorF borderColor = D2D1::ColorF(0.55f, 0.55f, 0.55f));

    void render() override;

    void SetPosition(float x, float y) { mX = x; mY = y; }
    void SetTextWidth(float w) { mTextWidth = w; }

protected:
    bool HitTest(float mx, float my) override;
    void GetRect(float& x, float& y, float& w, float& h) const override;

private:
    float mX, mY, mTextWidth;
};

#endif //!CHECKBOX_CLASS_HEADER