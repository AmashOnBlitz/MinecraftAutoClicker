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
    int  GetDigit() const;
    void SetDigit(int d);

private:
    HWND      mHwnd;
    Graphics* mGfx;
    float     mX, mY, mSize;
    int       mDigit = 0;
};

#endif