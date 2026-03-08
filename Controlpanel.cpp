#include "pch.h"
#include "ControlPanel.hpp"
#include "Config.hpp"
#include <algorithm>
#include <cwchar>

extern AcConfig         g_cfg;
extern CRITICAL_SECTION g_statsLock;

static const wchar_t* kTabLabels[2] = { L"Sliders", L"Keys" };

static void PopulateFKeyDropdown(CustomDropdown* dd) {
    for (int i = 0; i < KeySelector::kKeyCount; ++i)
        dd->AddItem(KeySelector::kKeyNames[i], KeySelector::kVkCodes[i]);
}

static int FKeyIndexForVK(int vk) {
    for (int i = 0; i < KeySelector::kKeyCount; ++i)
        if (KeySelector::kVkCodes[i] == vk) return i;
    return 0;
}

ControlPanel::ControlPanel() {}

ControlPanel::~ControlPanel() {
    delete mSliderCps;
    delete mSliderCooldown;
    delete mSliderTrigCD;
    delete mKeySelector;
    delete mDdDebugToggle;
    delete mDdControlToggle;
    delete mGfx;
}

void ControlPanel::SetStage(HWND hwnd) {
    mHwnd = hwnd;
    RECT rc;
    GetClientRect(hwnd, &rc);
    mWinW = (float)(rc.right - rc.left);
    mWinH = (float)(rc.bottom - rc.top);
    mGfx = new Graphics(hwnd);
    mGfx->Init(true);
    RebuildWidgets();
}

void ControlPanel::Resize(int w, int h) {
    mWinW = (float)w;
    mWinH = (float)h;
    if (mGfx) mGfx->Resize((UINT)w, (UINT)h);
    RebuildWidgets();
}

void ControlPanel::RebuildWidgets() {
    delete mSliderCps;         mSliderCps = nullptr;
    delete mSliderCooldown;    mSliderCooldown = nullptr;
    delete mSliderTrigCD;      mSliderTrigCD = nullptr;
    delete mKeySelector;       mKeySelector = nullptr;
    delete mDdDebugToggle;     mDdDebugToggle = nullptr;
    delete mDdControlToggle;   mDdControlToggle = nullptr;

    const float hPad = mWinW * 0.09f;
    const float sliderX = hPad;
    const float sliderW = mWinW - 2.0f * hPad;
    const float contentH = mWinH - kContentY - 12.0f;
    const float rowH = contentH / 3.0f;

    const float kTitleOff = 10.0f;
    const float kTitleH = 14.0f;
    const float kValueOff = kTitleOff + kTitleH + 5.0f;
    const float kValueH = 18.0f;
    const float kSliderOff = kValueOff + kValueH + 10.0f;

    static const D2D1::ColorF kFills[3] = {
        D2D1::ColorF(0.20f, 0.47f, 0.75f),
        D2D1::ColorF(0.80f, 0.50f, 0.10f),
        D2D1::ColorF(0.12f, 0.60f, 0.55f)
    };
    static const float kMins[3] = { 10.0f, 0.5f,  4.0f };
    static const float kMaxs[3] = { 22.0f, 3.0f, 10.0f };

    float defs[3] = { g_cfg.cps, g_cfg.cooldown, g_cfg.triggerCooldown };
    Slider** targets[3] = { &mSliderCps, &mSliderCooldown, &mSliderTrigCD };

    for (int r = 0; r < 3; ++r) {
        float rowTop = kContentY + rowH * r;
        float sliderY = rowTop + kSliderOff;
        *targets[r] = new Slider(mHwnd, mGfx,
                                 sliderX, sliderY, sliderW,
                                 kMins[r], kMaxs[r], defs[r],
                                 D2D1::ColorF(0.78f, 0.78f, 0.80f), kFills[r]);
    }

    mSliderCps->SetOnChange([](float v) { g_cfg.cps = v; SaveConfig(g_cfg); });
    mSliderCooldown->SetOnChange([](float v) { g_cfg.cooldown = v; SaveConfig(g_cfg); });
    mSliderTrigCD->SetOnChange([](float v) { g_cfg.triggerCooldown = v; SaveConfig(g_cfg); });

    float pairTotal = kComboW * 2.0f + kPairGap;
    float ksX = (mWinW - pairTotal) * 0.5f;
    float ksY = kContentY + 14.0f;

    mKeySelector = new KeySelector(mHwnd, mGfx, ksX, ksY, kComboW);
    mKeySelector->SetLClickVK(g_cfg.lClickVK);
    mKeySelector->SetRClickVK(g_cfg.rClickVK);

    float toggleDropY = ksY + 16.0f + 4.0f + 28.0f + 26.0f + 16.0f + 4.0f;
    float lx = ksX;
    float rx = ksX + kComboW + kPairGap;

    mDdDebugToggle = new CustomDropdown(mHwnd, mGfx, lx, toggleDropY, kComboW, 28.0f, 24.0f, 3);
    PopulateFKeyDropdown(mDdDebugToggle);
    mDdDebugToggle->SetSelectedIndex(FKeyIndexForVK(g_cfg.debugToggleVK));

    mDdControlToggle = new CustomDropdown(mHwnd, mGfx, rx, toggleDropY, kComboW, 28.0f, 24.0f, 3);
    PopulateFKeyDropdown(mDdControlToggle);
    mDdControlToggle->SetSelectedIndex(FKeyIndexForVK(g_cfg.controlToggleVK));
}

bool ControlPanel::IsDragArea(float mx, float my) const {
    return my >= 0.0f && my < kDragH && mx >= 0.0f && mx <= mWinW;
}

int ControlPanel::TabHitTest(float mx, float my) const {
    if (my < kTabY || my > kTabY + kTabH) return -1;
    float tabW = mWinW / (float)kPageCount;
    int   i = (int)(mx / tabW);
    return (i >= 0 && i < kPageCount) ? i : -1;
}

void ControlPanel::UpdateOverflowHeight() {
    float needed = (mPage == 0) ? kBaseH : kKeysH;

    if (mDdDebugToggle && mDdDebugToggle->IsOpen())
        needed = std::max(needed, mDdDebugToggle->GetBottom() + 14.0f);
    if (mDdControlToggle && mDdControlToggle->IsOpen())
        needed = std::max(needed, mDdControlToggle->GetBottom() + 14.0f);
    if (mKeySelector) {
        float kb = mKeySelector->GetDropdownBottom();
        if (kb > 0.0f) needed = std::max(needed, kb + 14.0f);
    }

    RECT wr{};
    GetWindowRect(mHwnd, &wr);
    int newH = (int)needed;
    if (newH != (wr.bottom - wr.top))
        SetWindowPos(mHwnd, nullptr, 0, 0, wr.right - wr.left, newH,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void ControlPanel::RenderBackground() {
    mGfx->SetAliased(false);
    mGfx->FillRoundedRect(0.0f, 0.0f, mWinW, mWinH, kCornerR,
                          D2D1::ColorF(0.97f, 0.96f, 0.93f));
    mGfx->DrawRoundedRect(0.5f, 0.5f, mWinW - 1.0f, mWinH - 1.0f, kCornerR,
                          D2D1::ColorF(0.60f, 0.60f, 0.63f, 0.85f), 1.5f);
    mGfx->SetAliased(true);

    float dotGap = 7.0f;
    float dotR = 2.2f;
    float dotsY = kDragH * 0.5f;
    float startX = mWinW * 0.5f - dotGap;
    for (int d = 0; d < 3; ++d)
        mGfx->FillCircle(startX + d * dotGap, dotsY, dotR,
                         D2D1::ColorF(0.58f, 0.58f, 0.61f));
}

void ControlPanel::RenderTabs() {
    float tabW = mWinW / (float)kPageCount;

    mGfx->SetAliased(false);
    mGfx->FillRoundedRect(0.0f, kTabY, mWinW, kTabH,
                          0.0f, D2D1::ColorF(0.90f, 0.89f, 0.87f));
    mGfx->SetAliased(true);

    for (int i = 0; i < kPageCount; ++i) {
        float tx = i * tabW;
        bool  active = (i == mPage);
        bool  hover = (i == mTabHover && !active);

        D2D1::ColorF bg = active ? D2D1::ColorF(0.20f, 0.47f, 0.75f)
            : hover ? D2D1::ColorF(0.84f, 0.89f, 0.97f)
            : D2D1::ColorF(0.90f, 0.89f, 0.87f);
        D2D1::ColorF fg = active ? D2D1::ColorF(1.0f, 1.0f, 1.0f)
            : D2D1::ColorF(0.25f, 0.25f, 0.28f);

        mGfx->SetAliased(false);
        mGfx->FillRoundedRect(tx + 4.0f, kTabY + 3.0f, tabW - 8.0f, kTabH - 4.0f, 6.0f, bg);
        mGfx->SetAliased(true);
        mGfx->DrawTextCentered(kTabLabels[i], tx, kTabY, tabW, kTabH, fg, 12.0f);
    }

    mGfx->SetAliased(false);
    mGfx->FillRoundedRect(8.0f, kTabY + kTabH + 1.0f, mWinW - 16.0f, 1.5f, 0.0f,
                          D2D1::ColorF(0.76f, 0.76f, 0.79f));
    mGfx->SetAliased(true);
}

void ControlPanel::RenderSlidersPage() {
    const float hPad = mWinW * 0.09f;
    const float sliderX = hPad;
    const float sliderW = mWinW - 2.0f * hPad;
    const float contentH = mWinH - kContentY - 12.0f;
    const float rowH = contentH / 3.0f;

    static const wchar_t* kTitles[3] = {
        L"Clicks Per Second", L"Cooldown Time", L"Cooldown After"
    };
    static const wchar_t* kUnits[3] = { L"", L"s", L"s" };
    static const float    kMins[3] = { 10.0f, 0.5f,  4.0f };
    static const float    kMaxs[3] = { 22.0f, 3.0f, 10.0f };

    Slider* sliders[3] = { mSliderCps, mSliderCooldown, mSliderTrigCD };

    D2D1::ColorF labelCol(0.50f, 0.50f, 0.53f);
    D2D1::ColorF titleCol(0.16f, 0.16f, 0.18f);
    D2D1::ColorF valCol(0.20f, 0.47f, 0.75f);

    for (int r = 0; r < 3; ++r) {
        if (!sliders[r]) continue;

        float rowTop = kContentY + rowH * r;
        float titleY = rowTop + 10.0f;
        mGfx->DrawTextCentered(kTitles[r], 0.0f, titleY, mWinW, 14.0f, titleCol, 11.5f);

        float valueY = titleY + 14.0f + 5.0f;
        wchar_t buf[32];
        swprintf_s(buf, L"%.1f%s", sliders[r]->GetValue(), kUnits[r]);
        mGfx->DrawTextCentered(buf, 0.0f, valueY, mWinW, 18.0f, valCol, 14.0f);

        sliders[r]->Render();

        float sliderThumbY = valueY + 18.0f + 10.0f;
        float minMaxY = sliderThumbY + 10.0f;
        wchar_t lo[16], hi[16];
        swprintf_s(lo, L"%.1f", kMins[r]);
        swprintf_s(hi, L"%.1f", kMaxs[r]);
        mGfx->DrawTextLeft(lo, sliderX, minMaxY, 36.0f, 13.0f, labelCol, 10.0f);
        mGfx->DrawTextLeft(hi, sliderX + sliderW - 32.0f, minMaxY, 36.0f, 13.0f, labelCol, 10.0f);

        if (r < 2) {
            float divY = rowTop + rowH - 1.0f;
            mGfx->SetAliased(false);
            mGfx->FillRoundedRect(sliderX, divY, sliderW, 1.0f, 0.0f,
                                  D2D1::ColorF(0.82f, 0.82f, 0.85f));
            mGfx->SetAliased(true);
        }
    }
}

void ControlPanel::RenderKeysPage() {
    if (!mKeySelector) return;

    float pairTotal = kComboW * 2.0f + kPairGap;
    float lx = (mWinW - pairTotal) * 0.5f;
    float rx = lx + kComboW + kPairGap;
    float ksY = kContentY + 14.0f;
    float toggleLabelY = ksY + 16.0f + 4.0f + 28.0f + 26.0f;

    D2D1::ColorF labelCol(0.24f, 0.24f, 0.27f);
    mGfx->DrawTextLeft(L"Debug Panel Toggle", lx, toggleLabelY, kComboW, 16.0f, labelCol, 11.0f);
    mGfx->DrawTextLeft(L"Control Panel Toggle", rx, toggleLabelY, kComboW, 16.0f, labelCol, 11.0f);

    if (mDdDebugToggle)   mDdDebugToggle->Render();
    if (mDdControlToggle) mDdControlToggle->Render();
    mKeySelector->Render();
}

void ControlPanel::SyncKeysToConfig() {
    if (mKeySelector) {
        g_cfg.lClickVK = mKeySelector->GetLClickVK();
        g_cfg.rClickVK = mKeySelector->GetRClickVK();
    }
    if (mDdDebugToggle)   g_cfg.debugToggleVK = mDdDebugToggle->GetSelectedValue();
    if (mDdControlToggle) g_cfg.controlToggleVK = mDdControlToggle->GetSelectedValue();
    SaveConfig(g_cfg);
}

void ControlPanel::Render() {
    if (!mGfx) return;
    mGfx->BeginDraw();
    RenderBackground();
    RenderTabs();
    switch (mPage) {
    case 0: RenderSlidersPage(); break;
    case 1: RenderKeysPage();    break;
    }
    mGfx->EndDraw();
}

void ControlPanel::OnMouseMove(float mx, float my) {
    int prev = mTabHover;
    mTabHover = TabHitTest(mx, my);
    if (mTabHover != prev) InvalidateRect(mHwnd, nullptr, FALSE);

    switch (mPage) {
    case 0:
        if (mSliderCps)      mSliderCps->OnMouseMove(mx, my);
        if (mSliderCooldown) mSliderCooldown->OnMouseMove(mx, my);
        if (mSliderTrigCD)   mSliderTrigCD->OnMouseMove(mx, my);
        break;
    case 1:
        if (mDdDebugToggle)   mDdDebugToggle->OnMouseMove(mx, my);
        if (mDdControlToggle) mDdControlToggle->OnMouseMove(mx, my);
        if (mKeySelector)     mKeySelector->OnMouseMove(mx, my);
        break;
    }
}

void ControlPanel::OnMouseDown(float mx, float my) {
    int hitTab = TabHitTest(mx, my);
    if (hitTab >= 0 && hitTab != mPage) {
        mPage = hitTab;
        RECT wr{};
        GetWindowRect(mHwnd, &wr);
        int targetH = (mPage == 0) ? (int)kBaseH : (int)kKeysH;
        SetWindowPos(mHwnd, nullptr, 0, 0, wr.right - wr.left, targetH,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(mHwnd, nullptr, FALSE);
        return;
    }

    switch (mPage) {
    case 0:
        if (mSliderCps)      mSliderCps->OnMouseDown(mx, my);
        if (mSliderCooldown) mSliderCooldown->OnMouseDown(mx, my);
        if (mSliderTrigCD)   mSliderTrigCD->OnMouseDown(mx, my);
        break;
    case 1: {
        bool ksConsumed = mKeySelector && mKeySelector->OnMouseDown(mx, my);
        if (ksConsumed) {
            if (mDdDebugToggle && mDdDebugToggle->IsOpen())   mDdDebugToggle->OnMouseLeave();
            if (mDdControlToggle && mDdControlToggle->IsOpen()) mDdControlToggle->OnMouseLeave();
        }
        else {
            if (mDdDebugToggle)   mDdDebugToggle->OnMouseDown(mx, my);
            if (mDdControlToggle) mDdControlToggle->OnMouseDown(mx, my);
        }
        SyncKeysToConfig();
        UpdateOverflowHeight();
        break;
    }
    }
}

void ControlPanel::OnMouseUp(float mx, float my) {
    switch (mPage) {
    case 0:
        if (mSliderCps)      mSliderCps->OnMouseUp(mx, my);
        if (mSliderCooldown) mSliderCooldown->OnMouseUp(mx, my);
        if (mSliderTrigCD)   mSliderTrigCD->OnMouseUp(mx, my);
        break;
    case 1:
        if (mKeySelector)     mKeySelector->OnMouseUp(mx, my);
        if (mDdDebugToggle)   mDdDebugToggle->OnMouseUp(mx, my);
        if (mDdControlToggle) mDdControlToggle->OnMouseUp(mx, my);
        SyncKeysToConfig();
        UpdateOverflowHeight();
        break;
    }
}

void ControlPanel::OnMouseLeave() {
    mTabHover = -1;
    if (mSliderCps)       mSliderCps->OnMouseLeave();
    if (mSliderCooldown)  mSliderCooldown->OnMouseLeave();
    if (mSliderTrigCD)    mSliderTrigCD->OnMouseLeave();
    if (mKeySelector)     mKeySelector->OnMouseLeave();
    if (mDdDebugToggle)   mDdDebugToggle->OnMouseLeave();
    if (mDdControlToggle) mDdControlToggle->OnMouseLeave();
    InvalidateRect(mHwnd, nullptr, FALSE);
}

void ControlPanel::OnMouseWheel(float mx, float my, int delta) {
    if (mPage != 1) return;
    if (mKeySelector && mKeySelector->OnMouseWheel(mx, my, delta))     return;
    if (mDdDebugToggle && mDdDebugToggle->OnMouseWheel(mx, my, delta))   return;
    if (mDdControlToggle && mDdControlToggle->OnMouseWheel(mx, my, delta)) return;
}