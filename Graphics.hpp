#ifndef D2D_GRAPHICS_HEADER
#define D2D_GRAPHICS_HEADER

#include <d2d1.h>
#include <string>
#include "windows.h"
#include <dwrite.h>

class Graphics final {
public:
    Graphics(HWND hwnd);
    ~Graphics();
    bool Init();
    bool IsInit();
    void BeginDraw();
    void EndDraw();
    void Resize(UINT w, UINT h);
    void ClearScreen(float r, float g, float b, float a = 1.0f);
    void ClearScreen(D2D1::ColorF color);
    void drawCircle(float x, float y, float xradius, float yradius, float r, float g, float b, float a, float strokeW = 1.0f);
    void drawCircle(float x, float y, float xradius, float yradius, D2D1::ColorF color, float strokeW = 1.0f);
    void FillCircle(float cx, float cy, float r, D2D1::ColorF color);
    void DrawArc(float cx, float cy, float r, float startAngleDeg, float sweepDeg, D2D1::ColorF color, float strokeW = 2.0f);
    void FillRoundedRect(float x, float y, float w, float h, float radius, D2D1::ColorF color);
    void DrawTextCentered(const std::wstring& text, float x, float y, float w, float h, D2D1::ColorF color, float fontSize = 14.0f);
    void SetAliased(bool aliased);
    void DrawRoundedRect(float x, float y, float w, float h, float radius, D2D1::ColorF color, float strokeW = 1.0f);
    void DrawTextLeft(const std::wstring& text, float x, float y, float w, float h, D2D1::ColorF color, float fontSize = 14.0f);
    void DrawLine(float x1, float y1, float x2, float y2, D2D1::ColorF color, float strokeW);
private: 
    bool setInit(bool b);
    template <typename t>
    void ReleaseID2D1Object(t* object) {
        if (object) object->Release();
        object = nullptr;
    }
private:
    bool misInit = false;
    HWND mHwnd;
    ID2D1Factory* mpfact;
    ID2D1HwndRenderTarget* mpRend;
    ID2D1SolidColorBrush* mpBrush;
    IDWriteFactory* mpDWriteFactory;
};

#endif //!D2D_GRAPHICS_HEADER