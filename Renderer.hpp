#ifndef RENDERER_CLASS_HEADER
#define RENDERER_CLASS_HEADER

#include <Windows.h>
#include "Graphics.hpp"

class Renderer {
public:
    Renderer() : mHWnd(nullptr), mGfx(nullptr) {}
    ~Renderer() { delete mGfx; }
    virtual void SetStage(HWND hWnd);
    virtual void Render();
    virtual void BaseRender() final;
    HWND getCurrentStage();
    Graphics* GetGraphics(){return mGfx;}
protected:
    HWND mHWnd;
    Graphics* mGfx;
};

class MainWindowRenderer : public Renderer {
public:
    void Render() override;
};
#endif //!RENDERER_CLASS_HEADER