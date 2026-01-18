#ifndef __SYSLTHREAD_H__
#define __SYSLTHREAD_H__

// This is a redirector to get the system dependent include
#if defined(__LINUX) || defined(__APPLE__)
#include "sys/linux/lthread.h"
#elif __XBOX
#include "sys/xbox/lthread.h"
#elif _WIN32
#include "sys/win/lthread.h"
#endif

#endif // __SYSTHREAD_H__

