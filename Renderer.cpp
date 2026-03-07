#include "renderer.hpp"

#pragma warning(push)
#pragma warning(disable : 4244) 

void Renderer::SetStage(HWND hWnd)
{
    if (mHWnd != hWnd) mHWnd = hWnd;
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
    mGfx->EndDraw();
}

#pragma warning(pop)