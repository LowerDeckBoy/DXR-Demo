#include "Engine/Engine.hpp"


_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	Engine app(hInstance);
	try
	{
		app.Initialize();
		app.Run();
	}
	catch (...)
	{
		::MessageBoxA(app.GetHWND(), "Failed to run app!", "Error", MB_OK);
		return -1;
	}
	
	return 0;
}
