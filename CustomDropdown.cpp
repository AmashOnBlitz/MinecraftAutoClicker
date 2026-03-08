#define NOMINMAX
#include "CustomDropdown.hpp"
#include <algorithm>

CustomDropdown::CustomDropdown(HWND hwnd, Graphics* gfx,
                               float x, float y, float w,
                               float headerH, float itemH,
                               int maxVisible)
    : mHwnd(hwnd), mGfx(gfx),
    mX(x), mY(y), mW(w),
    mHeaderH(headerH), mItemH(itemH),
    mMaxVisible(maxVisible)
{
}

void CustomDropdown::AddItem(const std::wstring& label, int value) {
    mItems.push_back({ label, value });
}

void CustomDropdown::SetSelectedIndex(int idx) {
    if (idx >= 0 && idx < (int)mItems.size()) {
        mSelectedIdx = idx;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
}

int CustomDropdown::GetSelectedValue() const {
    if (mSelectedIdx < 0 || mSelectedIdx >= (int)mItems.size()) return 0;
    return mItems[mSelectedIdx].value;
}

int CustomDropdown::MaxVisibleItems() const {
    return std::min(mMaxVisible, (int)mItems.size());
}

void CustomDropdown::ClampScrollOffset() {
    int maxOffset = std::max(0, (int)mItems.size() - mMaxVisible);
    mScrollOffset = std::clamp(mScrollOffset, 0, maxOffset);
}

float CustomDropdown::GetBottom() const {
    float base = mY + mHeaderH;
    if (mOpen) base += 2.0f + MaxVisibleItems() * mItemH;
    return base;
}

bool CustomDropdown::HitTestHeader(float mx, float my) const {
    return mx >= mX && mx <= mX + mW &&
        my >= mY && my <= mY + mHeaderH;
}

bool CustomDropdown::HitTestList(float mx, float my) const {
    if (!mOpen) return false;
    float listY = mY + mHeaderH + 2.0f;
    float listH = MaxVisibleItems() * mItemH;
    return mx >= mX && mx <= mX + mW && my >= listY && my <= listY + listH;
}

int CustomDropdown::HitTestItem(float mx, float my) const {
    if (!mOpen) return -1;
    float listY = mY + mHeaderH + 2.0f;
    if (mx < mX || mx > mX + mW) return -1;
    int visible = MaxVisibleItems();
    for (int i = 0; i < visible; ++i) {
        float iy = listY + i * mItemH;
        if (my >= iy && my < iy + mItemH) return i + mScrollOffset;
    }
    return -1;
}

void CustomDropdown::DrawChevron(float cx, float cy, bool pointUp) {
    D2D1::ColorF col(0.45f, 0.45f, 0.50f);
    float sz = 4.0f;
    if (pointUp) {
        mGfx->DrawLine(cx - sz, cy + sz * 0.5f, cx, cy - sz * 0.5f, col, 1.5f);
        mGfx->DrawLine(cx, cy - sz * 0.5f, cx + sz, cy + sz * 0.5f, col, 1.5f);
    }
    else {
        mGfx->DrawLine(cx - sz, cy - sz * 0.5f, cx, cy + sz * 0.5f, col, 1.5f);
        mGfx->DrawLine(cx, cy + sz * 0.5f, cx + sz, cy - sz * 0.5f, col, 1.5f);
    }
}

void CustomDropdown::Render() {
    mGfx->SetAliased(false);

    D2D1::ColorF borderCol = (mOpen || mHeaderHover)
        ? D2D1::ColorF(0.20f, 0.47f, 0.75f)
        : D2D1::ColorF(0.70f, 0.70f, 0.72f);
    float strokeW = (mOpen || mHeaderHover) ? 2.0f : 1.5f;

    D2D1::ColorF headerBg = (mHeaderHover && !mOpen)
        ? D2D1::ColorF(0.95f, 0.97f, 1.00f)
        : D2D1::ColorF(1.0f, 1.0f, 1.0f);

    mGfx->FillRoundedRect(mX, mY, mW, mHeaderH, kRadius, headerBg);
    mGfx->DrawRoundedRect(mX, mY, mW, mHeaderH, kRadius, borderCol, strokeW);

    if (!mItems.empty() && mSelectedIdx >= 0 && mSelectedIdx < (int)mItems.size()) {
        mGfx->DrawTextLeft(mItems[mSelectedIdx].label,
                           mX + kPadX, mY,
                           mW - kPadX - 20.0f, mHeaderH,
                           D2D1::ColorF(0.12f, 0.12f, 0.14f), kFontSize);
    }

    DrawChevron(mX + mW - 14.0f, mY + mHeaderH * 0.5f, mOpen);

    if (mOpen) {
        int visible = MaxVisibleItems();
        int total = (int)mItems.size();
        bool needBar = total > mMaxVisible;

        float listY = mY + mHeaderH + 2.0f;
        float listH = visible * mItemH;
        float itemDrawW = needBar ? mW - kScrollBarW - 2.0f : mW;

        mGfx->FillRoundedRect(mX, listY, mW, listH, kRadius,
                              D2D1::ColorF(0.99f, 0.99f, 1.00f));
        mGfx->DrawRoundedRect(mX, listY, mW, listH, kRadius,
                              D2D1::ColorF(0.20f, 0.47f, 0.75f), 1.5f);

        for (int vi = 0; vi < visible; ++vi) {
            int i = vi + mScrollOffset;
            float iy = listY + vi * mItemH;

            if (i == mSelectedIdx) {
                mGfx->FillRoundedRect(mX + 3.0f, iy + 2.0f, itemDrawW - 6.0f, mItemH - 4.0f,
                                      4.0f, D2D1::ColorF(0.20f, 0.47f, 0.75f));
                mGfx->DrawTextLeft(mItems[i].label,
                                   mX + kPadX, iy, itemDrawW - kPadX * 2.0f, mItemH,
                                   D2D1::ColorF(1.0f, 1.0f, 1.0f), kFontSize);
            }
            else if (i == mHoverItemIdx) {
                mGfx->FillRoundedRect(mX + 3.0f, iy + 2.0f, itemDrawW - 6.0f, mItemH - 4.0f,
                                      4.0f, D2D1::ColorF(0.88f, 0.92f, 0.99f));
                mGfx->DrawTextLeft(mItems[i].label,
                                   mX + kPadX, iy, itemDrawW - kPadX * 2.0f, mItemH,
                                   D2D1::ColorF(0.12f, 0.12f, 0.14f), kFontSize);
            }
            else {
                mGfx->DrawTextLeft(mItems[i].label,
                                   mX + kPadX, iy, itemDrawW - kPadX * 2.0f, mItemH,
                                   D2D1::ColorF(0.30f, 0.30f, 0.32f), kFontSize);
            }
        }

        if (needBar) {
            float barX = mX + mW - kScrollBarW - 2.0f;
            float trackH = listH - 8.0f;
            float trackTop = listY + 4.0f;

            mGfx->FillRoundedRect(barX, trackTop, kScrollBarW, trackH,
                                  kScrollBarW * 0.5f,
                                  D2D1::ColorF(0.85f, 0.85f, 0.87f));

            float thumbH = std::max(16.0f, trackH * (float)mMaxVisible / (float)total);
            float thumbT = trackTop + (trackH - thumbH) * (float)mScrollOffset / (float)(total - mMaxVisible);

            mGfx->FillRoundedRect(barX, thumbT, kScrollBarW, thumbH,
                                  kScrollBarW * 0.5f,
                                  D2D1::ColorF(0.20f, 0.47f, 0.75f));
        }
    }

    mGfx->SetAliased(true);
}

bool CustomDropdown::OnMouseMove(float mx, float my) {
    bool prevHeader = mHeaderHover;
    int  prevItem = mHoverItemIdx;

    mHeaderHover = HitTestHeader(mx, my);
    mHoverItemIdx = HitTestItem(mx, my);

    if (prevHeader != mHeaderHover || prevItem != mHoverItemIdx)
        InvalidateRect(mHwnd, nullptr, FALSE);

    return mHeaderHover || HitTestList(mx, my);
}

bool CustomDropdown::OnMouseDown(float mx, float my) {
    if (HitTestHeader(mx, my)) {
        mOpen = !mOpen;
        if (mOpen) ClampScrollOffset();
        mHoverItemIdx = -1;
        InvalidateRect(mHwnd, nullptr, FALSE);
        return true;
    }

    if (mOpen) {
        int idx = HitTestItem(mx, my);
        if (idx >= 0) {
            mSelectedIdx = idx;
            mOpen = false;
            mHoverItemIdx = -1;
            InvalidateRect(mHwnd, nullptr, FALSE);
        }
        else {
            mOpen = false;
            InvalidateRect(mHwnd, nullptr, FALSE);
        }
        return true;
    }

    return false;
}

bool CustomDropdown::OnMouseUp(float mx, float my) {
    return HitTestHeader(mx, my) || HitTestList(mx, my);
}

void CustomDropdown::OnMouseLeave() {
    bool changed = mHeaderHover || (mHoverItemIdx >= 0);
    mHeaderHover = false;
    mHoverItemIdx = -1;
    if (changed) InvalidateRect(mHwnd, nullptr, FALSE);
}

bool CustomDropdown::OnMouseWheel(float mx, float my, int delta) {
    if (!mOpen) return false;
    if (!HitTestList(mx, my) && !HitTestHeader(mx, my)) return false;

    mScrollOffset -= (delta > 0 ? 1 : -1);
    ClampScrollOffset();
    mHoverItemIdx = HitTestItem(mx, my);
    InvalidateRect(mHwnd, nullptr, FALSE);
    return true;
}