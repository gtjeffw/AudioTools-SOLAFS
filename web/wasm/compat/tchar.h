/*
 * Minimal stand-in for MSVC's <tchar.h>, used only when compiling the native SOLAFS/WavIO
 * sources with clang/Emscripten (it is on the include path for those builds only; Visual Studio
 * never sees it). Maps the narrow-character _t* names and the *_s functions the sources use to
 * standard C.
 */
#ifndef SOLAFS_COMPAT_TCHAR_H
#define SOLAFS_COMPAT_TCHAR_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char _TCHAR;
typedef char TCHAR;
typedef int errno_t;

#define _T(x) x
#define _TRUNCATE ((size_t)-1)

#define _tprintf_s printf
#define _ftprintf_s fprintf
#define _tcslen strlen
#define _tcscmp strcmp
#define _tcschr strchr
#define _tstoi atoi
#define _tstof atof
#define _tmain main
#define _sntprintf_s(buf, size, count, ...) snprintf((buf), (size), __VA_ARGS__)

static inline errno_t _tfopen_s(FILE ** fp, const char * path, const char * mode)
{
	*fp = fopen(path, mode);
	return *fp ? 0 : errno;
}

#endif
