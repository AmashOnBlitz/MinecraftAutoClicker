#include "pch.h"
#include "KeySelector.hpp"

const int KeySelector::kVkCodes[kKeyCount] = {
    VK_F1,  VK_F2,  VK_F3,  VK_F4,
    VK_F5,  VK_F6,  VK_F7,  VK_F8,
    VK_F9,  VK_F10, VK_F11, VK_F12
};
const wchar_t* const KeySelector::kKeyNames[kKeyCount] = {
    L"F1",  L"F2",  L"F3",  L"F4",
    L"F5",  L"F6",  L"F7",  L"F8",
    L"F9",  L"F10", L"F11", L"F12"
};

KeySelector::KeySelector(HWND parentHwnd, Graphics* gfx,
                         float x, float y, float comboW)
    : mParent(parentHwnd), mGfx(gfx), mX(x), mY(y), mComboW(comboW)
{
    float dropY = y + kLabelH + kLabelGap;
    float rx = x + comboW + kPairGap;
    mLDrop = new CustomDropdown(parentHwnd, gfx, x, dropY, comboW);
    mRDrop = new CustomDropdown(parentHwnd, gfx, rx, dropY, comboW);
    for (int i = 0; i < kKeyCount; ++i) {
        mLDrop->AddItem(kKeyNames[i], kVkCodes[i]);
        mRDrop->AddItem(kKeyNames[i], kVkCodes[i]);
    }
    mLDrop->SetSelectedIndex(8);
    mRDrop->SetSelectedIndex(9);
}

KeySelector::~KeySelector() { delete mLDrop; delete mRDrop; }

int KeySelector::IndexForVK(int vk) const {
    for (int i = 0; i < kKeyCount; ++i)
        if (kVkCodes[i] == vk) return i;
    return 0;
}

void KeySelector::SetLClickVK(int vk) {
    if (mLDrop) { mLDrop->SetSelectedIndex(IndexForVK(vk)); SyncMutualExclusion(mLDrop, mRDrop); }
}
void KeySelector::SetRClickVK(int vk) {
    if (mRDrop) { mRDrop->SetSelectedIndex(IndexForVK(vk)); SyncMutualExclusion(mRDrop, mLDrop); }
}

float KeySelector::GetDropdownBottom() const {
    float b = 0.0f;
    if (mLDrop && mLDrop->IsOpen()) b = std::max(b, mLDrop->GetBottom());
    if (mRDrop && mRDrop->IsOpen()) b = std::max(b, mRDrop->GetBottom());
    return b;
}

void KeySelector::Render() {
    D2D1::ColorF lc(0.20f, 0.20f, 0.22f);
    mGfx->DrawTextLeft(L"L-Click Toggle", mX, mY, mComboW, kLabelH, lc, 11.0f);
    mGfx->DrawTextLeft(L"R-Click Toggle", mX + mComboW + kPairGap, mY, mComboW, kLabelH, lc, 11.0f);
    if (mLDrop) mLDrop->Render();
    if (mRDrop) mRDrop->Render();
}

bool KeySelector::OnMouseMove(float mx, float my) {
    bool l = mLDrop ? mLDrop->OnMouseMove(mx, my) : false;
    bool r = mRDrop ? mRDrop->OnMouseMove(mx, my) : false;
    return l || r;
}
bool KeySelector::OnMouseDown(float mx, float my) {
    int pl = mLDrop ? mLDrop->GetSelectedIndex() : -1;
    int pr = mRDrop ? mRDrop->GetSelectedIndex() : -1;
    bool l = mLDrop ? mLDrop->OnMouseDown(mx, my) : false;
    bool r = (!l && mRDrop) ? mRDrop->OnMouseDown(mx, my) : false;
    if (mLDrop && mLDrop->GetSelectedIndex() != pl) SyncMutualExclusion(mLDrop, mRDrop);
    else if (mRDrop && mRDrop->GetSelectedIndex() != pr) SyncMutualExclusion(mRDrop, mLDrop);
    return l || r;
}
bool KeySelector::OnMouseUp(float mx, float my) {
    bool l = mLDrop ? mLDrop->OnMouseUp(mx, my) : false;
    bool r = mRDrop ? mRDrop->OnMouseUp(mx, my) : false;
    return l || r;
}
void KeySelector::OnMouseLeave() {
    if (mLDrop) mLDrop->OnMouseLeave();
    if (mRDrop) mRDrop->OnMouseLeave();
}
bool KeySelector::OnMouseWheel(float mx, float my, int delta) {
    bool l = mLDrop ? mLDrop->OnMouseWheel(mx, my, delta) : false;
    bool r = (!l && mRDrop) ? mRDrop->OnMouseWheel(mx, my, delta) : false;
    return l || r;
}

int  KeySelector::GetLClickVK() const { return mLDrop ? mLDrop->GetSelectedValue() : VK_F1; }
int  KeySelector::GetRClickVK() const { return mRDrop ? mRDrop->GetSelectedValue() : VK_F2; }

void KeySelector::SyncMutualExclusion(CustomDropdown* changed, CustomDropdown* other) {
    int sel = changed->GetSelectedIndex();
    if (other->GetSelectedIndex() == sel)
        for (int i = 0; i < kKeyCount; ++i)
            if (i != sel) { other->SetSelectedIndex(i); break; }
}