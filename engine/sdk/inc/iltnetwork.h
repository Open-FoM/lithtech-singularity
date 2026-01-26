// ------------------------------------------------------------------------- //
//
// FILE      : ILTNETWORK.H
//
// PURPOSE   : FoM-specific network interface used by client/server shells.
//
// ------------------------------------------------------------------------- //

#ifndef __ILTNETWORK_H__
#define __ILTNETWORK_H__

#include "ltbasetypes.h"
#include "ltcodes.h"
#include "ltmodule.h"

class VariableSizedPacket;

struct NetworkAddress {
  uint32 m_nIp = 0;
  uint16 m_nPort = 0;
};

// RakNet packet priority enum
enum class PacketPriority : uint8 { kImmediate = 0, kHigh = 1, kMedium = 2, kLow = 3 };

// RakNet packet reliability enum
enum class PacketReliability : uint8 {
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

class ILTNetwork : public IBase {
public:
  interface_version(ILTNetwork, 0);

  // Returns true if the master connection is alive.
  virtual bool IsMasterConnected() = 0;

  // Returns true if the world connection is alive.
  virtual bool IsWorldConnected() = 0;

  // Initializes client networking and binds to a local port.
  virtual LTRESULT InitNetwork(uint16 localPort) = 0;

  // Shuts down client networking and clears internal state.
  virtual LTRESULT ShutdownNetwork() = 0;

  // Updates notification flags on the network backend.
  virtual bool SetNotificationFlag(int id, int group) = 0;

  // Connects to a world server address.
  virtual LTRESULT ConnectWorld(const NetworkAddress &address) = 0;

  // Disconnects active world/master connections.
  virtual LTRESULT Disconnect() = 0;

  // Polls master address info and updates cached values.
  virtual bool PollMasterAddress() = 0;

  // Starts a login request to the master server.
  virtual LTRESULT BeginLogin(const char *user, const char *password, uint16 port, int loginFlags, const char *token,
                              bool hasPayload, const void *payload, int payloadSize) = 0;

  // Dispatches a packet to master or world (dest=1 master, dest=2 world).
  /// Decomp refs: IDA Object.lto: sub_74AF40 @ 0x74AF40.
  virtual LTRESULT DispatchPacket(VariableSizedPacket *packet, PacketPriority priority, PacketReliability reliability,
                                  uint8 orderingChannel, DispatchTarget destination) = 0;

  // Sends a ping/handshake packet to the given address buffer.
  virtual LTRESULT SendHandshake(const NetworkAddress &address) = 0;

  // Ticks internal connection state.
  virtual void UpdateNetwork() = 0;

  // Bitstream helpers (FoM custom encoding).
  virtual int WriteBitStream(void *src, int maxBits, void *stream) = 0;
  virtual int ReadBitStream(void *dst, int maxBits, void *stream) = 0;

  // Returns master stats result (driver-specific).
  virtual int GetMasterStats() = 0;

  // Dumps stats for master/world to the log.
  virtual void DumpStats(int target) = 0;

  // Flag management.
  virtual uint8 HasFlags(int mask) = 0;
  virtual void SetFlags(int mask) = 0;
  virtual void ClearFlags(int mask) = 0;
  virtual void TickFlags() = 0;

  // Returns engine frame time (ms).
  virtual int GetFrameTime() = 0;

  // Data slot access (FoM profile data pointers).
  virtual void *GetDataSlot(int index) = 0;
  virtual void SetDataSlot(int index, void *value) = 0;
};

#endif // __ILTNETWORK_H__
