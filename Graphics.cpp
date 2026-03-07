#include "Graphics.hpp"
#include "dwrite.h"
#include <string>
#include "windows.h"
#include <cmath>

#define CHECK_HR_OK if (FAILED(hr)) return setInit(false);
#define GUARD_NO_INIT if (!misInit) return;
static bool isFactInit = false;

static constexpr float kGfxPi = 3.14159265358979323846f;

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

void Graphics::FillCircle(float cx, float cy, float r, D2D1::ColorF color)
{
    GUARD_NO_INIT;
    mpBrush->SetColor(color);
    mpRend->FillEllipse(
        D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r),
        mpBrush
    );
}

void Graphics::DrawArc(float cx, float cy, float r,
                       float startAngleDeg, float sweepDeg,
                       D2D1::ColorF color, float strokeW)
{
    GUARD_NO_INIT;

    ID2D1PathGeometry* pPath = nullptr;
    HRESULT hr = mpfact->CreatePathGeometry(&pPath);
    if (FAILED(hr) || !pPath) return;

    ID2D1GeometrySink* pSink = nullptr;
    hr = pPath->Open(&pSink);
    if (FAILED(hr) || !pSink) { pPath->Release(); return; }

    float startRad = startAngleDeg * kGfxPi / 180.0f;
    float endRad = (startAngleDeg + sweepDeg) * kGfxPi / 180.0f;

    D2D1_POINT_2F startPt = D2D1::Point2F(
        cx + r * std::cosf(startRad),
        cy + r * std::sinf(startRad));

    D2D1_POINT_2F endPt = D2D1::Point2F(
        cx + r * std::cosf(endRad),
        cy + r * std::sinf(endRad));

    pSink->BeginFigure(startPt, D2D1_FIGURE_BEGIN_HOLLOW);
    pSink->AddArc(D2D1::ArcSegment(
        endPt,
        D2D1::SizeF(r, r),
        0.0f,
        D2D1_SWEEP_DIRECTION_CLOCKWISE,
        sweepDeg > 180.0f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
    ));
    pSink->EndFigure(D2D1_FIGURE_END_OPEN);
    pSink->Close();

    mpBrush->SetColor(color);
    mpRend->DrawGeometry(pPath, mpBrush, strokeW);

    pSink->Release();
    pPath->Release();
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

void Graphics::DrawLine(float x1, float y1, float x2, float y2, D2D1::ColorF color, float strokeW) {
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