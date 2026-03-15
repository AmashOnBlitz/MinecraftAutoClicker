#include "pch.h"
#include "PidInput.hpp"

PidInput::PidInput(HWND hwnd, Graphics* gfx, float x, float y)
    : mHwnd(hwnd), mGfx(gfx), mX(x), mY(y)
{
    float by = y + kLabelH + kLabelGap;
    for (int i = 0; i < kCount; ++i) {
        mBoxes[i] = new IntBox(hwnd, gfx, x + i * (kBoxSize + kGap), by, kBoxSize);
    }
}

PidInput::~PidInput() {
    for (int i = 0; i < kCount; ++i) {
        delete mBoxes[i];
        mBoxes[i] = nullptr;
    }
}

void PidInput::Render() {
    float totalW = kCount * kBoxSize + (kCount - 1) * kGap;
    mGfx->DrawTextCentered(L"PID", mX, mY, totalW, kLabelH,
                           D2D1::ColorF(0.20f, 0.20f, 0.22f), 12.0f);
    for (int i = 0; i < kCount; ++i) {
        mBoxes[i]->Render(i == mFocusIdx);
    }
}

void PidInput::OnMouseDown(float mx, float my) {
    for (int i = 0; i < kCount; ++i) {
        if (mBoxes[i]->HitTest(mx, my)) {
            MoveFocus(i);
            SetFocus(mHwnd);
            return;
        }
    }
    mFocusIdx = -1;
    InvalidateRect(mHwnd, nullptr, FALSE);
}

void PidInput::OnChar(wchar_t ch) {
    if (mFocusIdx < 0) return;

    if (ch == L'\r') {
        if (mFocusIdx < kCount - 1)
            MoveFocus(mFocusIdx + 1);
        return;
    }

    if (ch == L'\b') {
        mBoxes[mFocusIdx]->SetDigit(0);
        if (mFocusIdx > 0)
            MoveFocus(mFocusIdx - 1);
        return;
    }

    int digit = 0;
    if (ch >= L'0' && ch <= L'9')
        digit = static_cast<int>(ch - L'0');

    mBoxes[mFocusIdx]->SetDigit(digit);

    if (mFocusIdx < kCount - 1)
        MoveFocus(mFocusIdx + 1);
}

void PidInput::OnKeyDown(int vk) {
    if (mFocusIdx < 0) return;

    if (vk == VK_RIGHT && mFocusIdx < kCount - 1)
        MoveFocus(mFocusIdx + 1);
    else if (vk == VK_LEFT && mFocusIdx > 0)
        MoveFocus(mFocusIdx - 1);
    else if (vk == VK_TAB)
        MoveFocus(mFocusIdx < kCount - 1 ? mFocusIdx + 1 : mFocusIdx);
}

void PidInput::MoveFocus(int idx) {
    mFocusIdx = idx;
    InvalidateRect(mHwnd, nullptr, FALSE);
}

int PidInput::GetPid() const {
    int result = 0;
    for (int i = 0; i < kCount; ++i)
        result = result * 10 + mBoxes[i]->GetDigit();
    return result;
}