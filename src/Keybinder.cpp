#include "pch.h"
#include "KeyBinder.hpp"
#include <algorithm>
#include <cwchar>

// ---------------------------------------------------------------------------
// VkToName  – human-readable key name from a virtual-key code
// ---------------------------------------------------------------------------
std::wstring KeyBinder::VkToName(int vk)
{
    if (vk == 0) return L"None";

    // Special-case a handful of VKs that GetKeyNameText mis-labels or skips
    switch (vk) {
    case VK_LBUTTON:  return L"Left Mouse";
    case VK_RBUTTON:  return L"Right Mouse";
    case VK_MBUTTON:  return L"Middle Mouse";
    case VK_XBUTTON1: return L"Mouse X1";
    case VK_XBUTTON2: return L"Mouse X2";
    case VK_ESCAPE:   return L"Escape";
    case VK_RETURN:   return L"Enter";
    case VK_SPACE:    return L"Space";
    case VK_TAB:      return L"Tab";
    case VK_BACK:     return L"Backspace";
    case VK_DELETE:   return L"Delete";
    case VK_INSERT:   return L"Insert";
    case VK_HOME:     return L"Home";
    case VK_END:      return L"End";
    case VK_PRIOR:    return L"Page Up";
    case VK_NEXT:     return L"Page Down";
    case VK_LEFT:     return L"Left";
    case VK_RIGHT:    return L"Right";
    case VK_UP:       return L"Up";
    case VK_DOWN:     return L"Down";
    case VK_SHIFT:    return L"Shift";
    case VK_CONTROL:  return L"Ctrl";
    case VK_MENU:     return L"Alt";
    case VK_CAPITAL:  return L"Caps Lock";
    default: break;
    }

    // Use GetKeyNameText for everything else (F-keys, letter/number keys, …)
    UINT scanCode = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    if (scanCode != 0) {
        wchar_t name[64] = {};
        // Extended keys need the extended bit set in the lParam
        LONG lParam = static_cast<LONG>(scanCode << 16);
        // Mark some keys that need the extended bit
        if (vk == VK_NUMLOCK || vk == VK_RCONTROL || vk == VK_RMENU ||
            vk == VK_RSHIFT || vk == VK_DIVIDE || vk == VK_SNAPSHOT)
            lParam |= (1 << 24);
        if (GetKeyNameTextW(lParam, name, 64) > 0)
            return name;
    }

    // Fallback: hex code
    wchar_t buf[16];
    swprintf_s(buf, L"VK 0x%02X", vk);
    return buf;
}

// ---------------------------------------------------------------------------
KeyBinder::KeyBinder(HWND hwnd, Graphics* gfx,
                     float x, float y, float w, float h,
                     const std::wstring& label)
    : mHwnd(hwnd), mGfx(gfx),
    mX(x), mY(y), mW(w), mH(h),
    mLabel(label)
{
}

bool KeyBinder::HitTest(float mx, float my) const
{
    return mx >= mX && mx <= mX + mW &&
        my >= mY && my <= mY + mH;
}

void KeyBinder::SetKey(int vk)
{
    mBoundKey = vk;
    InvalidateRect(mHwnd, nullptr, FALSE);
}

void KeyBinder::OnMouseMove(float mx, float my)
{
    if (mState == State::Waiting) return;           // don't flicker while recording
    State next = HitTest(mx, my) ? State::Hover : State::Normal;
    if (next != mState) {
        mState = next;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
}

void KeyBinder::OnMouseLeave()
{
    if (mState == State::Waiting) return;
    if (mState != State::Normal) {
        mState = State::Normal;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
}

void KeyBinder::OnMouseDown(float mx, float my)
{
    if (HitTest(mx, my)) {
        // Toggle into / out of waiting
        mState = (mState == State::Waiting) ? State::Normal : State::Waiting;
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
    else {
        // Click outside cancels recording
        if (mState == State::Waiting) {
            mState = State::Normal;
            InvalidateRect(mHwnd, nullptr, FALSE);
        }
    }
}

bool KeyBinder::OnKeyDown(int vk)
{
    if (mState != State::Waiting) return false;

    if (vk == VK_ESCAPE) {
        // Cancel without saving
        mState = State::Normal;
        InvalidateRect(mHwnd, nullptr, FALSE);
        return true;
    }

    // Record the key
    mBoundKey = vk;
    mState = State::Normal;
    InvalidateRect(mHwnd, nullptr, FALSE);
    if (mOnChange) mOnChange(mBoundKey);
    return true;
}

// ---------------------------------------------------------------------------
void KeyBinder::Render()
{
    // --- colours based on state ---
    D2D1::ColorF bg(1.0f, 1.0f, 1.0f);
    D2D1::ColorF border(0.65f, 0.65f, 0.67f);
    D2D1::ColorF textCol(0.12f, 0.12f, 0.14f);
    float strokeW = 1.5f;

    switch (mState) {
    case State::Waiting:
        bg = D2D1::ColorF(1.0f, 0.96f, 0.86f);
        border = D2D1::ColorF(0.88f, 0.48f, 0.08f);
        textCol = D2D1::ColorF(0.72f, 0.30f, 0.02f);
        strokeW = 2.0f;
        break;
    case State::Hover:
        bg = D2D1::ColorF(0.91f, 0.95f, 1.00f);
        border = D2D1::ColorF(0.20f, 0.47f, 0.75f);
        break;
    default: break;
    }

    // --- draw box ---
    mGfx->SetAliased(false);
    mGfx->FillRoundedRect(mX, mY, mW, mH, 7.0f, bg);
    mGfx->DrawRoundedRect(mX, mY, mW, mH, 7.0f, border, strokeW);
    mGfx->SetAliased(true);

    // --- draw content text ---
    if (mState == State::Waiting) {
        mGfx->DrawTextCentered(L"Press any key…", mX, mY, mW, mH, textCol, 12.0f);
    }
    else {
        std::wstring display = VkToName(mBoundKey);
        mGfx->DrawTextCentered(display, mX, mY, mW, mH, textCol, 13.0f);
    }
}