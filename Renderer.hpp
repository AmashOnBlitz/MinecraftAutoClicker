#ifndef RENDERER_CLASS_HEADER
#define RENDERER_CLASS_HEADER

#include <Windows.h>
#include "Graphics.hpp"
#include "Button.hpp"
#include "Checkbox.hpp"
#include "knob.hpp"
#include "PidInput.hpp"
#include "KeySelector.hpp"
#include "CustomDropdown.hpp"

class Renderer {
public:
    Renderer()
        : mHWnd(nullptr),
        mGfx(nullptr),
        btnInject(nullptr),
        chkDebugPanel(nullptr),
        chkControlDialog(nullptr),
        knobCps(nullptr),
        knobCooldown(nullptr),
        knobTriggerCooldown(nullptr),
        pidInput(nullptr),
        keySelector(nullptr),
        ddDebugToggleKey(nullptr),
        ddControlToggleKey(nullptr),
        mMouseOwner(MouseOwner::None)
    {
    }

    ~Renderer() {
        delete mGfx;
        delete pidInput;
        delete keySelector;
        delete ddDebugToggleKey;
        delete ddControlToggleKey;
    }

    virtual void SetStage(HWND hWnd);
    virtual void Render();
    virtual void BaseRender() final;

    HWND      getCurrentStage();
    Graphics* GetGraphics() { return mGfx; }

    void OnMouseMove(float mx, float my);
    void OnMouseDown(float mx, float my);
    void OnMouseUp(float mx, float my);
    void OnMouseLeave();
    void OnMouseWheel(float mx, float my, int delta);
    void OnChar(wchar_t ch);
    void OnKeyDown(int vk);
    void OnCommand(WPARAM wParam, LPARAM lParam);

public:
    BottomPaddedButton* btnInject;
    FixedCheckbox* chkDebugPanel;
    FixedCheckbox* chkControlDialog;
    CpsKnob* knobCps;
    CooldownKnob* knobCooldown;
    TriggerCooldownKnob* knobTriggerCooldown;
    PidInput* pidInput;
    KeySelector* keySelector;
    CustomDropdown* ddDebugToggleKey;
    CustomDropdown* ddControlToggleKey;

protected:
    HWND      mHWnd;
    Graphics* mGfx;

private:
    enum class MouseOwner {
        None,
        KeySelector,
        DebugToggleDrop,
        ControlToggleDrop,
        Other         
    };
    MouseOwner mMouseOwner;
};

class MainWindowRenderer : public Renderer {
public:
    void Render() override;
};

#endif