# 🖱️ Minecraft Auto Clicker

> A DLL injector + auto-clicker built as a Win32/Direct2D learning project.  
> Injects into a running Minecraft process and clicks so you don't have to — with *just* enough randomness to feel human.

---

## ✨ What It Does

Once injected, the DLL spawns a few background threads that handle everything:

- **Auto-clicking** — left or right click at a configurable CPS, with jittered delays to avoid detection
- **Debug overlay** — a transparent HUD showing live CPS, average CPS, and expected CPS
- **Control panel** — a floating in-game panel (Direct2D rendered) to tune settings on the fly without re-injecting

All config is persisted to a binary file in `%TEMP%` so your settings survive across sessions.

---

## 🔧 Features

| Thing | Details |
|---|---|
| CPS range | 10 – 22 (configurable per session) |
| Click modes | Left click, right click (mutually exclusive toggle) |
| Humanization | Jittered delays, occasional micro-bursts, drift simulation |
| Trigger cooldown | Configurable time after which cooldown break triggers |
| Cooldown period | Configurable break between click sessions to avoid suspicion |
| Key binds | F1–F12 + Middle Mouse, fully remappable in-app |
| UI | Custom Direct2D widgets — sliders, knobs, dropdowns, checkboxes |

---

## 🏗️ Project Structure

Everything lives flat in the solution root — that's intentional for Visual Studio filter organization. Open it in **VS** and the filters will make the structure obvious.

```
Addon.dll       ← the injected payload (auto-clicker logic + UI)
main.cpp        ← the injector host app (Win32 window, DLL injection)
Graphics.cpp/h  ← thin Direct2D wrapper
Renderer.cpp/h  ← main window rendering + widget layout
ControlPanel.*  ← in-game floating control panel
Slider.*        ← custom slider widget
Knob.*          ← rotary knob widget
Button.*        ← button widget (normal + bottom-padded variants)
Checkbox.*      ← checkbox widget
CustomDropdown.*← scrollable dropdown
KeySelector.*   ← paired F-key picker
PidInput.*      ← digit-box PID entry widget
Config.hpp      ← flat binary config r/w to %TEMP%
```

---

## 🚀 Building

Requires **Visual Studio** with the Windows SDK and Direct2D/DWrite headers (included in the default Desktop development workload).

1. Clone the repo
2. Open the `.sln` in Visual Studio
3. Build `Release | x64`
4. Make sure `Addon.dll` ends up next to the injector exe

---

## 🎮 Usage

1. Start Minecraft
2. Open Task Manager → find the Minecraft PID
3. Launch the injector, enter the PID into the digit boxes
4. Configure CPS, cooldowns, and key binds
5. Hit **Inject**
6. Back in-game, press your configured toggle key to start/stop clicking

The control panel and debug overlay can be toggled at any time with their assigned hotkeys (defaults: `F11` / `F12`).

---

## ⚠️ Disclaimers

This is a **personal learning project** exploring Win32 API, Direct2D, DLL injection, and thread synchronization. It is not maintained, not production-ready, and not intended for competitive play.

- The author takes no responsibility for bans, account issues, or any other consequences from use
- Anti-cheat software (EAC, VAC, etc.) may detect injection regardless of click humanization
- Use at your own risk, on your own accounts, in environments where it's permitted

---

*Built to learn — not to grief.*