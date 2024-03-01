#include "gui.h"

#include "../parser/parser.h"

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_dx9.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_internal.h"

#include <iostream>
#include <windows.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter)) {
		return true;
	}

	switch (message) {
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED) {
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = LOWORD(longParameter);
			gui::ResetDevice();
		}
	} return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) {
			return 0;
		} break;


	case WM_DESTROY: {
		PostQuitMessage(0);
	} return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter);
	} return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON) {
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{};

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 && gui::position.x <= gui::WIDTH && gui::position.y >= 0 && gui::position.y <= 19) {
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0,
					0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
			}
		}

	} return 0;

	}
	}

	return DefWindowProcW(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName, const char* className) noexcept {
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(
		className,
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept {
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

}

bool gui::CreateDevice() noexcept {
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d) {
		return false;
	}

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
	{
		return false;
	}

	return true;
}

void gui::ResetDevice() noexcept {
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL) {
		IM_ASSERT(0);
	}

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept {
	if (device) {
		device->Release();
		device = nullptr;
	}

	if (d3d) {
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept {
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept {
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ResetDevice();
	}
}


int Selecteditem = 0; // TO MOVE ELSEWHERE
bool open_proc_selector = false; // ""
int NBYTES_DEFAULT = 1024; // ""

std::string bytes[1024]; // USE FUCKING STD::VECTOR
const char* bytes_cstr[1024]; // ""

void gui::Render() noexcept {
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		TITLE,
		&exit,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_MenuBar
	);

	int menu_action = -1;
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open...")) {
				menu_action = 0;
			}

			if (ImGui::MenuItem("Attach... (demo)")) {
				menu_action = 1;
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("About"))
		{
			if (ImGui::MenuItem("GitHub")) {
				ShellExecuteA(0, NULL, "https://github.com/c0w5lip/imgui-hex-viewer", NULL, NULL, SW_SHOWDEFAULT);
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	ImGui::Text("ADDR\t\t00 01 02 03 04 05 06 07\t08 09 0A 0B 0C 0D 0E 0F\tASCII");
	ImGui::Separator();
	static ImGuiTextBuffer hex_view;

	
	if (menu_action == 0) {
		OPENFILENAME ofn;
		char szFileName[MAX_PATH] = "";

		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = "PE (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = "exe";

		if (GetOpenFileName(&ofn))
		{
			readOpenedProcess(szFileName, bytes, bytes_cstr, NBYTES_DEFAULT);
		}
	}


	if (menu_action == 1) {
		open_proc_selector = true;
	}

	if (open_proc_selector) {
		menu_action = -1;
		static const int nProcesses = 100; // TO REMOVE TO JUST LIST ALL PROCESSES
		std::string processes[nProcesses]; // ""
		const char* processes_cstr[nProcesses]; // ""
		getRunningProcesses(processes, processes_cstr, nProcesses); // ""

		ImGui::Begin("Attach process...");

		if (ImGui::Combo("", &Selecteditem, processes_cstr, IM_ARRAYSIZE(processes_cstr))) {
			readAttachedProcess(processes_cstr[Selecteditem], bytes, bytes_cstr, NBYTES_DEFAULT);
			open_proc_selector = false;
			menu_action = 1;
		}

		ImGui::EndChild();
		ImGui::End();
	}


	if (menu_action != -1) { // update hex_view whenever a file is opened or a process attached
		hex_view.clear();

		for (int i = 0; i < IM_ARRAYSIZE(bytes); i += 16) {
			std::stringstream addr_stream;
			addr_stream << std::hex << std::setfill('0') << std::setw(8) << i;
			std::string hex_addr(addr_stream.str());
			hex_view.appendf(hex_addr.c_str());
			std::transform(hex_addr.begin(), hex_addr.end(), hex_addr.begin(), ::toupper); // uppercase

			hex_view.appendf(":   ");


			for (int j = 0; j < 16; j++) {
				std::stringstream strValue;
				strValue << bytes_cstr[i + j];

				int intValue;
				strValue >> intValue;

				std::stringstream bytes_stream;
				bytes_stream << std::hex << std::setfill('0') << std::setw(2) << intValue;

				std::string hex_bytes(bytes_stream.str());

				std::transform(hex_bytes.begin(), hex_bytes.end(), hex_bytes.begin(), ::toupper); // uppercase
				hex_view.appendf("%0s", hex_bytes.c_str());

				if (j == 7) {
					hex_view.appendf("\t");
				}
				else {
					hex_view.appendf(" ");
				}
			}
			hex_view.appendf("\t");


			for (int k = 0; k < 16; k++) {
				wchar_t character = static_cast<wchar_t>(std::stoi(bytes_cstr[i + k]));

				if (std::stoi(bytes_cstr[i + k]) < 32 || std::stoi(bytes_cstr[i + k]) > 126) {
					hex_view.appendf(".");
				}
				else {
					char mbChar[2] = { 0 };
					WideCharToMultiByte(CP_ACP, 0, &character, 1, mbChar, 2, NULL, NULL);
					const char* convertedChar = mbChar;
					hex_view.appendf(convertedChar);
				}

			}

			hex_view.appendf("\n");
		}
	}

	// TODO: fix overlapping
	/*
	auto windowWidth = ImGui::GetWindowSize().y;
	auto textWidth = ImGui::CalcTextSize("nbytes").y;

	float pos_y_save = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY((windowWidth - 2.5*textWidth));
	ImGui::Separator();
	ImGui::InputInt("nbytes", &NBYTES_DEFAULT, 16, 1, 0);
	ImGui::SetCursorPosY(pos_y_save);
	*/

	ImGui::BeginChild("HexView");
	ImGui::TextUnformatted(hex_view.begin(), hex_view.end());
	ImGui::EndChild();


	ImGui::End();
}
