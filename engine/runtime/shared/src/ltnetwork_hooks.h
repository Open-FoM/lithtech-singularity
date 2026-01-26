// ------------------------------------------------------------------------- //
//
// FILE      : LTNETWORK_HOOKS.H
//
// PURPOSE   : Internal hooks for injecting MMO networking packets.
//
// ------------------------------------------------------------------------- //

#ifndef __LTNETWORK_HOOKS_H__
#define __LTNETWORK_HOOKS_H__

#include "ltbasetypes.h"
#include "packet.h"
#include "iltmessage.h"

using LTNetPacketInjector = void (*)(const CPacket_Read &packet, const uint8 *senderAddr, uint16 senderPort);
using LTNetMessageWriteAllocator = ILTMessage_Write *(*)();

void LTNet_SetClientPacketInjector(LTNetPacketInjector injector);
void LTNet_SetServerPacketInjector(LTNetPacketInjector injector);
void LTNet_SetClientMessageWriteAllocator(LTNetMessageWriteAllocator allocator);
void LTNet_SetServerMessageWriteAllocator(LTNetMessageWriteAllocator allocator);

#endif // __LTNETWORK_HOOKS_H__
