// ------------------------------------------------------------------------- //
//
// FILE      : CLTNETWORK.H
//
// PURPOSE   : FoM-specific network interface implementation (engine side).
//
// ------------------------------------------------------------------------- //

#ifndef __CLTNETWORK_H__
#define __CLTNETWORK_H__

#include "iltnetwork.h"
#include <array>
#include <cstdint>

namespace SLNet {
class RakPeerInterface;
struct SystemAddress;
} // namespace SLNet

class CLTNetwork : public ILTNetwork {
public:
  declare_interface(CLTNetwork);

  CLTNetwork();

  bool IsMasterConnected() override;
  bool IsWorldConnected() override;
  LTRESULT InitNetwork(uint16 localPort) override;
  LTRESULT ShutdownNetwork() override;
  bool SetNotificationFlag(int id, int group) override;
  LTRESULT ConnectWorld(const NetworkAddress &address) override;
  LTRESULT Disconnect() override;
  LTRESULT ConnectMaster(const NetworkAddress &address) override;
  LTRESULT SendPacket(VariableSizedPacket *packet, PacketPriority priority, PacketReliability reliability,
                      uint8 orderingChannel, DispatchTarget destination) override;
  LTRESULT SendHandshake(const NetworkAddress &address) override;
  void Tick() override;
  int WriteBitStream(void *src, int maxBits, void *stream) override;
  int ReadBitStream(void *dst, int maxBits, void *stream) override;
  int GetMasterStats() override;
  void DumpStats(int target) override;
  uint8 HasFlags(int mask) override;
  void SetFlags(int mask) override;
  void ClearFlags(int mask) override;
  void TickFlags() override;
  void *GetDataSlot(int index) override;
  void SetDataSlot(int index, void *value) override;

private:
  static uint32 GetTimeMs();
  static void ExtractSenderAddress(const SLNet::SystemAddress &address, uint8 senderAddr[4], uint16 &senderPort);
  void ProcessIncomingPeer(SLNet::RakPeerInterface *peer, DispatchTarget source);

  bool m_initialized = false;
  bool m_masterConnected = false;
  bool m_worldConnected = false;

  NetworkAddress m_masterAddress{};
  NetworkAddress m_worldAddress{};
  NetworkAddress m_lastMasterAddress{};
  SLNet::SystemAddress m_masterSystem{};
  SLNet::SystemAddress m_worldSystem{};
  bool m_hasMasterAddress = false;
  uint32 m_masterRetryAtMs = 0;
  uint32 m_masterRetryDelayMs = 0;

  uint32 m_flags = 0;
  std::array<uint32, 32> m_flagTimes{};
  std::array<void *, 10> m_dataSlots{};

  SLNet::RakPeerInterface *m_masterPeer = nullptr;
  SLNet::RakPeerInterface *m_worldPeer = nullptr;
};

#endif // __CLTNETWORK_H__
