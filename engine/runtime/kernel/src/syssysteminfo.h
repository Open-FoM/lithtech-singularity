#ifndef __SYSSYSTEMINFO_H__
#define __SYSSYSTEMINFO_H__

#if defined(__LINUX) || defined(__APPLE__)
#include "sys/linux/systeminfo.h"
#elif __XBOX
#elif _WIN32
#include "sys/win/systeminfo.h"
#endif

#endif // __SYSSYSTEMINFO_H__
