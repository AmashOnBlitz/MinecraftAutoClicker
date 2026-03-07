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
        const float kTopStart = 100.0f;
        const float kRowStride = 30.0f;   
        const float kTextWidth = 200.0f;
        float btnX, btnY, btnW, btnH;
        btnInject->GetRect(btnX, btnY, btnW, btnH);


        delete chkDebugPanel;
        chkDebugPanel = new FixedCheckbox(
            mHWnd, mGfx,
            L"Add Debug Panel",
            kLeft, btnY - (2*kRowStride),
            kTextWidth
        );
        chkDebugPanel->SetChecked(true);

        delete chkControlDialog;
        chkControlDialog = new FixedCheckbox(
            mHWnd, mGfx,
            L"Show Control Dialog",
            kLeft, btnY - kRowStride,
            kTextWidth
        );
        chkControlDialog->SetChecked(true);

        RECT rc;
        GetClientRect(mHWnd, &rc);
        float winW = static_cast<float>(rc.right - rc.left);
        float knobR = 36.0f;
        float knobCx = winW * 0.5f;
        float knobCy = 70.0f;           
        delete knobCps;
        knobCps = new CpsKnob(mHWnd, mGfx, knobCx, knobCy, knobR);
        knobCps->SetValue(18);
    }
}

void Renderer::Render()
{
    this->BaseRender();
    RECT rect;
    GetClientRect(mHWnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    int x = (int)(w / 2);
    int y = (int)(h / 2);

    mGfx->BeginDraw();
    mGfx->drawCircle(x, y, w / 2, h / 2, D2D1::ColorF::Crimson);
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
    if (knobCps)        knobCps->render();
    if (chkDebugPanel)  chkDebugPanel->render();
    if (chkControlDialog) chkControlDialog->render();
    if (btnInject) btnInject->render();
    mGfx->EndDraw();
}

void Renderer::OnMouseMove(float mx, float my) {
    if (btnInject)     btnInject->OnMouseMove(mx, my);
    if (chkDebugPanel)  chkDebugPanel->OnMouseMove(mx, my);
    if (chkControlDialog) chkControlDialog->OnMouseMove(mx, my);
    if (knobCps)        knobCps->OnMouseMove(mx, my);
}

void Renderer::OnMouseDown(float mx, float my) {
    if (btnInject)     btnInject->OnMouseDown(mx, my);
    if (chkDebugPanel)  chkDebugPanel->OnMouseDown(mx, my);
    if (chkControlDialog) chkControlDialog->OnMouseDown(mx, my);
    if (knobCps)        knobCps->OnMouseDown(mx, my);
}

void Renderer::OnMouseUp(float mx, float my) {
    if (btnInject)     btnInject->OnMouseUp(mx, my);
    if (chkDebugPanel)  chkDebugPanel->OnMouseUp(mx, my);
    if (chkControlDialog) chkControlDialog->OnMouseUp(mx, my);
    if (knobCps)        knobCps->OnMouseUp(mx, my);
}

void Renderer::OnMouseLeave() {
    if (btnInject)     btnInject->OnMouseLeave();
    if (chkDebugPanel)  chkDebugPanel->OnMouseLeave();
    if (chkControlDialog) chkControlDialog->OnMouseLeave();
    if (knobCps)        knobCps->OnMouseLeave();
}

#pragma warning(pop)