#include "bdefs.h"
#include "systeminfo.h"

#include <unistd.h>

uint32 CSystemInfo::GetProcessorCount()
{
	const long count = sysconf(_SC_NPROCESSORS_ONLN);
	if (count > 0)
	{
		return static_cast<uint32>(count);
	}

	return 1;
}
