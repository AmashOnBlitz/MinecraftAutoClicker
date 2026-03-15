#include "pch.h"
#include "Windows.h"
#include "tchar.h"
#include "strsafe.h"
#include "Renderer.hpp"
#include "RenderManager.hpp"
#include <filesystem>
#include "Config.hpp"
#include "../resources/resource.h"

namespace fs = std::filesystem;

LRESULT CALLBACK fnWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	TCHAR szAppName[] = _T("Auto Clicker");
	TCHAR szMainWndName[] = _T("Minecraft Auto Clicker");
	HWND hWnd = {};
	WNDCLASSEX wndClass = {};
	MSG msg = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
	wndClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hIconSm = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = fnWinProc;
	wndClass.lpszClassName = szMainWndName;
	wndClass.lpszMenuName = NULL;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!::RegisterClassEx(&wndClass)) {
		const int BuffLimit = 256;
		TCHAR szBuff[BuffLimit];
		DWORD DWErr = GetLastError();
		StringCchPrintf(
			szBuff, BuffLimit,
			_TEXT("Cannot Register Window Class Of Name '%s' \nReintalling The App May Fix!\nError Code: %lu"),
			wndClass.lpszClassName, DWErr
		);
		::MessageBox(NULL, szBuff, _TEXT("Internal Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	hWnd = ::CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		szMainWndName,
		szMainWndName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		360, 550,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hWnd == NULL) {
		const int BuffLimit = 256;
		TCHAR szBuff[BuffLimit];
		DWORD DWErr = ::GetLastError();
		StringCchPrintf(
			szBuff, BuffLimit,
			_TEXT("Cannot Create Window Of Name '%s' \nReintalling The App May Fix!\nError Code: %lu"),
			szMainWndName, DWErr
		);
		::MessageBox(NULL, szBuff, _TEXT("Internal Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	::ShowWindow(hWnd, iCmdShow);
	::UpdateWindow(hWnd);

	ObjectManager::GetMainRenderer().btnInject->SetOnClick([hWnd]() {

#define INJ_DEBUG 1
#ifdef INJ_DEBUG
		wchar_t _dbgTmp[MAX_PATH]{};
		GetTempPathW(MAX_PATH, _dbgTmp);
		wchar_t _dbgPath[MAX_PATH]{};
		swprintf_s(_dbgPath, L"%sac_inject.log", _dbgTmp);
		FILE* _dbgF = nullptr;
		_wfopen_s(&_dbgF, _dbgPath, L"w");
		auto InjLog = [&](const wchar_t* fmt, auto... args) {
			if (!_dbgF) return;
			wchar_t _b[512]{};
			swprintf_s(_b, fmt, args...);
			fwprintf(_dbgF, L"[%llu] %s\n", GetTickCount64(), _b);
			fflush(_dbgF);
		};
#else
		auto InjLog = [](const wchar_t*, auto...) {};
#endif

		auto RemoteLoadLibrary = [&](HANDLE hProc, const std::string& path, FARPROC pLLA) -> DWORD {
			void* mem = ::VirtualAllocEx(hProc, 0, path.size() + 1,
										 MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			InjLog(L"RemoteLoadLibrary: alloc %p for %S", mem, path.c_str());
			if (!mem) return 0;
			::WriteProcessMemory(hProc, mem, path.c_str(), path.size() + 1, 0);
			HANDLE hT = ::CreateRemoteThread(hProc, 0, 0,
											 (LPTHREAD_START_ROUTINE)pLLA, mem, 0, 0);
			InjLog(L"RemoteLoadLibrary: thread %p err=%lu", hT, GetLastError());
			DWORD exitCode = 0;
			if (hT && hT != INVALID_HANDLE_VALUE) {
				WaitForSingleObject(hT, 5000);
				GetExitCodeThread(hT, &exitCode);
				::CloseHandle(hT);
			}
			::VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
			InjLog(L"RemoteLoadLibrary: exitCode=0x%lX for %S", exitCode, path.c_str());
			return exitCode;
		};

		InjLog(L"Inject: button clicked");

		fs::path dllPath = fs::absolute("Addon.dll");
		InjLog(L"Inject: dllPath exists=%d  path=%s", (int)fs::exists(dllPath), dllPath.wstring().c_str());
		if (!fs::exists(dllPath)) {
			MessageBox(hWnd,
					   _TEXT("Addon.dll not found. Try reinstalling."),
					   _TEXT("DLL Not Found"), MB_ICONERROR | MB_OK);
			return;
		}

		auto& r = ObjectManager::GetMainRenderer();
		AcConfig cfg{};
		cfg.cps = r.knobCps ? r.knobCps->GetValue() : 18.0f;
		cfg.cooldown = r.knobCooldown ? r.knobCooldown->GetValue() : 1.0f;
		cfg.triggerCooldown = r.knobTriggerCooldown ? r.knobTriggerCooldown->GetValue() : 4.0f;
		cfg.lClickVK = r.keySelector ? r.keySelector->GetLClickVK() : VK_F9;
		cfg.rClickVK = r.keySelector ? r.keySelector->GetRClickVK() : VK_F10;
		cfg.debugPanel = r.chkDebugPanel ? r.chkDebugPanel->IsChecked() : true;
		cfg.controlDialog = r.chkControlDialog ? r.chkControlDialog->IsChecked() : true;
		cfg.debugToggleVK = r.ddDebugToggleKey ? r.ddDebugToggleKey->GetSelectedValue() : VK_F11;
		cfg.controlToggleVK = r.ddControlToggleKey ? r.ddControlToggleKey->GetSelectedValue() : VK_F12;
		SaveConfig(cfg);
		InjLog(L"Inject: SaveConfig done cps=%.1f", cfg.cps);

		int PID = ObjectManager::GetMainRenderer().pidInput->GetPid();
		InjLog(L"Inject: target PID=%d", PID);

		HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
		InjLog(L"Inject: OpenProcess -> %p err=%lu", hProcess, GetLastError());
		if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
			MessageBox(hWnd,
					   _TEXT("Failed to open process. It may not exist or access was denied."),
					   _TEXT("Failure"), MB_ICONERROR | MB_OK);
			return;
		}

		HMODULE hK32 = GetModuleHandleW(L"kernel32.dll");
		FARPROC pLLA = GetProcAddress(hK32, "LoadLibraryA");
		InjLog(L"Inject: kernel32=%p LoadLibraryA=%p", hK32, pLLA);

		fs::path jvmSrc = fs::absolute("jvm.dll");
		InjLog(L"Inject: jvm.dll exists=%d  path=%s", (int)fs::exists(jvmSrc), jvmSrc.wstring().c_str());
		if (fs::exists(jvmSrc)) {
			std::string jvmPathStr = jvmSrc.string();
			DWORD jvmCode = RemoteLoadLibrary(hProcess, jvmPathStr, pLLA);
			InjLog(L"Inject: jvm remote load exitCode=0x%lX", jvmCode);
			if (jvmCode == 0) {
				InjLog(L"Inject: WARNING jvm.dll failed to load in remote process, continuing anyway");
				int choice = MessageBox(hWnd,
										_TEXT("jvm.dll failed to load into the target process.\nContinue injecting Addon.dll anyway?"),
										_TEXT("JVM Warning"), MB_ICONWARNING | MB_YESNO);
				if (choice == IDNO) {
					::CloseHandle(hProcess);
					return;
				}
			}
		}
		else {
			InjLog(L"Inject: jvm.dll not found next to injector, skipping");
		}

		std::string dllPathStr = dllPath.string();
		DWORD addonCode = RemoteLoadLibrary(hProcess, dllPathStr, pLLA);
		InjLog(L"Inject: Addon.dll remote load exitCode=0x%lX", addonCode);

		::CloseHandle(hProcess);

		wchar_t resultMsg[256]{};
		swprintf_s(resultMsg,
				   L"jvm.dll loaded first, then Addon.dll.\nAddon exitCode: 0x%lX\n%s",
				   addonCode,
				   addonCode ? L"OK" : L"FAILED — check ac_inject.log in %%TEMP%%");
		MessageBox(hWnd, resultMsg,
				   addonCode ? L"Injected!" : L"Load Failed",
				   MB_OK | MB_APPLMODAL | (addonCode ? MB_ICONINFORMATION : MB_ICONERROR));

#ifdef INJ_DEBUG
		if (_dbgF) fclose(_dbgF);
#endif
	});

	while (::GetMessage(&msg, NULL, 0, 0)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return 1;
}

void HandleCreate(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ObjectManager::GetMainRenderer().SetStage(hwnd);
}

void HandleMainWndPaint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ObjectManager::GetMainRenderer().Render();
}

LRESULT CALLBACK fnWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_CREATE:
		HandleCreate(hwnd, msg, wParam, lParam);
		return 1;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		HandleMainWndPaint(hwnd, msg, wParam, lParam);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 1;
	case WM_MOUSEMOVE: {
		float mx = (float)LOWORD(lParam);
		float my = (float)HIWORD(lParam);
		ObjectManager::GetMainRenderer().OnMouseMove(mx, my);
		TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
		TrackMouseEvent(&tme);
		return 0;
	}
	case WM_MOUSELEAVE:
		ObjectManager::GetMainRenderer().OnMouseLeave();
		return 0;
	case WM_LBUTTONDOWN:
		ObjectManager::GetMainRenderer().OnMouseDown((float)LOWORD(lParam), (float)HIWORD(lParam));
		return 0;
	case WM_LBUTTONUP:
		ObjectManager::GetMainRenderer().OnMouseUp((float)LOWORD(lParam), (float)HIWORD(lParam));
		return 0;
	case WM_MOUSEWHEEL: {
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		ScreenToClient(hwnd, &pt);
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		ObjectManager::GetMainRenderer().OnMouseWheel((float)pt.x, (float)pt.y, delta);
		return 0;
	}
	case WM_CHAR:
		ObjectManager::GetMainRenderer().OnChar((wchar_t)wParam);
		return 0;
	case WM_KEYDOWN:
		ObjectManager::GetMainRenderer().OnKeyDown((int)wParam);
		return 0;
	case WM_COMMAND:
		ObjectManager::GetMainRenderer().OnCommand(wParam, lParam);
		return 0;
	default:
		break;
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}