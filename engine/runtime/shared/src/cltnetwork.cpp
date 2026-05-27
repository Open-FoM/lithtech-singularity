// ------------------------------------------------------------------------- //
//
// FILE      : CLTNETWORK.CPP
//
// PURPOSE   : FoM-specific network interface implementation (engine side).
//
// ------------------------------------------------------------------------- //

#include "cltnetwork.h"

#include "clientnetworking.h"
#include "ltmodule.h"

namespace {
ClientNetworking &GetNetworking() {
  return GetClientNetworking();
}
} // namespace

CLTNetwork::CLTNetwork() = default;

bool CLTNetwork::IsMasterConnected() { return GetNetworking().IsMasterConnected(); }

bool CLTNetwork::IsWorldConnected() { return GetNetworking().IsWorldConnected(); }

NetResult CLTNetwork::Init(uint16 localPort) {
  if (GetNetworking().Init("", localPort)) {
    return NetResult::kOk;
  }
  return NetResult::kNotInitialized;
}

NetResult CLTNetwork::Shutdown() {
  GetNetworking().Shutdown();
  return NetResult::kOk;
}

void CLTNetwork::SubscribePacket(uint8 packetId, PacketRoute route) {
  (void)GetNetworking().RegisterPacketHandler(packetId, route);
}

void CLTNetwork::UnsubscribePacket(uint8 packetId, PacketRoute route) {
  (void)GetNetworking().RemovePacketHandler(packetId, route);
}

void CLTNetwork::ClearSubscriptions(PacketRoute route) { GetNetworking().ClearPacketHandlers(route); }

NetResult CLTNetwork::ConnectWorld(const NetworkAddress &address) { return GetNetworking().ConnectWorld(address); }

NetResult CLTNetwork::DisconnectAll() { return GetNetworking().DisconnectAll(); }

NetResult CLTNetwork::ConnectMaster(const NetworkAddress &address) { return GetNetworking().ConnectMaster(address); }

NetResult CLTNetwork::SendPacket(VariableSizedPacket *packet,
                                 NetPacketPriority priority,
                                 NetPacketReliability reliability,
                                 uint8 orderingChannel,
                                 DispatchTarget destination) {
  return GetNetworking().SendPacket(packet, priority, reliability, orderingChannel, destination);
}

void CLTNetwork::Update() { GetNetworking().Update(); }

void *CLTNetwork::GetDataSlot(int index) {
  if (index < 0 || static_cast<size_t>(index) >= m_dataSlots.size()) {
    return nullptr;
  }

  return m_dataSlots[static_cast<size_t>(index)];
}

void CLTNetwork::SetDataSlot(int index, void *value) {
  if (index < 0 || static_cast<size_t>(index) >= m_dataSlots.size()) {
    return;
  }

  m_dataSlots[static_cast<size_t>(index)] = value;
}

// Expose the engine interface implementation.
define_interface(CLTNetwork, ILTNetwork);
implements_also(CLTNetwork, Default, ILTNetwork, Client);
implements_also(CLTNetwork, Default, ILTNetwork, Server);
