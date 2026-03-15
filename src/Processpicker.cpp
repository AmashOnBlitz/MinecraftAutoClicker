#include "pch.h"
#include "ProcessPicker.hpp"
#include <tlhelp32.h>
#include <algorithm>
#include <cwctype>
#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#define IDC_PP_FILTER  101
#define IDC_PP_LIST    102
#define IDC_PP_SELECT  103
#define IDC_PP_CANCEL  104

namespace Pal {
    constexpr COLORREF BG = RGB(18, 20, 32);
    constexpr COLORREF SURFACE = RGB(26, 28, 42);
    constexpr COLORREF INPUT_BG = RGB(14, 16, 26);
    constexpr COLORREF ROW_EVEN = RGB(18, 20, 32);
    constexpr COLORREF ROW_ODD = RGB(22, 24, 38);
    constexpr COLORREF ROW_HOVER = RGB(28, 32, 52);
    constexpr COLORREF SEL_BG = RGB(42, 74, 138);
    constexpr COLORREF SEL_TXT = RGB(220, 235, 255);
    constexpr COLORREF SEL_DIM = RGB(160, 195, 255);
    constexpr COLORREF BORDER = RGB(48, 52, 78);
    constexpr COLORREF BORDER_FOC = RGB(72, 120, 200);
    constexpr COLORREF TEXT = RGB(205, 208, 228);
    constexpr COLORREF DIM = RGB(100, 106, 148);
    constexpr COLORREF ACCENT = RGB(74, 130, 218);
    constexpr COLORREF ACCENT_H = RGB(96, 152, 240);
    constexpr COLORREF ACCENT_P = RGB(56, 106, 188);
    constexpr COLORREF CNL_BG = RGB(36, 38, 58);
    constexpr COLORREF CNL_H = RGB(48, 52, 76);
    constexpr COLORREF CNL_P = RGB(28, 30, 46);
    constexpr COLORREF DIVIDER = RGB(38, 42, 64);
    constexpr COLORREF SCROLLBAR = RGB(42, 46, 70);
}

namespace Lay {
    constexpr int CW = 420;
    constexpr int CH = 394;

    constexpr int PAD = 14;
    constexpr int HDR_H = 46;

    constexpr int LBL_X = PAD;
    constexpr int LBL_Y = (HDR_H - 20) / 2;
    constexpr int LBL_W = 58;
    constexpr int ED_X = LBL_X + LBL_W + 6;
    constexpr int ED_W = CW - ED_X - PAD;
    constexpr int ED_Y = (HDR_H - 24) / 2;
    constexpr int ED_H = 24;

    constexpr int LST_X = 0;
    constexpr int LST_Y = HDR_H;
    constexpr int LST_W = CW;
    constexpr int LST_H = 300;

    constexpr int FTR_Y = LST_Y + LST_H;
    constexpr int FTR_H = CH - FTR_Y;

    constexpr int BTN_H = 28;
    constexpr int SEL_W = 92;
    constexpr int CNL_W = 80;
    constexpr int BTN_Y = FTR_Y + (FTR_H - BTN_H) / 2;
    constexpr int SEL_X = CW - PAD - SEL_W;
    constexpr int CNL_X = SEL_X - 8 - CNL_W;
}

static std::vector<ProcessEntry> SnapProcesses()
{
    std::vector<ProcessEntry> out;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    PROCESSENTRY32W pe{ sizeof(pe) };
    if (Process32FirstW(snap, &pe))
        do { out.push_back({ pe.th32ProcessID, pe.szExeFile }); } while (Process32NextW(snap, &pe));
    CloseHandle(snap);
    std::sort(out.begin(), out.end(), [](const ProcessEntry& a, const ProcessEntry& b) {
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    return out;
}

struct PickerState {
    std::vector<ProcessEntry> all;
    std::vector<ProcessEntry> shown;
    DWORD  result = 0;
    bool   done = false;
    bool   ok = false;
    bool   editFocus = false;
    HFONT  hUiFont = nullptr;
    HFONT  hMono = nullptr;
    HBRUSH hBrBg = nullptr;
    HBRUSH hBrSurf = nullptr;
    HBRUSH hBrInput = nullptr;
};

static LRESULT CALLBACK BtnSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto orig = reinterpret_cast<WNDPROC>(GetPropW(hwnd, L"_OP"));

    switch (msg) {
    case WM_MOUSEMOVE:
        if (!GetPropW(hwnd, L"_Hot")) {
            SetPropW(hwnd, L"_Hot", (HANDLE)TRUE);
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        RemovePropW(hwnd, L"_Hot");
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    }
    return CallWindowProcW(orig, hwnd, msg, wParam, lParam);
}

static void SubclassBtn(HWND hwnd)
{
    auto orig = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(BtnSubProc)));
    SetPropW(hwnd, L"_OP", reinterpret_cast<HANDLE>(orig));
}

static void RebuildList(HWND hList, PickerState* s, const wchar_t* filter)
{
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    s->shown.clear();
    std::wstring lo = filter;
    std::transform(lo.begin(), lo.end(), lo.begin(), ::towlower);
    for (auto& e : s->all) {
        if (!lo.empty()) {
            std::wstring nm = e.name;
            std::transform(nm.begin(), nm.end(), nm.begin(), ::towlower);
            if (nm.find(lo) == std::wstring::npos) continue;
        }
        s->shown.push_back(e);
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)e.name.c_str());
    }
}

static void CommitSelection(HWND hwnd, PickerState* s)
{
    int sel = (int)SendDlgItemMessage(hwnd, IDC_PP_LIST, LB_GETCURSEL, 0, 0);
    if (sel >= 0 && sel < (int)s->shown.size()) {
        s->result = s->shown[sel].pid;
        s->ok = true;
    }
    s->done = true;
    DestroyWindow(hwnd);
}

static void FillBorder(HDC dc, const RECT& rc, COLORREF fill, COLORREF border)
{
    HBRUSH br = CreateSolidBrush(fill);
    FillRect(dc, &rc, br);
    DeleteObject(br);

    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HPEN old = (HPEN)SelectObject(dc, pen);
    HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH ob = (HBRUSH)SelectObject(dc, nb);
    Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(dc, old);
    SelectObject(dc, ob);
    DeleteObject(pen);
}

static LRESULT CALLBACK PickerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PickerState* s = reinterpret_cast<PickerState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE:
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        s = reinterpret_cast<PickerState*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
        HINSTANCE hI = cs->hInstance;

        s->hUiFont = CreateFontW(
            14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        s->hMono = CreateFontW(
            13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Cascadia Mono");

        s->hBrBg = CreateSolidBrush(Pal::BG);
        s->hBrSurf = CreateSolidBrush(Pal::SURFACE);
        s->hBrInput = CreateSolidBrush(Pal::INPUT_BG);

        HWND hLbl = CreateWindowW(L"STATIC", L"Filter",
                                  WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                                  Lay::LBL_X, Lay::LBL_Y, Lay::LBL_W, 20,
                                  hwnd, nullptr, hI, nullptr);
        SendMessage(hLbl, WM_SETFONT, (WPARAM)s->hUiFont, FALSE);

        HWND hEd = CreateWindowW(L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                 Lay::ED_X, Lay::ED_Y, Lay::ED_W, Lay::ED_H,
                                 hwnd, (HMENU)(UINT_PTR)IDC_PP_FILTER, hI, nullptr);
        SendMessage(hEd, WM_SETFONT, (WPARAM)s->hUiFont, FALSE);
        SetWindowTheme(hEd, L"", L"");

        HWND hList = CreateWindowW(L"LISTBOX", nullptr,
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                   LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                                   LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
                                   Lay::LST_X, Lay::LST_Y, Lay::LST_W, Lay::LST_H,
                                   hwnd, (HMENU)(UINT_PTR)IDC_PP_LIST, hI, nullptr);
        SendMessage(hList, WM_SETFONT, (WPARAM)s->hMono, FALSE);
        SetWindowTheme(hList, L"", L"");

        HWND hCnl = CreateWindowW(L"BUTTON", L"Cancel",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  Lay::CNL_X, Lay::BTN_Y, Lay::CNL_W, Lay::BTN_H,
                                  hwnd, (HMENU)(UINT_PTR)IDC_PP_CANCEL, hI, nullptr);
        SendMessage(hCnl, WM_SETFONT, (WPARAM)s->hUiFont, FALSE);
        SubclassBtn(hCnl);

        HWND hSel = CreateWindowW(L"BUTTON", L"Select",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  Lay::SEL_X, Lay::BTN_Y, Lay::SEL_W, Lay::BTN_H,
                                  hwnd, (HMENU)(UINT_PTR)IDC_PP_SELECT, hI, nullptr);
        SendMessage(hSel, WM_SETFONT, (WPARAM)s->hUiFont, FALSE);
        SubclassBtn(hSel);

        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));

        RebuildList(hList, s, L"");
        SetFocus(hEd);
        return 0;
    }

    case WM_ERASEBKGND:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        RECT rc; GetClientRect(hwnd, &rc);
        FillRect(dc, &rc, s->hBrBg);
        return 1;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);

        {
            RECT hdr{ 0, 0, Lay::CW, Lay::HDR_H };
            FillRect(dc, &hdr, s->hBrSurf);
            HPEN p = CreatePen(PS_SOLID, 1, Pal::BORDER);
            HPEN op = (HPEN)SelectObject(dc, p);
            MoveToEx(dc, 0, Lay::HDR_H - 1, nullptr);
            LineTo(dc, Lay::CW, Lay::HDR_H - 1);
            SelectObject(dc, op); DeleteObject(p);
        }

        {
            COLORREF bCol = s->editFocus ? Pal::BORDER_FOC : Pal::BORDER;
            RECT erc{
                Lay::ED_X - 2,
                Lay::ED_Y - 2,
                Lay::ED_X + Lay::ED_W + 2,
                Lay::ED_Y + Lay::ED_H + 2
            };
            HPEN p = CreatePen(PS_SOLID, 1, bCol);
            HPEN op = (HPEN)SelectObject(dc, p);
            HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH ob = (HBRUSH)SelectObject(dc, nb);
            Rectangle(dc, erc.left, erc.top, erc.right, erc.bottom);
            SelectObject(dc, op); SelectObject(dc, ob);
            DeleteObject(p);
        }

        {
            RECT ftr{ 0, Lay::FTR_Y, Lay::CW, Lay::CH };
            FillRect(dc, &ftr, s->hBrSurf);
            HPEN p = CreatePen(PS_SOLID, 1, Pal::BORDER);
            HPEN op = (HPEN)SelectObject(dc, p);
            MoveToEx(dc, 0, Lay::FTR_Y, nullptr);
            LineTo(dc, Lay::CW, Lay::FTR_Y);
            SelectObject(dc, op); DeleteObject(p);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, Pal::DIM);
        return reinterpret_cast<LRESULT>(s->hBrSurf);
    }

    case WM_CTLCOLOREDIT:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkColor(dc, Pal::INPUT_BG);
        SetTextColor(dc, Pal::TEXT);
        return reinterpret_cast<LRESULT>(s->hBrInput);
    }

    case WM_CTLCOLORLISTBOX:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkColor(dc, Pal::INPUT_BG);
        SetTextColor(dc, Pal::TEXT);
        return reinterpret_cast<LRESULT>(s->hBrInput);
    }

    case WM_MEASUREITEM:
    {
        auto* mi = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (mi->CtlID == IDC_PP_LIST) mi->itemHeight = 22;
        return TRUE;
    }

    case WM_DRAWITEM:
    {
        auto* di = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);

        if (di->CtlType == ODT_LISTBOX) {
            if (di->itemID == (UINT)-1) return TRUE;

            PickerState* st = reinterpret_cast<PickerState*>(
                GetWindowLongPtrW(GetParent(di->hwndItem), GWLP_USERDATA));
            if (!st || di->itemID >= (UINT)st->shown.size()) return TRUE;

            const ProcessEntry& e = st->shown[di->itemID];
            bool sel = (di->itemState & ODS_SELECTED) != 0;
            bool alt = (di->itemID & 1) != 0;

            COLORREF bg = sel ? Pal::SEL_BG : (alt ? Pal::ROW_ODD : Pal::ROW_EVEN);
            HBRUSH br = CreateSolidBrush(bg);
            FillRect(di->hDC, &di->rcItem, br);
            DeleteObject(br);

            if (!sel) {
                HPEN dp = CreatePen(PS_SOLID, 1, Pal::DIVIDER);
                HPEN odp = (HPEN)SelectObject(di->hDC, dp);
                MoveToEx(di->hDC, di->rcItem.left, di->rcItem.bottom - 1, nullptr);
                LineTo(di->hDC, di->rcItem.right, di->rcItem.bottom - 1);
                SelectObject(di->hDC, odp); DeleteObject(dp);
            }

            if (sel) {
                RECT bar{ di->rcItem.left, di->rcItem.top,
                          di->rcItem.left + 3, di->rcItem.bottom };
                HBRUSH ab = CreateSolidBrush(Pal::ACCENT_H);
                FillRect(di->hDC, &bar, ab);
                DeleteObject(ab);
            }

            HFONT hF = (HFONT)SendMessage(di->hwndItem, WM_GETFONT, 0, 0);
            HFONT oldF = hF ? (HFONT)SelectObject(di->hDC, hF) : nullptr;
            SetBkMode(di->hDC, TRANSPARENT);

            RECT nr = di->rcItem;
            nr.left += (sel ? 10 : 8);
            nr.right = di->rcItem.right - 70;
            SetTextColor(di->hDC, sel ? Pal::SEL_TXT : Pal::TEXT);
            DrawTextW(di->hDC, e.name.c_str(), -1, &nr,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

            wchar_t pidStr[24];
            swprintf_s(pidStr, L"%lu", (unsigned long)e.pid);
            RECT pr = di->rcItem;
            pr.right -= 10;
            SetTextColor(di->hDC, sel ? Pal::SEL_DIM : Pal::DIM);
            DrawTextW(di->hDC, pidStr, -1, &pr,
                      DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

            if (oldF) SelectObject(di->hDC, oldF);
            return TRUE;
        }

        if (di->CtlType == ODT_BUTTON) {
            bool accent = (di->CtlID == IDC_PP_SELECT);
            bool pressed = (di->itemState & ODS_SELECTED) != 0;
            bool hovered = GetPropW(di->hwndItem, L"_Hot") != nullptr;
            bool focused = (di->itemState & ODS_FOCUS) != 0;

            COLORREF bgCol, fgCol, bdCol;
            if (accent) {
                bgCol = pressed ? Pal::ACCENT_P : (hovered ? Pal::ACCENT_H : Pal::ACCENT);
                fgCol = RGB(255, 255, 255);
                bdCol = pressed ? Pal::ACCENT_P : (hovered ? Pal::ACCENT_H : Pal::ACCENT);
            }
            else {
                bgCol = pressed ? Pal::CNL_P : (hovered ? Pal::CNL_H : Pal::CNL_BG);
                fgCol = hovered ? Pal::TEXT : Pal::DIM;
                bdCol = hovered ? Pal::BORDER_FOC : Pal::BORDER;
            }

            HDC  dc = di->hDC;
            RECT rc = di->rcItem;

            FillBorder(dc, rc, bgCol, bdCol);

            wchar_t txt[64]{};
            GetWindowTextW(di->hwndItem, txt, 64);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, fgCol);
            HFONT hF = (HFONT)SendMessage(di->hwndItem, WM_GETFONT, 0, 0);
            HFONT oldF = hF ? (HFONT)SelectObject(dc, hF) : nullptr;
            DrawTextW(dc, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (oldF) SelectObject(dc, oldF);

            if (focused) {
                RECT fr = rc; InflateRect(&fr, -3, -3);
                DrawFocusRect(dc, &fr);
            }
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int notif = HIWORD(wParam);

        if (id == IDC_PP_FILTER) {
            if (notif == EN_CHANGE) {
                wchar_t buf[256]{};
                GetDlgItemTextW(hwnd, IDC_PP_FILTER, buf, 256);
                RebuildList(GetDlgItem(hwnd, IDC_PP_LIST), s, buf);
            }
            else if (notif == EN_SETFOCUS) {
                s->editFocus = true;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (notif == EN_KILLFOCUS) {
                s->editFocus = false;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        else if (id == IDC_PP_SELECT || (id == IDC_PP_LIST && notif == LBN_DBLCLK)) {
            CommitSelection(hwnd, s);
        }
        else if (id == IDC_PP_CANCEL) {
            s->done = true;
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { s->done = true; DestroyWindow(hwnd); }
        break;

    case WM_CLOSE:
        s->done = true;
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (s) {
            if (s->hUiFont) { DeleteObject(s->hUiFont);  s->hUiFont = nullptr; }
            if (s->hMono) { DeleteObject(s->hMono);    s->hMono = nullptr; }
            if (s->hBrBg) { DeleteObject(s->hBrBg);    s->hBrBg = nullptr; }
            if (s->hBrSurf) { DeleteObject(s->hBrSurf);  s->hBrSurf = nullptr; }
            if (s->hBrInput) { DeleteObject(s->hBrInput); s->hBrInput = nullptr; }
        }
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DWORD ShowProcessPicker(HWND hParent)
{
    static bool registered = false;
    HINSTANCE hInst = GetModuleHandleW(nullptr);

    if (!registered) {
        WNDCLASSW wc{};
        wc.lpfnWndProc = PickerProc;
        wc.hInstance = hInst;
        wc.hbrBackground = nullptr;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"ProcessPickerWnd";
        RegisterClassW(&wc);
        registered = true;
    }

    PickerState state;
    state.all = SnapProcesses();

    EnableWindow(hParent, FALSE);

    RECT rc{ 0, 0, Lay::CW, Lay::CH };
    AdjustWindowRectEx(&rc,
                       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                       FALSE,
                       WS_EX_DLGMODALFRAME);

    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"ProcessPickerWnd", L"Select Process",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        hParent, nullptr, hInst, &state);

    if (!hwnd) {
        EnableWindow(hParent, TRUE);
        return 0;
    }

    {
        RECT pr{}, wr{};
        GetWindowRect(hParent, &pr);
        GetWindowRect(hwnd, &wr);
        int ww = wr.right - wr.left;
        int wh = wr.bottom - wr.top;
        int cx = (pr.left + pr.right) / 2 - ww / 2;
        int cy = (pr.top + pr.bottom) / 2 - wh / 2;
        SetWindowPos(hwnd, nullptr, cx, cy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    MSG msg;
    while (!state.done) {
        BOOL ret = GetMessageW(&msg, nullptr, 0, 0);
        if (ret == 0) { PostQuitMessage(static_cast<int>(msg.wParam)); break; }
        if (ret < 0)  break;
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    return state.ok ? state.result : 0;
}