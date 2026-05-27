/**
 * Face of Mankind - ClientNetworking
 *
 * Implementation placeholder for engine-side networking logic.
 * See header for reverse-engineered entry points and addresses.
 */

#include "clientnetworking.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#include "ltmessage.h"
#include "ltnetwork_hooks.h"
#include "packet.h"

#include "slikenet/BitStream.h"
#include "slikenet/MessageIdentifiers.h"
#include "slikenet/peerinterface.h"
#include "slikenet/types.h"

struct ClientNetworkingSLikeNetState {
  SLNet::SystemAddress m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  SLNet::SystemAddress m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
};

namespace {
constexpr uint8 kFomTimestampMarker = 0x19;
constexpr size_t kFomTimestampPacketIdOffset = 9;
constexpr uint32 kConnectionRetryDelayMs = 5000;
constexpr uint8 kRakNetUnknown5f = 0x5F;
constexpr uint8 kLoginRequestReturn = 0x6D;

PacketPriority ToSLikeNetPriority(NetPacketPriority priority) {
  return static_cast<PacketPriority>(priority);
}

PacketReliability ToSLikeNetReliability(NetPacketReliability reliability) {
  return static_cast<PacketReliability>(reliability);
}

const NetworkAddress *GetPacketAddress(const void *packet) {
  if (packet == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<const NetworkAddress *>(static_cast<const uint8 *>(packet) + 4);
}

bool IsAssigned(const NetworkAddress &address) { return address.m_nIp != 0 || address.m_nPort != 0; }

uint8 ExtractPacketId(const uint8 *data, size_t length) {
  if (data == nullptr || length == 0) {
    return 0xFF;
  }

  if (data[0] == kFomTimestampMarker && length > kFomTimestampPacketIdOffset) {
    return data[kFomTimestampPacketIdOffset];
  }

  return data[0];
}

std::array<char, 16> FormatIpv4Host(const NetworkAddress &address) {
  std::array<char, 16> host{};
  uint8 octets[4] = {};
  std::memcpy(octets, &address.m_nIp, sizeof(octets));
  std::snprintf(host.data(), host.size(), "%u.%u.%u.%u",
                static_cast<unsigned int>(octets[0]),
                static_cast<unsigned int>(octets[1]),
                static_cast<unsigned int>(octets[2]),
                static_cast<unsigned int>(octets[3]));
  return host;
}

void FillNetworkAddress(const SLNet::SystemAddress &address, NetworkAddress &outAddress) {
  if (address.GetIPVersion() == 4) {
    uint32_t rawAddr = address.address.addr4.sin_addr.s_addr;
    std::memcpy(&outAddress.m_nIp, &rawAddr, sizeof(rawAddr));
    outAddress.m_nPort = address.GetPort();
    return;
  }
  outAddress = {};
}

bool IsSystemPacketId(uint8 packetId) {
  switch (packetId) {
  case ID_CONNECTION_REQUEST_ACCEPTED:
  case ID_CONNECTION_ATTEMPT_FAILED:
  case ID_ALREADY_CONNECTED:
  case ID_NO_FREE_INCOMING_CONNECTIONS:
  case ID_CONNECTION_BANNED:
  case ID_INVALID_PASSWORD:
  case ID_INCOMPATIBLE_PROTOCOL_VERSION:
  case ID_IP_RECENTLY_CONNECTED:
  case ID_DISCONNECTION_NOTIFICATION:
  case ID_CONNECTION_LOST:
  case kRakNetUnknown5f:
    return true;
  default:
    return false;
  }
}

PacketRoute MergePacketRoute(PacketRoute existingRoute, PacketRoute newRoute) {
  if (existingRoute == newRoute) {
    return existingRoute;
  }
  if (existingRoute == PacketRoute::kBoth || newRoute == PacketRoute::kBoth) {
    return PacketRoute::kBoth;
  }
  return PacketRoute::kBoth;
}

ILTMessage_Write *AllocateMessageWriter() {
  const auto clientAllocator = LTNetClientMessageWriteAllocator();
  if (clientAllocator) {
    return clientAllocator();
  }

  const auto serverAllocator = LTNetServerMessageWriteAllocator();
  if (serverAllocator) {
    return serverAllocator();
  }

  return nullptr;
}
} // namespace

ClientNetworking::ClientNetworking() : m_slikenetState(std::make_unique<ClientNetworkingSLikeNetState>()) {}

ClientNetworking::~ClientNetworking() { Shutdown(); }

bool ClientNetworking::Init(std::string_view masterHost, uint16 localPort) {
  if (m_bInitialized) {
    return true;
  }

  std::memset(m_MasterHostBuffer, 0, sizeof(m_MasterHostBuffer));
  const size_t hostLen = std::min(masterHost.size(), sizeof(m_MasterHostBuffer) - 1);
  std::memcpy(m_MasterHostBuffer, masterHost.data(), hostLen);
  m_nLocalPort = localPort;
  m_bInitialized = true;
  m_bMasterConnectInProgress = false;
  m_bWorldConnectInProgress = false;
  m_bWorldSwitchPending = false;
  m_lastMasterConnectAttemptMs = 0;
  m_lastWorldConnectAttemptMs = 0;

  m_masterPeer = SLNet::RakPeerInterface::GetInstance();
  m_worldPeer = SLNet::RakPeerInterface::GetInstance();
  m_slikenetState->m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_slikenetState->m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;

  if (!m_masterPeer || !m_worldPeer) {
    m_bInitialized = false;
    return false;
  }

  SLNet::SocketDescriptor masterDescriptor(localPort, nullptr);
  SLNet::SocketDescriptor worldDescriptor(0, nullptr);

  if (m_masterPeer->Startup(1, &masterDescriptor, 1) != SLNet::RAKNET_STARTED) {
    SLNet::RakPeerInterface::DestroyInstance(m_masterPeer);
    SLNet::RakPeerInterface::DestroyInstance(m_worldPeer);
    m_masterPeer = nullptr;
    m_worldPeer = nullptr;
    m_bInitialized = false;
    return false;
  }
  if (m_worldPeer->Startup(1, &worldDescriptor, 1) != SLNet::RAKNET_STARTED) {
    m_masterPeer->Shutdown(0);
    SLNet::RakPeerInterface::DestroyInstance(m_masterPeer);
    SLNet::RakPeerInterface::DestroyInstance(m_worldPeer);
    m_masterPeer = nullptr;
    m_worldPeer = nullptr;
    m_bInitialized = false;
    return false;
  }

  /**
   * @todo Fill in master/world auth setup using SLikeNet peers.
   * Engine path:
   *  - Load key from fom_public.key
   *  - Configure password "uh76Tg95" (peer auth) and "37eG87Ph" (connect)
   * Source: fom_client.exe .text 0x499960
   */
  return true;
}

void ClientNetworking::Shutdown() {
  m_bInitialized = false;
  m_bMasterConnectInProgress = false;
  m_bWorldConnectInProgress = false;
  m_bWorldSwitchPending = false;
  m_slikenetState->m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_slikenetState->m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  if (m_masterPeer != nullptr) {
    m_masterPeer->Shutdown(0);
    SLNet::RakPeerInterface::DestroyInstance(m_masterPeer);
    m_masterPeer = nullptr;
  }
  if (m_worldPeer != nullptr) {
    m_worldPeer->Shutdown(0);
    SLNet::RakPeerInterface::DestroyInstance(m_worldPeer);
    m_worldPeer = nullptr;
  }
}

bool ClientNetworking::HasMasterAddress() const { return IsAssigned(m_MasterAddress); }

bool ClientNetworking::HasWorldAddress() const { return IsAssigned(m_WorldAddress); }

bool ClientNetworking::IsMasterConnected() const { return m_slikenetState->m_masterSystem != SLNet::UNASSIGNED_SYSTEM_ADDRESS; }

bool ClientNetworking::IsWorldConnected() const { return m_slikenetState->m_worldSystem != SLNet::UNASSIGNED_SYSTEM_ADDRESS; }

int32 ClientNetworking::GetWorldConnectionId() const {
  /**
   * @todo Map to RakNet peer lookup by SystemAddress.
   * Source: fom_client.exe .text 0x499E20
   */
  return -1;
}

bool ClientNetworking::ExpireWorldTargetIfStale() {
  /**
   * @todo Implement world target timeout (10s) using m_WorldTargetTimer.
   * Source: fom_client.exe .text 0x499C30
   */
  return false;
}

NetResult ClientNetworking::ConnectMaster(const NetworkAddress &address) {
  if (!m_bInitialized || !m_masterPeer) {
    return NetResult::kNotInitialized;
  }
  if (!IsAssigned(address)) {
    return NetResult::kInvalidArgs;
  }

  const auto hostAddress = FormatIpv4Host(address);
  m_lastMasterConnectAttemptMs = GetTimeMs();
  if (m_masterPeer->Connect(hostAddress.data(), address.m_nPort, nullptr, 0) != SLNet::CONNECTION_ATTEMPT_STARTED) {
    return NetResult::kSendFailed;
  }

  m_MasterAddress = address;
  m_bMasterConnectInProgress = true;
  return NetResult::kOk;
}

NetResult ClientNetworking::ConnectWorld(const NetworkAddress &address) {
  if (!m_bInitialized || !m_worldPeer) {
    return NetResult::kNotInitialized;
  }
  if (!IsAssigned(address)) {
    return NetResult::kInvalidArgs;
  }

  if (!IsMasterConnected()) {
    return NetResult::kNotConnected;
  }

  const auto hostAddress = FormatIpv4Host(address);
  m_lastWorldConnectAttemptMs = GetTimeMs();
  if (m_worldPeer->Connect(hostAddress.data(), address.m_nPort, nullptr, 0) != SLNet::CONNECTION_ATTEMPT_STARTED) {
    return NetResult::kSendFailed;
  }

  m_WorldAddress = address;
  m_bWorldConnectInProgress = true;
  return NetResult::kOk;
}

NetResult ClientNetworking::DisconnectAll() {
  if (!m_bInitialized) {
    return NetResult::kNotInitialized;
  }

  if (m_masterPeer && m_slikenetState->m_masterSystem != SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
    m_masterPeer->CloseConnection(m_slikenetState->m_masterSystem, true);
  }
  if (m_worldPeer && m_slikenetState->m_worldSystem != SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
    m_worldPeer->CloseConnection(m_slikenetState->m_worldSystem, true);
  }

  m_MasterAddress = {};
  m_WorldAddress = {};
  m_WorldTarget = {};
  m_slikenetState->m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_slikenetState->m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_bMasterConnectInProgress = false;
  m_bWorldConnectInProgress = false;
  m_bWorldSwitchPending = false;
  m_lastMasterConnectAttemptMs = 0;
  m_lastWorldConnectAttemptMs = 0;

  return NetResult::kOk;
}

NetResult ClientNetworking::SendPacket(VariableSizedPacket *packet,
                                       NetPacketPriority priority,
                                       NetPacketReliability reliability,
                                       uint8 orderingChannel,
                                       DispatchTarget destination) {
  if (!m_bInitialized) {
    return NetResult::kNotInitialized;
  }

  if (packet == nullptr) {
    return NetResult::kInvalidArgs;
  }

  CLTMsgRef_Write writerRef(AllocateMessageWriter());
  auto *writer = static_cast<ILTMessage_Write *>(writerRef);
  if (!writer) {
    return NetResult::kNotInitialized;
  }

  writer->Reset();
  if (!packet->Write(writer)) {
    return NetResult::kSendFailed;
  }

  auto *ltWriter = static_cast<CLTMessage_Write *>(writer);
  CPacket_Read packetRead(ltWriter->GetPacket());
  const auto bitCount = packetRead.Size();
  if (bitCount == 0) {
    return NetResult::kInvalidArgs;
  }

  const auto byteCount = (bitCount + 7) / 8;
  std::vector<uint8_t> buffer(byteCount);
  packetRead.ReadData(buffer.data(), bitCount);

  SLNet::BitStream bitStream(buffer.data(), static_cast<unsigned int>(buffer.size()), false);
  SLNet::RakPeerInterface *peer = (destination == DispatchTarget::kWorld) ? m_worldPeer : m_masterPeer;
  if (!peer) {
    return NetResult::kNotInitialized;
  }

  SLNet::SystemAddress systemAddress =
      (destination == DispatchTarget::kWorld) ? m_slikenetState->m_worldSystem : m_slikenetState->m_masterSystem;
  if (systemAddress == SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
    systemAddress = peer->GetSystemAddressFromIndex(0);
  }
  if (systemAddress == SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
    return NetResult::kNotConnected;
  }

  const auto sendReceipt = peer->Send(&bitStream, ToSLikeNetPriority(priority), ToSLikeNetReliability(reliability),
                                      orderingChannel, systemAddress, false);

  return sendReceipt == 0 ? NetResult::kSendFailed : NetResult::kOk;
}

void ClientNetworking::Update() {
  if (!m_bInitialized) {
    return;
  }

  const uint32 now = GetTimeMs();
  if (now == m_lastUpdateMs) {
    return;
  }
  m_lastUpdateMs = now;

  TickMasterConnection(IsMasterConnected());
  TickWorldConnection();
  (void)ExpireWorldTargetIfStale();
  (void)Receive();
}

void ClientNetworking::TickMasterConnection(bool isMasterConnected) {
  if (!m_bInitialized) {
    return;
  }

  if (isMasterConnected) {
    m_bMasterConnectInProgress = false;
    return;
  }

  if (m_bMasterConnectInProgress || !HasMasterAddress()) {
    return;
  }

  const uint32 now = GetTimeMs();
  if (m_lastMasterConnectAttemptMs != 0 && now - m_lastMasterConnectAttemptMs < kConnectionRetryDelayMs) {
    return;
  }

  (void)ConnectMaster(m_MasterAddress);
}

void ClientNetworking::TickWorldConnection() {
  if (!m_bInitialized) {
    return;
  }

  if (IsWorldConnected()) {
    m_bWorldConnectInProgress = false;
    return;
  }

  if (m_bWorldConnectInProgress || !IsAssigned(m_WorldTarget)) {
    return;
  }

  const uint32 now = GetTimeMs();
  if (m_lastWorldConnectAttemptMs != 0 && now - m_lastWorldConnectAttemptMs < kConnectionRetryDelayMs) {
    return;
  }

  (void)ConnectWorld(m_WorldTarget);
}

bool ClientNetworking::Receive() {
  if (!m_bInitialized) {
    return false;
  }

  ReceivePeer(m_masterPeer, NetworkTarget::Master);
  ReceivePeer(m_worldPeer, NetworkTarget::World);
  return true;
}

PacketDispatchResult ClientNetworking::DispatchPacket(uint8 packetId, NetworkTarget target, void *packet) {
  if (!m_bInitialized || packetId == 0xFF || packet == nullptr) {
    return PacketDispatchResult::Ignored;
  }

  const NetworkAddress *packetAddress = GetPacketAddress(packet);

  if (IsSystemPacketId(packetId)) {
    switch (packetId) {
    case ID_CONNECTION_REQUEST_ACCEPTED:
      if (target == NetworkTarget::Master) {
        if (packetAddress != nullptr) {
          m_MasterAddress = *packetAddress;
        }
        m_bMasterConnectInProgress = false;
      } else {
        if (packetAddress != nullptr) {
          m_WorldAddress = *packetAddress;
        }
        m_WorldTarget = {};
        m_bWorldConnectInProgress = false;
        m_bWorldSwitchPending = false;
      }
      return PacketDispatchResult::Handled;
    case ID_CONNECTION_ATTEMPT_FAILED:
    case ID_ALREADY_CONNECTED:
    case ID_NO_FREE_INCOMING_CONNECTIONS:
    case ID_CONNECTION_BANNED:
    case ID_INVALID_PASSWORD:
    case ID_INCOMPATIBLE_PROTOCOL_VERSION:
    case ID_IP_RECENTLY_CONNECTED:
    case kRakNetUnknown5f:
      if (target == NetworkTarget::Master) {
        m_slikenetState->m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
        m_bMasterConnectInProgress = false;
      } else {
        m_slikenetState->m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
        m_bWorldConnectInProgress = false;
        m_bWorldSwitchPending = false;
      }
      return PacketDispatchResult::Handled;
    case ID_DISCONNECTION_NOTIFICATION:
    case ID_CONNECTION_LOST:
      if (target == NetworkTarget::Master) {
        m_MasterAddress = {};
        m_slikenetState->m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
        m_bMasterConnectInProgress = false;
      } else {
        m_WorldAddress = {};
        m_WorldTarget = {};
        m_slikenetState->m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
        m_bWorldConnectInProgress = false;
        m_bWorldSwitchPending = false;
      }
      return PacketDispatchResult::Handled;
    default:
      return PacketDispatchResult::Handled;
    }
  }

  if (packetId == kLoginRequestReturn && HandleLoginRequestReturn(packet)) {
    return PacketDispatchResult::Handled;
  }

  const auto route = GetPacketRoute(packetId);
  if (!route.has_value()) {
    return PacketDispatchResult::Ignored;
  }
  switch (route.value()) {
  case PacketRoute::kServer: return PacketDispatchResult::RouteServer;
  case PacketRoute::kBoth: return PacketDispatchResult::RouteBoth;
  default: return PacketDispatchResult::RouteClient;
  }
}

bool ClientNetworking::RequestWorldConnect(const NetworkAddress &address) {
  if (!m_bInitialized) {
    /**
     * @todo Log "Not initialized!!!"
     * Source: fom_client.exe .text 0x49AA80
     */
    return false;
  }

  if (!IsMasterConnected()) {
    /**
     * @todo Log "Attempting to connect to world, but not connected to master server!!!"
     * Source: fom_client.exe .text 0x49AAD4
     */
    return false;
  }

  m_bWorldSwitchPending = IsAssigned(m_WorldAddress);
  m_WorldTarget = address;

  /**
   * @todo Reset world connect timer (PerfTimer_Reset at this+0x70).
   * Source: fom_client.exe .text 0x49AB41
   */
  return true;
}

bool ClientNetworking::CloseMasterConnection() {
  if (!m_bInitialized) {
    return false;
  }

  if (m_masterPeer && m_slikenetState->m_masterSystem != SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
    m_masterPeer->CloseConnection(m_slikenetState->m_masterSystem, true);
  }

  m_bMasterConnectInProgress = false;
  m_MasterAddress = {};
  m_slikenetState->m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_lastMasterConnectAttemptMs = 0;
  return true;
}

bool ClientNetworking::CloseWorldConnection() {
  if (!m_bInitialized) {
    return false;
  }

  if (m_worldPeer && m_slikenetState->m_worldSystem != SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
    m_worldPeer->CloseConnection(m_slikenetState->m_worldSystem, true);
  }

  m_bWorldConnectInProgress = false;
  m_WorldAddress = {};
  m_WorldTarget = {};
  m_slikenetState->m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_lastWorldConnectAttemptMs = 0;
  return true;
}

bool ClientNetworking::SendPacket(VariableSizedPacket *packet, NetworkTarget target, uint8 priority, uint8 reliability,
                                  uint8 orderingChannel, bool logPacket) {
  (void)logPacket;
  const DispatchTarget destination =
      (target == NetworkTarget::World) ? DispatchTarget::kWorld : DispatchTarget::kMaster;
  const auto result = SendPacket(packet, static_cast<NetPacketPriority>(priority),
                                 static_cast<NetPacketReliability>(reliability), orderingChannel, destination);
  return result == NetResult::kOk;
}

bool ClientNetworking::HandleLoginRequestReturn(void *packet) {
  if (!m_bInitialized || packet == nullptr) {
    return false;
  }

  /**
   * @todo Parse login request return, construct Packet_ID_LOGIN, send to master.
   * Source: fom_client.exe .text 0x49CA70
   */
  return false;
}

bool ClientNetworking::RegisterPacketHandler(uint8 packetId, PacketRoute route) {
  if (packetId == 0) {
    return false;
  }

  for (auto &entry : m_PacketHandlers) {
    if (entry.m_Id == packetId) {
      entry.m_Route = MergePacketRoute(entry.m_Route, route);
      return true;
    }
  }

  for (auto &entry : m_PacketHandlers) {
    if (entry.m_Id == 0) {
      entry.m_Id = packetId;
      entry.m_Route = route;
      return true;
    }
  }

  return false;
}

bool ClientNetworking::RemovePacketHandler(uint8 packetId, PacketRoute route) {
  for (auto &entry : m_PacketHandlers) {
    if (entry.m_Id != packetId) {
      continue;
    }

    if (route == PacketRoute::kBoth || entry.m_Route == route) {
      entry.m_Id = 0;
      entry.m_Route = PacketRoute::kClient;
      return true;
    }

    if (entry.m_Route == PacketRoute::kBoth) {
      entry.m_Route = (route == PacketRoute::kClient) ? PacketRoute::kServer : PacketRoute::kClient;
      return true;
    }
  }

  return false;
}

void ClientNetworking::ClearPacketHandlers(PacketRoute route) {
  for (auto &entry : m_PacketHandlers) {
    if (entry.m_Id == 0) {
      continue;
    }

    if (route == PacketRoute::kBoth || entry.m_Route == route) {
      entry.m_Id = 0;
      entry.m_Route = PacketRoute::kClient;
      continue;
    }

    if (entry.m_Route == PacketRoute::kBoth) {
      entry.m_Route = (route == PacketRoute::kClient) ? PacketRoute::kServer : PacketRoute::kClient;
    }
  }
}

std::optional<PacketRoute> ClientNetworking::GetPacketRoute(uint8 packetId) const {
  const int32 index = FindHandlerIndex(packetId);
  if (index < 0) {
    return std::nullopt;
  }
  return m_PacketHandlers[static_cast<size_t>(index)].m_Route;
}

int32 ClientNetworking::FindHandlerIndex(uint8 packetId) const {
  for (size_t i = 0; i < m_PacketHandlers.size(); ++i) {
    if (m_PacketHandlers[i].m_Id == packetId) {
      return static_cast<int32>(i);
    }
  }

  return -1;
}

void ClientNetworking::ReceivePeer(SLNet::RakPeerInterface *peer, NetworkTarget target) {
  if (!peer) {
    return;
  }

  for (auto *packet = peer->Receive(); packet; packet = peer->Receive()) {
    if (!packet->data || packet->length == 0) {
      peer->DeallocatePacket(packet);
      continue;
    }

    const uint8 packetId = ExtractPacketId(packet->data, packet->length);
    if (packetId == ID_CONNECTION_REQUEST_ACCEPTED) {
      if (target == NetworkTarget::Master) {
        m_slikenetState->m_masterSystem = packet->systemAddress;
      } else {
        m_slikenetState->m_worldSystem = packet->systemAddress;
      }
    } else if (packetId == ID_CONNECTION_LOST || packetId == ID_DISCONNECTION_NOTIFICATION ||
               packetId == ID_CONNECTION_ATTEMPT_FAILED || packetId == ID_ALREADY_CONNECTED ||
               packetId == ID_NO_FREE_INCOMING_CONNECTIONS || packetId == ID_CONNECTION_BANNED ||
               packetId == ID_INVALID_PASSWORD || packetId == ID_INCOMPATIBLE_PROTOCOL_VERSION ||
               packetId == ID_IP_RECENTLY_CONNECTED || packetId == kRakNetUnknown5f) {
      if (target == NetworkTarget::Master) {
        m_slikenetState->m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
      } else {
        m_slikenetState->m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
      }
    }

    struct PacketEnvelope {
      uint32 m_pad = 0;
      NetworkAddress m_address{};
    } envelope;

    FillNetworkAddress(packet->systemAddress, envelope.m_address);

    const PacketDispatchResult dispatchResult = DispatchPacket(packetId, target, &envelope);
    if (dispatchResult == PacketDispatchResult::RouteClient || dispatchResult == PacketDispatchResult::RouteServer ||
        dispatchResult == PacketDispatchResult::RouteBoth) {
      CPacket_Write packetWrite;
      packetWrite.WriteDataRaw(packet->data, packet->length);

      uint8 senderAddr[4] = {0, 0, 0, 0};
      uint16 senderPort = 0;
      std::memcpy(senderAddr, &envelope.m_address.m_nIp, sizeof(senderAddr));
      senderPort = envelope.m_address.m_nPort;

      const auto clientInjector = LTNetClientPacketInjector();
      const auto serverInjector = LTNetServerPacketInjector();

      if (dispatchResult == PacketDispatchResult::RouteClient || dispatchResult == PacketDispatchResult::RouteBoth) {
        if (clientInjector) {
          CPacket_Read packetRead(packetWrite);
          clientInjector(packetRead, senderAddr, senderPort);
        }
      }
      if (dispatchResult == PacketDispatchResult::RouteServer || dispatchResult == PacketDispatchResult::RouteBoth) {
        if (serverInjector) {
          CPacket_Read packetRead(packetWrite);
          serverInjector(packetRead, senderAddr, senderPort);
        }
      }
    }

    peer->DeallocatePacket(packet);
  }
}

uint32 ClientNetworking::GetTimeMs() {
  using clock = std::chrono::steady_clock;
  using ms = std::chrono::milliseconds;
  return static_cast<uint32>(std::chrono::duration_cast<ms>(clock::now().time_since_epoch()).count());
}

ClientNetworking &GetClientNetworking() {
  static ClientNetworking instance;
  return instance;
}
