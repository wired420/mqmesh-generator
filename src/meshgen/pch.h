// pch.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined(_WIN32)
#include "targetver.h"

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

// Windows Header Files:
#include <windows.h>
#endif

#include <math.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#if defined(_WIN32)
#include <tchar.h>
#endif
#include <stdio.h>
#include <cstdint>

#include <iostream>
#if defined(_WIN32)
#include <tchar.h>
#include <io.h>
#endif
#include <string>
#include <fstream>

#if defined(_WIN32)
#include <shlobj.h>
#include <Shlwapi.h>
#endif

#if !defined(MESHGEN_HEADLESS)
#include <SDL2/SDL.h>
#endif
#if defined(_WIN32)
#include <SDL2/SDL_syswm.h>
#endif
