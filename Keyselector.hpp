#pragma once
#ifndef KEY_SELECTOR_CLASS_HEADER
#define KEY_SELECTOR_CLASS_HEADER

#include <Windows.h>
#include "Graphics.hpp"
#include "CustomDropdown.hpp"

class KeySelector {
public:
    static constexpr int kKeyCount = 12;
    static const int            kVkCodes[kKeyCount];
    static const wchar_t* const kKeyNames[kKeyCount];

    KeySelector(HWND parentHwnd, Graphics* gfx,
                float x, float y, float comboW = 148.0f);
    ~KeySelector();

    void Render();

    bool OnMouseMove(float mx, float my);
    bool OnMouseDown(float mx, float my);
    bool OnMouseUp(float mx, float my);
    void OnMouseLeave();
    bool OnMouseWheel(float mx, float my, int delta);

    int  GetLClickVK() const;
    int  GetRClickVK() const;
    void SetLClickVK(int vk);   
    void SetRClickVK(int vk);   

private:
    void SyncMutualExclusion(CustomDropdown* changed, CustomDropdown* other);
    int  IndexForVK(int vk) const; 

    static constexpr float kLabelH = 16.0f;
    static constexpr float kLabelGap = 4.0f;
    static constexpr float kPairGap = 20.0f;

    HWND      mParent = nullptr;
    Graphics* mGfx = nullptr;
    float     mX = 0.0f;
    float     mY = 0.0f;
    float     mComboW = 148.0f;

    CustomDropdown* mLDrop = nullptr;
    CustomDropdown* mRDrop = nullptr;
};

#endif