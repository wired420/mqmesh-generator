// compat.h - force-included shim so the Windows-only MQ2Nav sources build with
// clang/Emscripten (and native Linux for testing).
#pragma once

#if !defined(_WIN32)

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <strings.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef unsigned char BYTE;
typedef uint16_t WORD;
typedef char CHAR;

#define _fsopen(name, mode, share) fopen(name, mode)
#define _SH_DENYNO 0x40
#define _SH_DENYWR 0x20
#define _SH_DENYRW 0x10
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define stricmp strcasecmp

#ifdef __cplusplus
template <size_t N>
inline int sprintf_s(char (&buf)[N], const char* fmt, ...)
{
	va_list args; va_start(args, fmt);
	int r = vsnprintf(buf, N, fmt, args);
	va_end(args);
	return r;
}
inline int sprintf_s(char* buf, size_t n, const char* fmt, ...)
{
	va_list args; va_start(args, fmt);
	int r = vsnprintf(buf, n, fmt, args);
	va_end(args);
	return r;
}
template <size_t N>
inline int strcpy_s(char (&dst)[N], const char* src)
{
	snprintf(dst, N, "%s", src);
	return 0;
}
inline int strcpy_s(char* dst, size_t n, const char* src)
{
	snprintf(dst, n, "%s", src);
	return 0;
}
template <size_t N>
inline int strcat_s(char (&dst)[N], const char* src)
{
	size_t l = strlen(dst);
	if (l < N) snprintf(dst + l, N - l, "%s", src);
	return 0;
}
inline int strncpy_s(char* dst, size_t n, const char* src, size_t count)
{
	size_t len = strnlen(src, count);
	if (len >= n) len = n - 1;
	memcpy(dst, src, len); dst[len] = 0;
	return 0;
}
template <size_t N>
inline int strncpy_s(char (&dst)[N], const char* src, size_t count) { return strncpy_s(dst, N, src, count); }
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define __forceinline inline __attribute__((always_inline))

#endif // !_WIN32
