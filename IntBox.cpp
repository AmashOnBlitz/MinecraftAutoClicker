#include "IntBox.hpp"
#include <string>

IntBox::IntBox(HWND hwnd, Graphics* gfx, float x, float y, float size)
    : mHwnd(hwnd), mGfx(gfx), mX(x), mY(y), mSize(size)
{
}

void IntBox::Render(bool focused) {
    D2D1::ColorF bg = focused
        ? D2D1::ColorF(0.87f, 0.93f, 1.0f)
        : D2D1::ColorF(1.0f, 1.0f, 1.0f);
    D2D1::ColorF border = focused
        ? D2D1::ColorF(0.20f, 0.47f, 0.75f)
        : D2D1::ColorF(0.65f, 0.65f, 0.65f);
    float strokeW = focused ? 2.0f : 1.5f;

    mGfx->SetAliased(false);
    mGfx->FillRoundedRect(mX, mY, mSize, mSize, 5.0f, bg);
    mGfx->DrawRoundedRect(mX, mY, mSize, mSize, 5.0f, border, strokeW);
    mGfx->SetAliased(true);

    wchar_t buf[2] = { (wchar_t)(L'0' + mDigit), L'\0' };
    mGfx->DrawTextCentered(std::wstring(buf), mX, mY, mSize, mSize,
                           D2D1::ColorF(0.10f, 0.10f, 0.10f), 14.0f);
}

bool IntBox::HitTest(float mx, float my) const {
    return mx >= mX && mx <= mX + mSize && my >= mY && my <= mY + mSize;
}

int IntBox::GetDigit() const {
    return mDigit;
}

void IntBox::SetDigit(int d) {
    mDigit = d;
    InvalidateRect(mHwnd, nullptr, FALSE);
}