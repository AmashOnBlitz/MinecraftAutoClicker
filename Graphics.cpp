#include "Graphics.hpp"
#include "dwrite.h"
#include <string>
#include "windows.h"

#define CHECK_HR_OK if (FAILED(hr)) return setInit(false);
#define GUARD_NO_INIT if (!misInit) return;
static bool isFactInit = false;

#pragma region Initializers 

Graphics::Graphics(HWND hwnd)
    : mHwnd(hwnd),
    mpfact(nullptr),
    mpRend(nullptr),
    mpBrush(nullptr),
    misInit(false)
{
}

Graphics::~Graphics()
{
    ReleaseID2D1Object(mpfact);
    ReleaseID2D1Object(mpRend);
}

bool Graphics::Init()
{
    if (misInit) return true;
    HRESULT hr;
    if (!isFactInit) {
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &mpfact);
        CHECK_HR_OK;
    }

    RECT rect;
    GetClientRect(mHwnd, &rect);
    hr = mpfact->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(mHwnd, D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top)),
        &mpRend
    );
    CHECK_HR_OK;

    hr = mpRend->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &mpBrush);
    CHECK_HR_OK;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&mpDWriteFactory)
    );
    CHECK_HR_OK;

    return setInit(true);
}

bool Graphics::IsInit()
{
    return misInit;
}

#pragma endregion

#pragma region Draw Functions

void Graphics::BeginDraw()
{
    GUARD_NO_INIT;
    mpRend->BeginDraw();
    mpRend->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    mpRend->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
}

void Graphics::EndDraw()
{
    GUARD_NO_INIT;
    mpRend->EndDraw();
}

void Graphics::Resize(UINT w, UINT h)
{
    GUARD_NO_INIT;
    if (mpRend) mpRend->Resize(D2D1::SizeU(w, h));
}

void Graphics::ClearScreen(float r, float g, float b, float a)
{
    GUARD_NO_INIT;
    mpRend->Clear(D2D1::ColorF(r, g, b, a));
}

void Graphics::ClearScreen(D2D1::ColorF color)
{
    GUARD_NO_INIT;
    mpRend->Clear(D2D1::ColorF(color));
}

void Graphics::drawCircle(float x, float y, float radiusX, float radiusY,
                          float r, float g, float b, float a, float strokeW)
{
    GUARD_NO_INIT;
    mpBrush->SetColor(D2D1::ColorF(r, g, b, a));
    mpRend->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radiusX, radiusY), mpBrush, strokeW);
}

void Graphics::drawCircle(float x, float y, float radiusX, float radiusY,
                          D2D1::ColorF color, float strokeW)
{
    GUARD_NO_INIT;
    mpBrush->SetColor(D2D1::ColorF(color));
    mpRend->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radiusX, radiusY), mpBrush, strokeW);
}

void Graphics::FillRoundedRect(float x, float y, float w, float h,
                               float radius, D2D1::ColorF color)
{
    GUARD_NO_INIT;
    mpBrush->SetColor(color);
    D2D1_ROUNDED_RECT rect = D2D1::RoundedRect(
        D2D1::RectF(x, y, x + w, y + h),
        radius, radius
    );
    mpRend->FillRoundedRectangle(rect, mpBrush);
}

void Graphics::DrawRoundedRect(float x, float y, float w, float h,
                               float radius, D2D1::ColorF color, float strokeW)
{
    GUARD_NO_INIT;
    mpBrush->SetColor(color);
    D2D1_ROUNDED_RECT rect = D2D1::RoundedRect(
        D2D1::RectF(x, y, x + w, y + h),
        radius, radius
    );
    mpRend->DrawRoundedRectangle(rect, mpBrush, strokeW);
}

void Graphics::DrawLine(float x1, float y1, float x2, float y2, D2D1::ColorF color, float strokeW){
    GUARD_NO_INIT;
    mpBrush->SetColor(color);
    mpRend->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2),
        mpBrush,
        strokeW
    );
}

#pragma endregion

#pragma region Private Functions

bool Graphics::setInit(bool b)
{
    misInit = b;
    return b;
}

static void DrawTextInternal(
    ID2D1HwndRenderTarget* pRend,
    IDWriteFactory* pDWrite,
    ID2D1SolidColorBrush* pBrush,
    const std::wstring& text,
    float x, float y, float w, float h,
    D2D1::ColorF color,
    float fontSize,
    DWRITE_TEXT_ALIGNMENT hAlign)
{
    IDWriteTextFormat* pFmt = nullptr;
    HRESULT hr = pDWrite->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize, L"en-us", &pFmt
    );
    if (FAILED(hr) || !pFmt) return;

    pFmt->SetTextAlignment(hAlign);
    pFmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    pBrush->SetColor(color);
    pRend->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        pFmt,
        D2D1::RectF(x, y, x + w, y + h),
        pBrush
    );
    pFmt->Release();
}

void Graphics::DrawTextCentered(const std::wstring& text,
                                float x, float y, float w, float h,
                                D2D1::ColorF color, float fontSize)
{
    GUARD_NO_INIT;
    DrawTextInternal(mpRend, mpDWriteFactory, mpBrush,
                     text, x, y, w, h, color, fontSize,
                     DWRITE_TEXT_ALIGNMENT_CENTER);
}

void Graphics::DrawTextLeft(const std::wstring& text,
                            float x, float y, float w, float h,
                            D2D1::ColorF color, float fontSize)
{
    GUARD_NO_INIT;
    DrawTextInternal(mpRend, mpDWriteFactory, mpBrush,
                     text, x, y, w, h, color, fontSize,
                     DWRITE_TEXT_ALIGNMENT_LEADING);
}

void Graphics::SetAliased(bool aliased)
{
    GUARD_NO_INIT;
    mpRend->SetAntialiasMode(aliased
                             ? D2D1_ANTIALIAS_MODE_ALIASED
                             : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

#pragma endregion