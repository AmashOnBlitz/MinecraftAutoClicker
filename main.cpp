#include "Windows.h"
#include "tchar.h"
#include "strsafe.h"
#include "Renderer.hpp"
#include "RenderManager.hpp"
#include <filesystem>
#include "Config.hpp"

namespace fs = std::filesystem;

LRESULT CALLBACK fnWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	TCHAR szAppName[] = _T("Auto Clicker");
	TCHAR szMainWndName[] = _T("Minecraft Auto Clicker");
	HWND hWnd = {};
	WNDCLASS wndClass = {};
	MSG msg = {};
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
	wndClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = fnWinProc;
	wndClass.lpszClassName = szMainWndName;
	wndClass.lpszMenuName = NULL;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!::RegisterClass(&wndClass)) {
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
		360,
		550,
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
		fs::path dllPath = fs::absolute("Addon.dll");
		if (!fs::exists(dllPath)) {
			MessageBox(
				hWnd,
				_TEXT("Addon Dll does not exist (addon.dll) \nTry reinstalling the app"),
				_TEXT("ERROR : DLL Not Found"),
				MB_ICONERROR | MB_APPLMODAL | MB_OK
			);
			return;
		}
		int PID = ObjectManager::GetMainRenderer().pidInput->GetPid();
		HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
		if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE) {
			MessageBox(
				hWnd,
				_TEXT("Failed to open: process probably doesn't exist or access denied"),
				_TEXT("Failure"),
				MB_ICONERROR | MB_APPLMODAL | MB_OK
			);
			return;
		}
		void* memLoc = ::VirtualAllocEx(hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (memLoc == nullptr) {
			MessageBox(
				hWnd,
				_TEXT("Failed to allocate memory in remote process"),
				_TEXT("Access Denied"),
				MB_ICONERROR | MB_APPLMODAL | MB_OK
			);
			return;
		}
		std::string dllPathStr = dllPath.string();
		BOOL b = FALSE;
		b = ::WriteProcessMemory(hProcess, memLoc, dllPathStr.c_str(), dllPathStr.size() + 1, 0);
		if (b == FALSE) {
			MessageBox(
				hWnd,
				_TEXT("Failed to write in the remotely allocated memory"),
				_TEXT("Access Denied"),
				MB_ICONERROR | MB_APPLMODAL | MB_OK
			);
			return;
		}

		HANDLE hThread = ::CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, memLoc, 0, 0);

		if (hThread == INVALID_HANDLE_VALUE || hThread == nullptr) {
			MessageBox(
				hWnd,
				_TEXT("Failed to create thread in remote process"),
				_TEXT("Access Denied"),
				MB_ICONERROR | MB_APPLMODAL | MB_OK
			);
			return;
		}
		else {
			::CloseHandle(hThread);
		}
		if (hProcess) {
			::CloseHandle(hProcess);
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
		SaveConfig(cfg);

		MessageBox(hWnd, L"Injected!", L"Success", MB_OK | MB_APPLMODAL | MB_ICONINFORMATION);
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