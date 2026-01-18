#ifndef __MEMORY_H__
#define __MEMORY_H__

    #ifndef __NEW_H__
    #if defined(_WIN32)
    #include <new.h>
    #else
    #include <new>
    #endif
    #define __NEW_H__
    #endif

    #ifndef __MALLOC_H__
    #if defined(_WIN32)
    #include <malloc.h>
    #else
    #include <stdlib.h>
    #endif
    #define __MALLOC_H__
    #endif

    #ifndef __LITHEXCEPTION_H__
    #include "lithexception.h"
    #endif


#endif //__MEMORY_H__
