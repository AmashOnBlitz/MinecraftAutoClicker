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

        delete chkDebugDialog;
        chkDebugDialog = new FixedCheckbox(
            mHWnd, mGfx,
            L"Show Debug Dialog",
            kLeft, btnY - kRowStride,
            kTextWidth
        );
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
    if (chkDebugPanel)  chkDebugPanel->render();
    if (chkDebugDialog) chkDebugDialog->render();
    if (btnInject) btnInject->render();
    mGfx->EndDraw();
}

void Renderer::OnMouseMove(float mx, float my) {
    if (btnInject)     btnInject->OnMouseMove(mx, my);
    if (chkDebugPanel)  chkDebugPanel->OnMouseMove(mx, my);
    if (chkDebugDialog) chkDebugDialog->OnMouseMove(mx, my);
}

void Renderer::OnMouseDown(float mx, float my) {
    if (btnInject)     btnInject->OnMouseDown(mx, my);
    if (chkDebugPanel)  chkDebugPanel->OnMouseDown(mx, my);
    if (chkDebugDialog) chkDebugDialog->OnMouseDown(mx, my);
}

void Renderer::OnMouseUp(float mx, float my) {
    if (btnInject)     btnInject->OnMouseUp(mx, my);
    if (chkDebugPanel)  chkDebugPanel->OnMouseUp(mx, my);
    if (chkDebugDialog) chkDebugDialog->OnMouseUp(mx, my);
}

void Renderer::OnMouseLeave() {
    if (btnInject)     btnInject->OnMouseLeave();
    if (chkDebugPanel)  chkDebugPanel->OnMouseLeave();
    if (chkDebugDialog) chkDebugDialog->OnMouseLeave();
}

#pragma warning(pop)