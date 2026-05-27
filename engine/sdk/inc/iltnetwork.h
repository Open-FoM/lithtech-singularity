// ------------------------------------------------------------------------- //
//
// FILE      : ILTNETWORK.H
//
// PURPOSE   : FoM-specific network interface used by client/server shells.
//
// ------------------------------------------------------------------------- //

#ifndef __ILTNETWORK_H__
#define __ILTNETWORK_H__

#include "iltmessage.h"
#include "ltbasetypes.h"
#include "ltcodes.h"
#include "ltmodule.h"

#ifdef FOM_GAME_BUILD
class VariableSizedPacket;
#else
class VariableSizedPacket {
public:
  virtual ~VariableSizedPacket() = default;

  // Deserialize packet data from the provided message.
  virtual bool Read(ILTMessage_Read *message) = 0;

  // Serialize packet data into the provided message.
  virtual bool Write(ILTMessage_Write *message) = 0;
};
#endif

struct NetworkAddress {
  uint32 m_nIp = 0;
  uint16 m_nPort = 0;
};

// RakNet packet priority enum
enum class NetPacketPriority : uint8 { kImmediate = 0, kHigh = 1, kMedium = 2, kLow = 3 };

// RakNet packet reliability enum
enum class NetPacketReliability : uint8 {
  kUnreliable = 0,
  kUnreliableSequenced = 1,
  kReliable = 2,
  kReliableOrdered = 3,
  kReliableSequenced = 4,
  kUnreliableWithAckReceipt = 5,
  kReliableWithAckReceipt = 6,
  kReliableOrderedWithAckReceipt = 7
};

enum class DispatchTarget : uint8 { kMaster = 1, kWorld = 2 };

enum class PacketRoute : uint8 { kClient = 0, kServer = 1, kBoth = 2 };

enum class NetResult : uint8 { kOk = 0, kNotInitialized, kInvalidArgs, kNotConnected, kSendFailed };

class ILTNetwork : public IBase {
public:
  interface_version(ILTNetwork, 1);

  // Returns true if the master connection is alive.
  virtual bool IsMasterConnected() = 0;

  // Returns true if the world connection is alive.
  virtual bool IsWorldConnected() = 0;

  // Initializes client networking and binds to a local port.
  virtual NetResult Init(uint16 localPort) = 0;

  // Shuts down client networking and clears internal state.
  virtual NetResult Shutdown() = 0;

  // Updates FoM packet subscriptions on the network backend.
  virtual void SubscribePacket(uint8 packetId, PacketRoute route) = 0;
  virtual void UnsubscribePacket(uint8 packetId, PacketRoute route = PacketRoute::kBoth) = 0;
  virtual void ClearSubscriptions(PacketRoute route = PacketRoute::kBoth) = 0;

  // Connects to a world server address.
  virtual NetResult ConnectWorld(const NetworkAddress &address) = 0;

  // Disconnects active world/master connections.
  virtual NetResult DisconnectAll() = 0;

  // Connects to a master server address.
  virtual NetResult ConnectMaster(const NetworkAddress &address) = 0;

  // Dispatches a packet to master or world (dest=1 master, dest=2 world).
  virtual NetResult SendPacket(VariableSizedPacket *packet, NetPacketPriority priority, NetPacketReliability reliability,
                               uint8 orderingChannel, DispatchTarget destination) = 0;

  // Updates internal connection state.
  virtual void Update() = 0;

  // Data slot access
  virtual void *GetDataSlot(int index) = 0;
  virtual void SetDataSlot(int index, void *value) = 0;
};

#endif // __ILTNETWORK_H__
