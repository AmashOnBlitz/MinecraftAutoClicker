#pragma once
#ifndef KEYBINDER_CLASS_HEADER
#define KEYBINDER_CLASS_HEADER

#include "Graphics.hpp"
#include <string>
#include <functional>

// ---------------------------------------------------------------------------
// KeyBinder
//   Click the box  → enters "Waiting" state  (pulse-orange border)
//   Press any key  → records that VK, fires OnChange callback
//   Press Escape   → cancels without changing the binding
// ---------------------------------------------------------------------------
class KeyBinder {
public:
    KeyBinder(HWND hwnd, Graphics* gfx,
              float x, float y, float w, float h,
              const std::wstring& label);

    void Render();

    // Input forwarding – OnKeyDown returns true if the event was consumed
    void OnMouseMove(float mx, float my);
    void OnMouseDown(float mx, float my);
    void OnMouseLeave();
    bool OnKeyDown(int vk);

    void SetKey(int vk);
    int  GetKey() const { return mBoundKey; }
    bool IsWaiting() const { return mState == State::Waiting; }

    void SetOnChange(std::function<void(int)> cb) { mOnChange = cb; }

    // Shared helper used by Renderer and dllmain for display names
    static std::wstring VkToName(int vk);

private:
    bool HitTest(float mx, float my) const;

    enum class State { Normal, Hover, Waiting };
    State mState = State::Normal;

    HWND         mHwnd;
    Graphics* mGfx;
    float        mX, mY, mW, mH;
    std::wstring mLabel;
    int          mBoundKey = 0;

    std::function<void(int)> mOnChange;
};

#endif // !KEYBINDER_CLASS_HEADER