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

class CLTNetwork : public ILTNetwork {
public:
  declare_interface(CLTNetwork);

  CLTNetwork();

  bool IsMasterConnected() override;
  bool IsWorldConnected() override;
  NetResult Init(uint16 localPort) override;
  NetResult Shutdown() override;
  void SubscribePacket(uint8 packetId, PacketRoute route) override;
  void UnsubscribePacket(uint8 packetId, PacketRoute route) override;
  void ClearSubscriptions(PacketRoute route) override;
  NetResult ConnectWorld(const NetworkAddress &address) override;
  NetResult DisconnectAll() override;
  NetResult ConnectMaster(const NetworkAddress &address) override;
  NetResult SendPacket(VariableSizedPacket *packet, NetPacketPriority priority, NetPacketReliability reliability,
                       uint8 orderingChannel, DispatchTarget destination) override;
  void Update() override;
  void *GetDataSlot(int index) override;
  void SetDataSlot(int index, void *value) override;

private:
  std::array<void *, 10> m_dataSlots{};
};

#endif // __CLTNETWORK_H__
