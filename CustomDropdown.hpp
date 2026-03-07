#pragma once
#ifndef CUSTOM_DROPDOWN_HEADER
#define CUSTOM_DROPDOWN_HEADER

#include <string>
#include <vector>
#include "Graphics.hpp"

class CustomDropdown {
public:
    CustomDropdown(HWND hwnd, Graphics* gfx,
                   float x, float y, float w,
                   float headerH = 28.0f, float itemH = 24.0f);

    void AddItem(const std::wstring& label, int value);
    void SetSelectedIndex(int idx);
    int  GetSelectedIndex() const { return mSelectedIdx; }
    int  GetSelectedValue() const;

    void Render();
    bool OnMouseMove(float mx, float my);
    bool OnMouseDown(float mx, float my);
    bool OnMouseUp(float mx, float my);
    void OnMouseLeave();
    bool OnMouseWheel(float mx, float my, int delta);

    bool  IsOpen()    const { return mOpen; }
    float GetBottom() const;

private:
    bool HitTestHeader(float mx, float my) const;
    int  HitTestItem(float mx, float my) const;
    bool HitTestList(float mx, float my) const;
    void DrawChevron(float cx, float cy, bool pointUp);
    int  MaxVisibleItems() const;
    void ClampScrollOffset();

    struct Item { std::wstring label; int value; };

    HWND      mHwnd;
    Graphics* mGfx;
    float     mX, mY, mW, mHeaderH, mItemH;

    std::vector<Item> mItems;
    int  mSelectedIdx = 0;
    bool mOpen = false;
    int  mHoverItemIdx = -1;
    bool mHeaderHover = false;
    int  mScrollOffset = 0;

    static constexpr int   kMaxVisible = 5;
    static constexpr float kRadius = 6.0f;
    static constexpr float kFontSize = 12.0f;
    static constexpr float kPadX = 10.0f;
    static constexpr float kScrollBarW = 6.0f;
};

#endif