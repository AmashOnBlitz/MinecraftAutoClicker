#pragma once
#ifndef INTBOX_CLASS_HEADER
#define INTBOX_CLASS_HEADER

#include "Graphics.hpp"
#include <string>

class IntBox {
public:
    IntBox(HWND hwnd, Graphics* gfx, float x, float y, float size);

    void Render(bool focused);
    bool HitTest(float mx, float my) const;
    void SetDigit(int d);
    int  GetDigit() const { return mDigit; }
    void SetBlank(bool b) { mBlank = b; }
    bool IsBlank()  const { return mBlank; }

private:
    HWND      mHwnd;
    Graphics* mGfx;
    float     mX, mY, mSize;
    int       mDigit = 0;
    bool      mBlank = false;
};

#endif