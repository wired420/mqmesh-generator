// imgui_platform.cpp - bits the MQ imgui fork expects the host to provide.
#include <imgui.h>
#include <cstdarg>
#include <cstdio>

void ImGuiLogDebug(const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	vfprintf(stderr, szFormat, args);
	va_end(args);
}
