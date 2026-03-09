#pragma once
#ifndef CONTROL_PANEL_CLASS_HEADER
#define CONTROL_PANEL_CLASS_HEADER

#include "Graphics.hpp"
#include "Slider.hpp"
#include "KeySelector.hpp"
#include "CustomDropdown.hpp"

class ControlPanel {
public:
    ControlPanel();
    ~ControlPanel();

    void SetStage(HWND hwnd);
    void Render();
    void Resize(int w, int h);

    void OnMouseMove(float mx, float my);
    void OnMouseDown(float mx, float my);
    void OnMouseUp(float mx, float my);
    void OnMouseLeave();
    void OnMouseWheel(float mx, float my, int delta);

    bool IsDragArea(float mx, float my) const;

private:
    void RebuildWidgets();
    void RenderBackground();
    void RenderTabs();
    void RenderSlidersPage();
    void RenderKeysPage();
    void SyncKeysToConfig();
    int  TabHitTest(float mx, float my) const;
    void UpdateOverflowHeight();

    static constexpr int   kPageCount = 2;
    static constexpr float kBaseW = 400.0f;
    static constexpr float kBaseH = 420.0f;
    static constexpr float kKeysH = 340.0f;
    static constexpr float kCornerR = 12.0f;
    static constexpr float kDragH = 22.0f;
    static constexpr float kTabY = kDragH;
    static constexpr float kTabH = 30.0f;
    static constexpr float kContentY = kDragH + kTabH + 8.0f;
    static constexpr float kComboW = 150.0f;
    static constexpr float kPairGap = 16.0f;

    HWND      mHwnd = nullptr;
    Graphics* mGfx = nullptr;
    int       mPage = 0;
    int       mTabHover = -1;
    float     mWinW = kBaseW;
    float     mWinH = kBaseH;

    Slider* mSliderCps = nullptr;
    Slider* mSliderCooldown = nullptr;
    Slider* mSliderTrigCD = nullptr;
    KeySelector* mKeySelector = nullptr;
    CustomDropdown* mDdDebugToggle = nullptr;
    CustomDropdown* mDdControlToggle = nullptr;
};

#endif