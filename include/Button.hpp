#pragma once
#ifndef BUTTON_CLASS_HEADER
#define BUTTON_CLASS_HEADER

#include <string>
#include <functional>
#include "Graphics.hpp"
#include <Windows.h>


class Button {
public:
    Button(HWND hwnd, Graphics* gfx,
           const std::wstring& label,
           D2D1::ColorF bgColor,
           D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::White));
    virtual ~Button() = default;
    virtual void render();

    virtual void OnMouseMove(float mx, float my);
    virtual void OnMouseDown(float mx, float my);
    virtual void OnMouseUp(float mx, float my);
    virtual void OnMouseLeave(); 

    void SetOnClick(std::function<void()> cb) { mOnClick = cb; }
    void SetDisabled(bool disabled);

protected:
    virtual bool HitTest(float mx, float my) = 0;

    enum class State { Normal, Hover, Pressed, Disabled };
    State mState = State::Normal;

    HWND mHwnd;
    Graphics* mGfx;
    std::wstring  mLabel;
    D2D1::ColorF  mBgColor;    
    D2D1::ColorF  mTextColor;
    std::function<void()> mOnClick;

    D2D1::ColorF CurrentBgColor() const;
};

class BottomPaddedButton : public Button {
public:
    BottomPaddedButton(HWND hwnd, Graphics* gfx,
                       const std::wstring& label,
                       float padding = 16.0f,
                       float height = 40.0f,
                       D2D1::ColorF bgColor = D2D1::ColorF(0.20f, 0.47f, 0.75f),
                       D2D1::ColorF textColor = D2D1::ColorF(D2D1::ColorF::White));
    void render() override;
    void GetRect(float& outX, float& outY, float& outW, float& outH) const;

protected:
    bool HitTest(float mx, float my) override;

private:
    float mPadding;
    float mHeight;
};

#endif //!BUTTON_CLASS_HEADER