// ------------------------------------------------------------------------- //
//
// FILE      : LTNETWORK_HOOKS.CPP
//
// PURPOSE   : Lightweight hook storage for packet injection and message allocators.
//
// ------------------------------------------------------------------------- //

#include "ltnetwork_hooks.h"

namespace
{
LTNetPacketInjector g_clientPacketInjector = nullptr;
LTNetPacketInjector g_serverPacketInjector = nullptr;
LTNetMessageWriteAllocator g_clientMessageWriteAllocator = nullptr;
LTNetMessageWriteAllocator g_serverMessageWriteAllocator = nullptr;
} // namespace

void LTNet_SetClientPacketInjector(LTNetPacketInjector injector)
{
	g_clientPacketInjector = injector;
}

void LTNet_SetServerPacketInjector(LTNetPacketInjector injector)
{
	g_serverPacketInjector = injector;
}

void LTNet_SetClientMessageWriteAllocator(LTNetMessageWriteAllocator allocator)
{
	g_clientMessageWriteAllocator = allocator;
}

void LTNet_SetServerMessageWriteAllocator(LTNetMessageWriteAllocator allocator)
{
	g_serverMessageWriteAllocator = allocator;
}
