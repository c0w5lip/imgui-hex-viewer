#include "gui/gui.h"

int __stdcall wWinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	gui::CreateHWindow(gui::TITLE, "HexViewer Class");
	gui::CreateDevice();
	gui::CreateImGui();

	while (gui::exit) {
		gui::BeginRender();
		gui::Render();
		gui::EndRender();
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return EXIT_SUCCESS;
}
