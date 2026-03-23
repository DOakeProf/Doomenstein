#include "Game/App.hpp"

#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)

//-----------------------------------------------------------------------------------------------
// #SD1ToDo: Later we will move this useful macro to a more central place, e.g. Engine/Core/EngineCommon.hpp
//
#define UNUSED(x) (void)(x);


//-----------------------------------------------------------------------------------------------
// #SD1ToDo: This will eventually go away once we add a Window engine class later on.
// 
constexpr float CLIENT_ASPECT = 2.0f; // We are requesting a 1:1 aspect (square) window area

HDC g_displayDeviceContext = nullptr;

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(applicationInstanceHandle);
	UNUSED(commandLineString);

	g_app = new App();
	g_app->RunMainLoop();
	delete g_app;
	g_app = nullptr;

	return 0;
}