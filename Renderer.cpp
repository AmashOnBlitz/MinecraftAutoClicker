#include "renderer.hpp"

#pragma warning(push)
#pragma warning(disable : 4244)

static constexpr float kComboW = 148.0f;
static constexpr float kPairGap = 20.0f;
static constexpr float kComboPair = kComboW * 2.0f + kPairGap;

static constexpr float kToggleSectionY = 295.0f;
static constexpr float kToggleLabelH = 16.0f;
static constexpr float kToggleLabelGap = 4.0f;

static void PopulateFKeyDropdown(CustomDropdown* dd) {
    for (int i = 0; i < KeySelector::kKeyCount; ++i)
        dd->AddItem(KeySelector::kKeyNames[i], KeySelector::kVkCodes[i]);
}

void Renderer::SetStage(HWND hWnd)
{
    if (mHWnd != hWnd) {
        mHWnd = hWnd;
        delete mGfx;
        mGfx = new Graphics(hWnd);

        delete btnInject;
        btnInject = new BottomPaddedButton(mHWnd, mGfx, L"Inject", 16.0f, 40.0f);

        const float kLeft = 20.0f;
        const float kRowStride = 30.0f;
        const float kTextWidth = 200.0f;

        float btnX, btnY, btnW, btnH;
        btnInject->GetRect(btnX, btnY, btnW, btnH);

        delete chkDebugPanel;
        chkDebugPanel = new FixedCheckbox(
            mHWnd, mGfx, L"Add Debug Panel",
            kLeft, btnY - (2.0f * kRowStride), kTextWidth);
        chkDebugPanel->SetChecked(true);

        delete chkControlDialog;
        chkControlDialog = new FixedCheckbox(
            mHWnd, mGfx, L"Show Control Dialog",
            kLeft, btnY - kRowStride, kTextWidth);
        chkControlDialog->SetChecked(true);

        RECT rc;
        GetClientRect(mHWnd, &rc);
        float winW = static_cast<float>(rc.right - rc.left);
        float knobR = 34.0f;
        float knobCy = 72.0f;

        delete knobCps;
        knobCps = new CpsKnob(mHWnd, mGfx, winW * 0.25f, knobCy, knobR);
        knobCps->SetValue(18);

        delete knobCooldown;
        knobCooldown = new CooldownKnob(mHWnd, mGfx, winW * 0.50f, knobCy, knobR);
        knobCooldown->SetValue(1.0f);

        delete knobTriggerCooldown;
        knobTriggerCooldown = new TriggerCooldownKnob(mHWnd, mGfx, winW * 0.75f, knobCy, knobR);
        knobTriggerCooldown->SetValue(4.0f);

        static constexpr float kPidBoxSize = 28.0f;
        static constexpr float kPidGap = 6.0f;
        static constexpr int   kPidCount = 6;
        float pidTotalW = kPidCount * kPidBoxSize + (kPidCount - 1) * kPidGap;
        delete pidInput;
        pidInput = new PidInput(mHWnd, mGfx, (winW - pidTotalW) * 0.5f, 158.0f);

        float ksX = (winW - kComboPair) * 0.5f;
        delete keySelector;
        keySelector = new KeySelector(mHWnd, mGfx, ksX, 225.0f, kComboW);

        float toggleDropY = kToggleSectionY + kToggleLabelH + kToggleLabelGap;
        float lx = (winW - kComboPair) * 0.5f;
        float rx = lx + kComboW + kPairGap;

        delete ddDebugToggleKey;
        ddDebugToggleKey = new CustomDropdown(
            mHWnd, mGfx, lx, toggleDropY, kComboW, 28.0f, 24.0f, 3);
        PopulateFKeyDropdown(ddDebugToggleKey);
        ddDebugToggleKey->SetSelectedIndex(10);   // F11

        delete ddControlToggleKey;
        ddControlToggleKey = new CustomDropdown(
            mHWnd, mGfx, rx, toggleDropY, kComboW, 28.0f, 24.0f, 3);
        PopulateFKeyDropdown(ddControlToggleKey);
        ddControlToggleKey->SetSelectedIndex(11);  // F12
    }
}


void Renderer::Render()
{
    this->BaseRender();
    RECT rect;
    GetClientRect(mHWnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    mGfx->BeginDraw();
    mGfx->drawCircle((float)(w / 2), (float)(h / 2),
                     (float)(w / 2), (float)(h / 2),
                     D2D1::ColorF::Crimson);
    mGfx->EndDraw();
}

void Renderer::BaseRender()
{
    if (!mGfx) mGfx = new Graphics(mHWnd);
    mGfx->Init();
}

HWND Renderer::getCurrentStage() { return mHWnd; }

void MainWindowRenderer::Render()
{
    this->BaseRender();
    mGfx->BeginDraw();
    mGfx->ClearScreen(D2D1::ColorF(0.96f, 0.94f, 0.90f));

    if (knobCps)             knobCps->render();
    if (knobCooldown)        knobCooldown->render();
    if (knobTriggerCooldown) knobTriggerCooldown->render();
    if (pidInput)            pidInput->Render();
    if (chkDebugPanel)       chkDebugPanel->render();
    if (chkControlDialog)    chkControlDialog->render();
    if (btnInject)           btnInject->render();

    {
        RECT rc;
        GetClientRect(mHWnd, &rc);
        float winW = static_cast<float>(rc.right - rc.left);
        float lx = (winW - kComboPair) * 0.5f;
        float rx = lx + kComboW + kPairGap;
        D2D1::ColorF labelCol(0.20f, 0.20f, 0.22f);
        mGfx->DrawTextLeft(L"Debug Panel Toggle",
                           lx, kToggleSectionY, kComboW, kToggleLabelH, labelCol, 11.0f);
        mGfx->DrawTextLeft(L"Control Panel Toggle",
                           rx, kToggleSectionY, kComboW, kToggleLabelH, labelCol, 11.0f);
    }


    if (ddDebugToggleKey)   ddDebugToggleKey->Render();
    if (ddControlToggleKey) ddControlToggleKey->Render();
    if (keySelector)        keySelector->Render();  

    mGfx->EndDraw();
}

void Renderer::OnMouseMove(float mx, float my)
{
    if (ddDebugToggleKey)   ddDebugToggleKey->OnMouseMove(mx, my);
    if (ddControlToggleKey) ddControlToggleKey->OnMouseMove(mx, my);

    if (keySelector && keySelector->OnMouseMove(mx, my)) {
        if (btnInject)        btnInject->OnMouseLeave();
        if (chkDebugPanel)    chkDebugPanel->OnMouseLeave();
        if (chkControlDialog) chkControlDialog->OnMouseLeave();
        return;
    }

    if (btnInject)           btnInject->OnMouseMove(mx, my);
    if (chkDebugPanel)       chkDebugPanel->OnMouseMove(mx, my);
    if (chkControlDialog)    chkControlDialog->OnMouseMove(mx, my);
    if (knobCps)             knobCps->OnMouseMove(mx, my);
    if (knobCooldown)        knobCooldown->OnMouseMove(mx, my);
    if (knobTriggerCooldown) knobTriggerCooldown->OnMouseMove(mx, my);
}

void Renderer::OnMouseDown(float mx, float my)
{

    mMouseOwner = MouseOwner::None;

    if (keySelector && keySelector->OnMouseDown(mx, my)) {
        mMouseOwner = MouseOwner::KeySelector;
        if (ddDebugToggleKey && ddDebugToggleKey->IsOpen())   ddDebugToggleKey->OnMouseLeave();
        if (ddControlToggleKey && ddControlToggleKey->IsOpen()) ddControlToggleKey->OnMouseLeave();
        return;
    }

    if (ddDebugToggleKey && ddDebugToggleKey->OnMouseDown(mx, my)) {
        mMouseOwner = MouseOwner::DebugToggleDrop;
        if (ddControlToggleKey && ddControlToggleKey->IsOpen()) ddControlToggleKey->OnMouseLeave();
        return;
    }
    if (ddControlToggleKey && ddControlToggleKey->OnMouseDown(mx, my)) {
        mMouseOwner = MouseOwner::ControlToggleDrop;
        if (ddDebugToggleKey && ddDebugToggleKey->IsOpen()) ddDebugToggleKey->OnMouseLeave();
        return;
    }

    mMouseOwner = MouseOwner::Other;
    if (btnInject)           btnInject->OnMouseDown(mx, my);
    if (chkDebugPanel)       chkDebugPanel->OnMouseDown(mx, my);
    if (chkControlDialog)    chkControlDialog->OnMouseDown(mx, my);
    if (knobCps)             knobCps->OnMouseDown(mx, my);
    if (knobCooldown)        knobCooldown->OnMouseDown(mx, my);
    if (knobTriggerCooldown) knobTriggerCooldown->OnMouseDown(mx, my);
    if (pidInput)            pidInput->OnMouseDown(mx, my);
}

void Renderer::OnMouseUp(float mx, float my)
{
    switch (mMouseOwner) {
    case MouseOwner::KeySelector:
        if (keySelector) keySelector->OnMouseUp(mx, my);
        break;

    case MouseOwner::DebugToggleDrop:
        if (ddDebugToggleKey) ddDebugToggleKey->OnMouseUp(mx, my);
        break;

    case MouseOwner::ControlToggleDrop:
        if (ddControlToggleKey) ddControlToggleKey->OnMouseUp(mx, my);
        break;

    case MouseOwner::Other:
    default:
        if (btnInject)           btnInject->OnMouseUp(mx, my);
        if (chkDebugPanel)       chkDebugPanel->OnMouseUp(mx, my);
        if (chkControlDialog)    chkControlDialog->OnMouseUp(mx, my);
        if (knobCps)             knobCps->OnMouseUp(mx, my);
        if (knobCooldown)        knobCooldown->OnMouseUp(mx, my);
        if (knobTriggerCooldown) knobTriggerCooldown->OnMouseUp(mx, my);
        break;
    }

    mMouseOwner = MouseOwner::None;
}

void Renderer::OnMouseLeave()
{
    mMouseOwner = MouseOwner::None;

    if (ddDebugToggleKey)   ddDebugToggleKey->OnMouseLeave();
    if (ddControlToggleKey) ddControlToggleKey->OnMouseLeave();

    if (btnInject)           btnInject->OnMouseLeave();
    if (chkDebugPanel)       chkDebugPanel->OnMouseLeave();
    if (chkControlDialog)    chkControlDialog->OnMouseLeave();
    if (knobCps)             knobCps->OnMouseLeave();
    if (knobCooldown)        knobCooldown->OnMouseLeave();
    if (knobTriggerCooldown) knobTriggerCooldown->OnMouseLeave();
    if (keySelector)         keySelector->OnMouseLeave();
}

void Renderer::OnMouseWheel(float mx, float my, int delta)
{
    if (keySelector && keySelector->OnMouseWheel(mx, my, delta)) return;
    if (ddDebugToggleKey && ddDebugToggleKey->OnMouseWheel(mx, my, delta))   return;
    if (ddControlToggleKey && ddControlToggleKey->OnMouseWheel(mx, my, delta)) return;
}

void Renderer::OnChar(wchar_t ch) { if (pidInput) pidInput->OnChar(ch); }
void Renderer::OnKeyDown(int vk) { if (pidInput) pidInput->OnKeyDown(vk); }
void Renderer::OnCommand(WPARAM w, LPARAM l) { (void)w; (void)l; }

#pragma warning(pop)