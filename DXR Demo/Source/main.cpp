#include "Engine/Engine.hpp"

//https://stackoverflow.com/questions/73481093/issue-with-read-vertex-index-buffer-with-structured-buffer-in-dxr

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
