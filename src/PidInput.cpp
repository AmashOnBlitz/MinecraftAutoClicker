#include "pch.h"
#include "PidInput.hpp"
#include "ProcessPicker.hpp"

PidInput::PidInput(HWND hwnd, Graphics* gfx, float x, float y)
    : mHwnd(hwnd), mGfx(gfx), mX(x), mY(y)
{
    float by = y + kLabelH + kLabelGap;
    for (int i = 0; i < kCount; ++i)
        mBoxes[i] = new IntBox(hwnd, gfx,
                               x + i * (kBoxSize + kGap), by, kBoxSize);
}

PidInput::~PidInput() {
    for (int i = 0; i < kCount; ++i) { delete mBoxes[i]; mBoxes[i] = nullptr; }
}

void PidInput::BrowseGetRect(float& bx, float& by) const
{
    float totalW = kCount * kBoxSize + (kCount - 1) * kGap;
    bx = mX + totalW + kBrowseGap;
    by = mY + kLabelH + kLabelGap + (kBoxSize - kBrowseSize) * 0.5f;
}

bool PidInput::BrowseHitTest(float mx, float my) const
{
    float bx, by;
    BrowseGetRect(bx, by);
    return mx >= bx && mx <= bx + kBrowseSize &&
        my >= by && my <= by + kBrowseSize;
}

void PidInput::Render()
{
    float totalW = kCount * kBoxSize + (kCount - 1) * kGap;
    mGfx->DrawTextCentered(L"PID", mX, mY, totalW, kLabelH,
                           D2D1::ColorF(0.20f, 0.20f, 0.22f), 12.0f);

    for (int i = 0; i < kCount; ++i)
        mBoxes[i]->Render(i == mFocusIdx);

    float bx, by;
    BrowseGetRect(bx, by);

    D2D1::ColorF btnBg = mBrowseHover
        ? D2D1::ColorF(0.88f, 0.92f, 0.99f)
        : D2D1::ColorF(0.95f, 0.95f, 0.97f);
    D2D1::ColorF btnBorder = mBrowseHover
        ? D2D1::ColorF(0.20f, 0.47f, 0.75f)
        : D2D1::ColorF(0.70f, 0.70f, 0.72f);
    float btnStroke = mBrowseHover ? 2.0f : 1.5f;

    mGfx->SetAliased(false);
    mGfx->FillRoundedRect(bx, by, kBrowseSize, kBrowseSize, 4.0f, btnBg);
    mGfx->DrawRoundedRect(bx, by, kBrowseSize, kBrowseSize, 4.0f, btnBorder, btnStroke);
    mGfx->SetAliased(true);

    mGfx->DrawTextCentered(L"\u2026",
                           bx, by, kBrowseSize, kBrowseSize,
                           D2D1::ColorF(0.30f, 0.30f, 0.35f), 11.0f);
}

void PidInput::OnMouseMove(float mx, float my)
{
    bool wasHover = mBrowseHover;
    mBrowseHover = BrowseHitTest(mx, my);
    if (mBrowseHover != wasHover)
        InvalidateRect(mHwnd, nullptr, FALSE);
}

void PidInput::OnMouseLeave()
{
    if (mBrowseHover) {
        mBrowseHover = false;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
}

void PidInput::OnMouseDown(float mx, float my)
{
    if (BrowseHitTest(mx, my)) {
        OpenBrowse();
        return;
    }

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

void PidInput::OnChar(wchar_t ch)
{
    if (mFocusIdx < 0) return;

    if (ch == L'\r') {
        if (mFocusIdx < kCount - 1) MoveFocus(mFocusIdx + 1);
        return;
    }
    if (ch == L'\b') {
        mBoxes[mFocusIdx]->SetDigit(0);
        mBoxes[mFocusIdx]->SetBlank(false);
        if (mFocusIdx > 0) MoveFocus(mFocusIdx - 1);
        return;
    }
    if (ch >= L'0' && ch <= L'9') {
        for (int i = 0; i < kCount; ++i) mBoxes[i]->SetBlank(false);
        mBoxes[mFocusIdx]->SetDigit(static_cast<int>(ch - L'0'));
        if (mFocusIdx < kCount - 1) MoveFocus(mFocusIdx + 1);
    }
}

void PidInput::OnKeyDown(int vk)
{
    if (mFocusIdx < 0) return;
    if (vk == VK_RIGHT && mFocusIdx < kCount - 1) MoveFocus(mFocusIdx + 1);
    else if (vk == VK_LEFT && mFocusIdx > 0)           MoveFocus(mFocusIdx - 1);
    else if (vk == VK_TAB)
        MoveFocus(mFocusIdx < kCount - 1 ? mFocusIdx + 1 : mFocusIdx);
}

void PidInput::SetPid(DWORD pid)
{
    int digits[kCount]{};
    DWORD tmp = pid;
    for (int i = kCount - 1; i >= 0; --i) {
        digits[i] = static_cast<int>(tmp % 10);
        tmp /= 10;
    }

    int firstNZ = 0;
    while (firstNZ < kCount - 1 && digits[firstNZ] == 0) ++firstNZ;

    for (int i = 0; i < kCount; ++i) {
        mBoxes[i]->SetDigit(digits[i]);
        mBoxes[i]->SetBlank(i < firstNZ);
    }

    mFocusIdx = -1;
    InvalidateRect(mHwnd, nullptr, FALSE);
}

int PidInput::GetPid() const {
    int result = 0;
    for (int i = 0; i < kCount; ++i)
        result = result * 10 + mBoxes[i]->GetDigit();
    return result;
}

void PidInput::MoveFocus(int idx) {
    mFocusIdx = idx;
    InvalidateRect(mHwnd, nullptr, FALSE);
}

void PidInput::OpenBrowse()
{
    DWORD pid = ShowProcessPicker(mHwnd);
    if (pid != 0)
        SetPid(pid);
}