/**
 * Face of Mankind - ClientNetworking
 *
 * Engine-side client networking manager reconstructed from fom_client.exe.
 * Owns connections to master and world servers and dispatches inbound packets.
 *
 * Source: fom_client.exe (engine)
 * - Init: .text 0x499960
 * - Receive: .text 0x499C90
 * - TickMasterConnection: .text 0x49A9C0
 * - WorldConnect: .text 0x49AB70
 * - SendPacket: .text 0x49AF40
 * - HandleLoginRequestReturn: .text 0x49CA70
 */

#ifndef _FOM_CLIENTNETWORKING_H_
#define _FOM_CLIENTNETWORKING_H_

#include "iltnetwork.h"
#include "ltbasetypes.h"
#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace SLNet {
class RakPeerInterface;
struct SystemAddress;
} // namespace SLNet

enum class NetworkTarget : uint8 { Master = 1, World = 2 };

struct PacketHandlerEntry {
  uint8 m_Id = 0;
  PacketRoute m_Route = PacketRoute::kClient;
};

enum class PacketDispatchResult : uint8 { Ignored, Handled, RouteClient, RouteServer, RouteBoth };

struct ClientNetworkingSLikeNetState;

/**
 * PerfTimer
 *
 * Minimal reconstruction of the engine timing helper used by ClientNetworking.
 * UpdateElapsed writes elapsed seconds to +0x10 in the engine binary.
 *
 * Source: fom_client.exe PerfTimer_UpdateElapsed/Reset + ClientNetworking timing usage
 */
struct PerfTimer {
  uint64 m_LastCounter = 0;
  uint64 m_Frequency = 0;
  double m_fElapsedSeconds = 0.0;
};

/**
 * ClientNetworking
 *
 * Wraps master/world RakNet peers and provides packet dispatch.
 * This reconstruction focuses on the state and flow observed in the engine,
 * but leaves the RakNet integration as a later dependency.
 */
class ClientNetworking {
public:
  ClientNetworking();
  virtual ~ClientNetworking();

  ClientNetworking(const ClientNetworking &) = delete;
  ClientNetworking &operator=(const ClientNetworking &) = delete;

  [[nodiscard]] bool Init(std::string_view masterHost, uint16 localPort);
  void Shutdown();

  [[nodiscard]] bool IsInitialized() const { return m_bInitialized; }
  [[nodiscard]] bool IsMasterConnected() const;
  [[nodiscard]] bool IsWorldConnected() const;
  [[nodiscard]] std::string_view GetMasterHost() const { return std::string_view(m_MasterHostBuffer); }

  void SetMasterAddress(const NetworkAddress &address) { m_MasterAddress = address; }
  void SetWorldAddress(const NetworkAddress &address) { m_WorldAddress = address; }
  void SetWorldTarget(const NetworkAddress &address) { m_WorldTarget = address; }

  [[nodiscard]] const NetworkAddress &GetMasterAddress() const { return m_MasterAddress; }
  [[nodiscard]] const NetworkAddress &GetWorldAddress() const { return m_WorldAddress; }
  [[nodiscard]] const NetworkAddress &GetWorldTarget() const { return m_WorldTarget; }

  [[nodiscard]] bool HasMasterAddress() const;
  [[nodiscard]] bool HasWorldAddress() const;
  [[nodiscard]] int32 GetWorldConnectionId() const;

  [[nodiscard]] bool ExpireWorldTargetIfStale();
  void TickMasterConnection(bool isMasterConnected);
  void TickWorldConnection();
  [[nodiscard]] bool Receive();

  [[nodiscard]] PacketDispatchResult DispatchPacket(uint8 packetId, NetworkTarget target, void *packet);
  [[nodiscard]] bool RegisterPacketHandler(uint8 packetId, PacketRoute route);
  [[nodiscard]] bool RemovePacketHandler(uint8 packetId, PacketRoute route);
  void ClearPacketHandlers(PacketRoute route);
  [[nodiscard]] std::optional<PacketRoute> GetPacketRoute(uint8 packetId) const;

  NetResult ConnectMaster(const NetworkAddress &address);
  NetResult ConnectWorld(const NetworkAddress &address);
  NetResult DisconnectAll();

  NetResult SendPacket(VariableSizedPacket *packet, NetPacketPriority priority, NetPacketReliability reliability,
                       uint8 orderingChannel, DispatchTarget destination);
  void Update();

  [[nodiscard]] bool RequestWorldConnect(const NetworkAddress &address);
  [[nodiscard]] bool CloseMasterConnection();
  [[nodiscard]] bool CloseWorldConnection();

  [[nodiscard]] bool SendPacket(VariableSizedPacket *packet, NetworkTarget target, uint8 priority, uint8 reliability,
                                uint8 orderingChannel, bool logPacket);

  [[nodiscard]] bool HandleLoginRequestReturn(void *packet);

private:
  [[nodiscard]] int32 FindHandlerIndex(uint8 packetId) const;
  void ReceivePeer(SLNet::RakPeerInterface *peer, NetworkTarget target);
  static uint32 GetTimeMs();
  bool m_bInitialized = false;
  bool m_bMasterConnectInProgress = false;
  bool m_bWorldConnectInProgress = false;
  bool m_bWorldSwitchPending = false;

  uint16 m_nLocalPort = 0;

  SLNet::RakPeerInterface *m_masterPeer = nullptr;
  SLNet::RakPeerInterface *m_worldPeer = nullptr;
  std::unique_ptr<ClientNetworkingSLikeNetState> m_slikenetState;

  NetworkAddress m_MasterAddress;
  NetworkAddress m_WorldAddress;
  NetworkAddress m_WorldTarget;
  uint32 m_lastUpdateMs = 0;
  uint32 m_lastMasterConnectAttemptMs = 0;
  uint32 m_lastWorldConnectAttemptMs = 0;

  PerfTimer m_MasterConnectTimer;
  PerfTimer m_WorldConnectTimer;
  PerfTimer m_WorldTargetTimer;

  char m_LoginUsername[64] = {};
  char m_LoginPassword[64] = {};
  char m_LegacyHostName[64] = {};
  char m_MasterHostBuffer[64] = {};
  uint8 m_SteamTicketBlob[1024] = {};
  uint32 m_SteamTicketLength = 0;
  bool m_bHasSteamTicket = false;
  std::vector<uint32> m_FileCrcs;
  std::array<PacketHandlerEntry, 0xC8> m_PacketHandlers{};
};

ClientNetworking &GetClientNetworking();

#endif // _FOM_CLIENTNETWORKING_H_
