#ifndef __SYSSOCKET_H__
#define __SYSSOCKET_H__

#if defined(__LINUX) || defined(__APPLE__)
#include "sys/linux/socket.h"
#elif __XBOX
#elif _WIN32
#include "sys/win/socket.h"
#endif

#endif // __SYSSOCKET_H__

