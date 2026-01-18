#ifndef __SYSUDPDRIVER_H__
#define __SYSUDPDRIVER_H__

#if defined(__LINUX) || defined(__APPLE__)
#include "sys/win/udpdriver.h"
#elif __XBOX
#elif _WIN32
#include "sys/win/udpdriver.h"
#endif

#endif // __SYSUDPDRIVER_H__

