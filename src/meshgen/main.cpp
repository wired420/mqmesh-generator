//
// Main.cpp
//

#include "meshgen/Application.h"

int main(int argc, char* argv[])
{
	std::string startingZone;
	if (argc > 1)
		startingZone = argv[1];

#if defined(__EMSCRIPTEN__)
	// The browser owns the main loop, so the application must outlive main().
	Application* window = new Application(startingZone);
	return window->RunMainLoop();
#else
	Application window(startingZone);
	return window.RunMainLoop();
#endif
}
