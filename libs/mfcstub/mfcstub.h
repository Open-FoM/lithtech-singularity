// mfcstub.h - Stub header file for duplicating the needed functionality to 
//		remove MFC from a project with as minimal an amount of change as possible

#ifndef __MFCSTUB_H__
#define __MFCSTUB_H__

// stdlith dependency
#include "stdlith.h"

// Some headers that MFC automatically includes for you
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef _WIN32
#include <crtdbg.h>
#endif

#ifndef _WIN32
#include <strings.h>
#include <ctype.h>
inline int stricmp(const char* string1, const char* string2)
{
	return strcasecmp(string1, string2);
}
inline char* _strupr(char* s)
{
	while (*s)
	{
		*s = static_cast<char>(toupper(*s));
		++s;
	}
	return s;
}
inline char* _strlwr(char* s)
{
	while (*s)
	{
		*s = static_cast<char>(tolower(*s));
		++s;
	}
	return s;
}
inline char* _strrev(char* s)
{
	if (!s)
	{
		return s;
	}
	auto* left = s;
	auto* right = s + strlen(s);
	if (right != s)
	{
		--right;
	}
	while (left < right)
	{
		const char tmp = *left;
		*left = *right;
		*right = tmp;
		++left;
		--right;
	}
	return s;
}
#endif

#include "mfcs_types.h"
#include "mfcs_point.h"
#include "mfcs_rect.h"
#include "mfcs_string.h"
#include "mfcs_misc.h"

#endif // __MFCSTUB_H__
