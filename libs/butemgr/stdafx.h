
#if defined(_WIN32)
#if !defined(_NOMFC)
#include "afxwin.h"
#include "afxext.h"
#include "afxcmn.h"
#else
#include <windows.h>
#include "mfcstub.h"
#endif
#else
#include "mfcstub.h"
#endif
