#include "Engine/Engine.hpp"

// https://github.com/acmarrs/IntroToDXR
// https://stackoverflow.com/questions/73481093/issue-with-read-vertex-index-buffer-with-structured-buffer-in-dxr

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	Engine* app = new Engine(hInstance);
	try
	{
		app->Initialize();
		app->Run();
		delete app;
	}
	catch (...)
	{
		::MessageBoxA(app->GetHWND(), "Failed to run app!", "Error", MB_OK);
		delete app;
		return -1;
	}
	
	return 0;
}
