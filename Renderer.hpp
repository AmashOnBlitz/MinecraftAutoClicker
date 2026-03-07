#ifndef RENDERER_CLASS_HEADER
#define RENDERER_CLASS_HEADER

#include <Windows.h>
#include "Graphics.hpp"
#include "Button.hpp"

class Renderer {
public:
    Renderer() 
        : mHWnd(nullptr), 
        mGfx(nullptr),
        btnInject(nullptr)
    {
    }
    ~Renderer() { delete mGfx; }
    virtual void SetStage(HWND hWnd);
    virtual void Render();
    virtual void BaseRender() final;
    HWND getCurrentStage();
    Graphics* GetGraphics(){return mGfx;}
    void OnMouseMove(float mx, float my);
    void OnMouseDown(float mx, float my);
    void OnMouseUp(float mx, float my);
    void OnMouseLeave();
public:
    BottomPaddedButton* btnInject;
protected:
    HWND mHWnd;
    Graphics* mGfx;
};

class MainWindowRenderer : public Renderer {
public:
    void Render() override;
};
#endif //!RENDERER_CLASS_HEADER