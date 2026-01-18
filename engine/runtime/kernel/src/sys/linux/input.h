// Linux/POSIX input header wrapper.
// This is used for mac builds to reuse the shared InputMgr interface.

#ifndef __LINUX_INPUT_H__
#define __LINUX_INPUT_H__

#include "ltbasetypes.h"

#ifndef BYTE
typedef uint8 BYTE;
#endif

#include "sys/win/input.h"

#endif
