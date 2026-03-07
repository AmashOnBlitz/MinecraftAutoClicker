#include "renderer.hpp"

#pragma warning(push)
#pragma warning(disable : 4244)

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
            mHWnd, mGfx,
            L"Add Debug Panel",
            kLeft, btnY - (2.0f * kRowStride),
            kTextWidth);
        chkDebugPanel->SetChecked(true);

        delete chkControlDialog;
        chkControlDialog = new FixedCheckbox(
            mHWnd, mGfx,
            L"Show Control Dialog",
            kLeft, btnY - kRowStride,
            kTextWidth);
        chkControlDialog->SetChecked(true);

        RECT rc;
        GetClientRect(mHWnd, &rc);
        float winW = static_cast<float>(rc.right - rc.left);
        float knobR = 34.0f;
        float knobCy = 72.0f;

        float col1 = winW * 0.25f;
        float col2 = winW * 0.50f;
        float col3 = winW * 0.75f;

        delete knobCps;
        knobCps = new CpsKnob(mHWnd, mGfx, col1, knobCy, knobR);
        knobCps->SetValue(18);

        delete knobCooldown;
        knobCooldown = new CooldownKnob(mHWnd, mGfx, col2, knobCy, knobR);
        knobCooldown->SetValue(1.0f);

        delete knobTriggerCooldown;
        knobTriggerCooldown = new TriggerCooldownKnob(mHWnd, mGfx, col3, knobCy, knobR);
        knobTriggerCooldown->SetValue(4.0f);

        static constexpr float kPidBoxSize = 28.0f;
        static constexpr float kPidGap = 6.0f;
        static constexpr int   kPidCount = 6;
        float pidTotalW = kPidCount * kPidBoxSize + (kPidCount - 1) * kPidGap;
        float pidX = (winW - pidTotalW) * 0.5f;
        float pidY = 158.0f;

        delete pidInput;
        pidInput = new PidInput(mHWnd, mGfx, pidX, pidY);

        static constexpr float kComboW = 148.0f;
        static constexpr float kComboPair = kComboW * 2.0f + 20.0f;
        float ksX = (winW - kComboPair) * 0.5f;
        float ksY = 225.0f;

        delete keySelector;
        keySelector = new KeySelector(mHWnd, mGfx, ksX, ksY, kComboW);
    }
}

void Renderer::Render()
{
    this->BaseRender();
    RECT rect;
    GetClientRect(mHWnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    int x = w / 2;
    int y = h / 2;

    mGfx->BeginDraw();
    mGfx->drawCircle((float)x, (float)y,
                     (float)(w / 2), (float)(h / 2),
                     D2D1::ColorF::Crimson);
    mGfx->EndDraw();
}

void Renderer::BaseRender()
{
    if (!mGfx) mGfx = new Graphics(mHWnd);
    mGfx->Init();
}

HWND Renderer::getCurrentStage()
{
    return mHWnd;
}

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
    if (keySelector)         keySelector->Render();

    mGfx->EndDraw();
}

void Renderer::OnMouseMove(float mx, float my)
{
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
    if (keySelector && keySelector->OnMouseDown(mx, my))
        return;

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
    if (keySelector && keySelector->OnMouseUp(mx, my))
        return;

    if (btnInject)           btnInject->OnMouseUp(mx, my);
    if (chkDebugPanel)       chkDebugPanel->OnMouseUp(mx, my);
    if (chkControlDialog)    chkControlDialog->OnMouseUp(mx, my);
    if (knobCps)             knobCps->OnMouseUp(mx, my);
    if (knobCooldown)        knobCooldown->OnMouseUp(mx, my);
    if (knobTriggerCooldown) knobTriggerCooldown->OnMouseUp(mx, my);
}

void Renderer::OnMouseLeave()
{
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
    if (keySelector) keySelector->OnMouseWheel(mx, my, delta);
}

void Renderer::OnChar(wchar_t ch)
{
    if (pidInput) pidInput->OnChar(ch);
}

void Renderer::OnKeyDown(int vk)
{
    if (pidInput) pidInput->OnKeyDown(vk);
}

void Renderer::OnCommand(WPARAM wParam, LPARAM lParam)
{
    (void)wParam; (void)lParam;
}

#pragma warning(pop)