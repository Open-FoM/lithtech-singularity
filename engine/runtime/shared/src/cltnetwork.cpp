// ------------------------------------------------------------------------- //
//
// FILE      : CLTNETWORK.CPP
//
// PURPOSE   : FoM-specific network interface implementation (engine side).
//
// ------------------------------------------------------------------------- //

#include "cltnetwork.h"

#include <chrono>
#include <cstring>
#include <vector>

#include "ltmodule.h"
#include "ltmessage.h"
#include "ltnetwork_hooks.h"
#include "packet.h"

#include "slikenet/BitStream.h"
#include "slikenet/MessageIdentifiers.h"
#include "slikenet/RakPeerInterface.h"
#include "slikenet/peerinterface.h"
#include "slikenet/SocketLayer.h"
#include "slikenet/types.h"

namespace {
constexpr int kNetNotInitialized = 73;
constexpr uint32 kFlagTimeoutMs = 0x1388;

LTNetPacketInjector g_clientPacketInjector = nullptr;
LTNetPacketInjector g_serverPacketInjector = nullptr;
LTNetMessageWriteAllocator g_clientMessageWriteAllocator = nullptr;
LTNetMessageWriteAllocator g_serverMessageWriteAllocator = nullptr;

SLNet::PacketPriority ToSLikeNetPriority(PacketPriority priority) {
  return static_cast<SLNet::PacketPriority>(priority);
}

SLNet::PacketReliability ToSLikeNetReliability(PacketReliability reliability) {
  return static_cast<SLNet::PacketReliability>(reliability);
}

bool ShouldForwardToUserland(const SLNet::Packet *packet) {
  if (!packet || !packet->data || packet->length == 0) {
    return false;
  }

  const auto messageId = static_cast<unsigned char>(packet->data[0]);
  return messageId >= ID_USER_PACKET_ENUM;
}

ILTMessage_Write *AllocateMessageWriter() {
  if (g_clientMessageWriteAllocator) {
    return g_clientMessageWriteAllocator();
  }
  if (g_serverMessageWriteAllocator) {
    return g_serverMessageWriteAllocator();
  }
  return nullptr;
}
} // namespace

void LTNet_SetClientPacketInjector(LTNetPacketInjector injector) { g_clientPacketInjector = injector; }
void LTNet_SetServerPacketInjector(LTNetPacketInjector injector) { g_serverPacketInjector = injector; }
void LTNet_SetClientMessageWriteAllocator(LTNetMessageWriteAllocator allocator) {
  g_clientMessageWriteAllocator = allocator;
}
void LTNet_SetServerMessageWriteAllocator(LTNetMessageWriteAllocator allocator) {
  g_serverMessageWriteAllocator = allocator;
}

CLTNetwork::CLTNetwork() = default;

const char *CLTNetwork::_InterfaceImplementation() { return "CLTNetwork"; }

bool CLTNetwork::IsMasterConnected() { return m_masterConnected; }

bool CLTNetwork::IsWorldConnected() { return m_worldConnected; }

/// Decomp refs:
/// - Docs/Notes/ClientNetworking.md (init sequence notes)
LTRESULT CLTNetwork::InitNetwork(uint16 localPort) {
  if (m_initialized) {
    return LT_OK;
  }

  m_masterPeer = SLNet::RakPeerInterface::GetInstance();
  m_worldPeer = SLNet::RakPeerInterface::GetInstance();
  if (!m_masterPeer || !m_worldPeer) {
    return LT_ERROR;
  }

  SLNet::SocketDescriptor masterDescriptor(localPort, nullptr);
  SLNet::SocketDescriptor worldDescriptor(0, nullptr);

  if (m_masterPeer->Startup(1, &masterDescriptor, 1) != SLNet::RAKNET_STARTED) {
    return LT_ERROR;
  }
  if (m_worldPeer->Startup(1, &worldDescriptor, 1) != SLNet::RAKNET_STARTED) {
    return LT_ERROR;
  }

  m_initialized = true;
  return LT_OK;
}

LTRESULT CLTNetwork::ShutdownNetwork() {
  if (!m_initialized) {
    return LT_ERROR;
  }

  if (m_masterPeer) {
    m_masterPeer->Shutdown(0);
    SLNet::RakPeerInterface::DestroyInstance(m_masterPeer);
    m_masterPeer = nullptr;
  }
  if (m_worldPeer) {
    m_worldPeer->Shutdown(0);
    SLNet::RakPeerInterface::DestroyInstance(m_worldPeer);
    m_worldPeer = nullptr;
  }

  m_initialized = false;
  m_masterConnected = false;
  m_worldConnected = false;
  m_masterAddress = {};
  m_worldAddress = {};
  m_lastMasterAddress = {};
  m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;

  m_flags = 0;
  m_flagTimes.fill(0);

  for (auto &slot : m_dataSlots) {
    slot = nullptr;
  }

  m_user.clear();
  m_password.clear();
  m_token.clear();
  m_payload.clear();
  m_loginFlags = 0;

  return LT_OK;
}

bool CLTNetwork::SetNotificationFlag(int id, int group) {
  (void)id;
  (void)group;

  if (!m_initialized) {
    return false;
  }

  // Pseudocode (RakNet):
  // - Map (id, group) to a notification bit or channel.
  // - Enable in net driver so receive loop signals the client shell.
  return true;
}

LTRESULT CLTNetwork::ConnectWorld(const NetworkAddress &address) {
  if (!m_initialized) {
    return LT_ERROR;
  }

  if (!m_masterConnected) {
    return LT_ERROR;
  }

  if (!m_worldPeer) {
    return LT_ERROR;
  }

  const char *hostAddress = SLNet::SocketLayer::InAddrToString(address.m_nIp);
  if (m_worldPeer->Connect(hostAddress, address.m_nPort, nullptr, 0) != SLNet::CONNECTION_ATTEMPT_STARTED) {
    return LT_ERROR;
  }
  m_worldAddress = address;
  return LT_OK;
}

LTRESULT CLTNetwork::Disconnect() {
  if (!m_initialized) {
    return LT_ERROR;
  }

  if (m_masterPeer) {
    m_masterPeer->Shutdown(0);
  }
  if (m_worldPeer) {
    m_worldPeer->Shutdown(0);
  }
  m_masterConnected = false;
  m_worldConnected = false;
  m_masterAddress = {};
  m_worldAddress = {};
  m_lastMasterAddress = {};
  m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
  m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;

  return LT_OK;
}

bool CLTNetwork::PollMasterAddress() {
  if (!m_initialized) {
    return false;
  }

  // Pseudocode (RakNet):
  // - If master peer connected, pull SystemAddress for master.
  // - Update m_lastMasterAddress when it changes.
  return m_masterConnected;
}

/// Decomp refs:
/// - Docs/Notes/Login_Flow.md
LTRESULT CLTNetwork::BeginLogin(const char *user, const char *password, uint16 port, int loginFlags, const char *token,
                                bool hasPayload, const void *payload, int payloadSize) {
  (void)port;

  if (!m_initialized) {
    return LT_ERROR;
  }

  if (!user || !password || !token) {
    return LT_ERROR;
  }

  m_user = user;
  m_password = password;
  m_token = token;
  m_loginFlags = loginFlags;

  m_payload.clear();
  if (hasPayload && payload && payloadSize > 0) {
    const auto *bytes = static_cast<const uint8_t *>(payload);
    m_payload.assign(bytes, bytes + payloadSize);
  }

  // Pseudocode (RakNet):
  // - Build LOGIN_REQUEST packet (user, password, port, flags, token, payload).
  // - Serialize to BitStream / VariableSizedPacket.
  // - DispatchPacket(packet, kHigh, kReliableOrdered, channel=0, kMaster).
  return LT_OK;
}

/// Decomp refs:
/// - IDA Object.lto: sub_74AF40 @ 0x74AF40
/// - IDA Object.lto: sub_79B670 @ 0x79B670
/// - IDA Object.lto: sub_7A1DF0 @ 0x7A1DF0
/// - Source/Object.lto.c:92872 (RakPeer_Send)
/// - Source/Object.lto.c:92933 (RakPeer_SendBitStream)
LTRESULT CLTNetwork::DispatchPacket(VariableSizedPacket *packet, PacketPriority priority, PacketReliability reliability,
                                    uint8 orderingChannel, DispatchTarget destination) {
  if (!m_initialized) {
    return LT_ERROR;
  }

  if (packet == nullptr) {
    return LT_ERROR;
  }

  auto *writer = AllocateMessageWriter();
  if (!writer) {
    return LT_ERROR;
  }

  if (destination == DispatchTarget::kWorld && !m_worldConnected) {
    writer->Release();
    return LT_ERROR;
  }
  if (destination == DispatchTarget::kMaster && !m_masterConnected) {
    writer->Release();
    return LT_ERROR;
  }

  writer->Reset();
  packet->Write(*writer);

  auto *ltWriter = static_cast<CLTMessage_Write *>(writer);
  CPacket_Read packetRead(ltWriter->GetPacket());
  const auto bitCount = packetRead.Size();
  if (bitCount == 0) {
    writer->Release();
    return LT_ERROR;
  }

  const auto byteCount = (bitCount + 7) / 8;
  std::vector<uint8_t> buffer(byteCount);
  packetRead.ReadData(buffer.data(), bitCount);

  SLNet::BitStream bitStream(buffer.data(), static_cast<unsigned int>(buffer.size()), false);
  SLNet::RakPeerInterface *peer = (destination == DispatchTarget::kWorld) ? m_worldPeer : m_masterPeer;
  if (!peer) {
    writer->Release();
    return LT_ERROR;
  }

  const auto &systemAddress = (destination == DispatchTarget::kWorld) ? m_worldSystem : m_masterSystem;
  if (systemAddress == SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
    writer->Release();
    return LT_ERROR;
  }

  peer->Send(&bitStream, ToSLikeNetPriority(priority), ToSLikeNetReliability(reliability), orderingChannel, systemAddress,
             false);

  writer->Release();
  return LT_OK;
}

LTRESULT CLTNetwork::SendHandshake(const NetworkAddress &address) {
  (void)address;

  if (!m_initialized) {
    return LT_ERROR;
  }

  // Pseudocode (RakNet):
  // - Build handshake/login-init packet.
  // - Send to master with RELIABLE_ORDERED.
  return LT_OK;
}

/// Decomp refs:
/// - Docs/AddressMaps/AddressMap.md (RakPeer recv loop)
void CLTNetwork::UpdateNetwork() {
  if (!m_initialized) {
    return;
  }

  ProcessIncomingPeer(m_masterPeer, DispatchTarget::kMaster);
  ProcessIncomingPeer(m_worldPeer, DispatchTarget::kWorld);
  // - Update connection flags (NEW_INCOMING_CONNECTION / DISCONNECTION).
  TickFlags();
}

/// Decomp refs:
/// - IDA Object.lto: 0x767F20 (write bitstream helper)
int CLTNetwork::WriteBitStream(void *src, int maxBits, void *stream) {
  if (!m_initialized || !stream || !src || maxBits <= 0) {
    return kNetNotInitialized;
  }

  auto *bitStream = static_cast<SLNet::BitStream *>(stream);
  bitStream->WriteBits(reinterpret_cast<const unsigned char *>(src), maxBits, true);
  return maxBits;
}

/// Decomp refs:
/// - IDA Object.lto: 0x767F50 (read bitstream helper)
int CLTNetwork::ReadBitStream(void *dst, int maxBits, void *stream) {
  if (!m_initialized || !stream || !dst || maxBits <= 0) {
    return kNetNotInitialized;
  }

  auto *bitStream = static_cast<SLNet::BitStream *>(stream);
  bitStream->ReadBits(reinterpret_cast<unsigned char *>(dst), maxBits, true);
  return maxBits;
}

int CLTNetwork::GetMasterStats() {
  if (!m_initialized) {
    return kNetNotInitialized;
  }

  // Pseudocode (RakNet):
  // - Query RakPeer statistics for master connection.
  // - Return ping/packet loss or an opaque handle used by the shell.
  return -1;
}

void CLTNetwork::DumpStats(int target) {
  (void)target;

  if (!m_initialized) {
    return;
  }

  // Pseudocode (RakNet):
  // - Gather RakPeer::GetStatistics for master/world.
  // - Log bytes/packets in/out, loss, and ping.
}

uint8 CLTNetwork::HasFlags(int mask) { return (m_flags & mask) != 0 ? 1 : 0; }

void CLTNetwork::SetFlags(int mask) {
  m_flags |= static_cast<uint32>(mask);

  uint32 time = GetTimeMs();
  uint32 bit = 1;
  for (size_t i = 0; i < m_flagTimes.size(); ++i) {
    if ((bit & static_cast<uint32>(mask)) != 0) {
      m_flagTimes[i] = time;
    }
    bit = (bit << 1) | (bit >> 31);
  }
}

void CLTNetwork::ClearFlags(int mask) {
  m_flags &= ~static_cast<uint32>(mask);

  uint32 bit = 1;
  for (size_t i = 0; i < m_flagTimes.size(); ++i) {
    if ((bit & static_cast<uint32>(mask)) != 0) {
      m_flagTimes[i] = 0;
    }
    bit = (bit << 1) | (bit >> 31);
  }
}

void CLTNetwork::TickFlags() {
  uint32 now = GetTimeMs();
  uint32 bit = 1;
  for (size_t i = 0; i < m_flagTimes.size(); ++i) {
    if ((m_flags & bit) != 0 && m_flagTimes[i] != 0 && (now - m_flagTimes[i]) > kFlagTimeoutMs) {
      m_flags &= ~bit;
      m_flagTimes[i] = 0;
    }
    bit = (bit << 1) | (bit >> 31);
  }
}

int CLTNetwork::GetFrameTime() { return static_cast<int>(GetTimeMs()); }

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

uint32 CLTNetwork::GetTimeMs() {
  using clock = std::chrono::steady_clock;
  using ms = std::chrono::milliseconds;
  return static_cast<uint32>(std::chrono::duration_cast<ms>(clock::now().time_since_epoch()).count());
}

void CLTNetwork::ExtractSenderAddress(const SLNet::SystemAddress &address, uint8 senderAddr[4], uint16 &senderPort) {
  if (address.GetIPVersion() == 4) {
    uint32_t rawAddr = address.address.addr4.sin_addr.s_addr;
    std::memcpy(senderAddr, &rawAddr, sizeof(rawAddr));
    senderPort = address.GetPort();
  } else {
    senderAddr[0] = senderAddr[1] = senderAddr[2] = senderAddr[3] = 0;
    senderPort = 0;
  }
}

void CLTNetwork::ProcessIncomingPeer(SLNet::RakPeerInterface *peer, DispatchTarget source) {
  if (!peer) {
    return;
  }

  for (auto *packet = peer->Receive(); packet; packet = peer->Receive()) {
    if (!packet->data || packet->length == 0) {
      peer->DeallocatePacket(packet);
      continue;
    }
    const auto messageId = static_cast<unsigned char>(packet->data[0]);
    switch (messageId) {
    case ID_CONNECTION_REQUEST_ACCEPTED:
      if (source == DispatchTarget::kMaster) {
        m_masterConnected = true;
        m_masterAddress.m_nPort = packet->systemAddress.GetPort();
        m_masterSystem = packet->systemAddress;
      } else {
        m_worldConnected = true;
        m_worldAddress.m_nPort = packet->systemAddress.GetPort();
        m_worldSystem = packet->systemAddress;
      }
      break;
    case ID_CONNECTION_ATTEMPT_FAILED:
    case ID_CONNECTION_LOST:
    case ID_DISCONNECTION_NOTIFICATION:
      if (source == DispatchTarget::kMaster) {
        m_masterConnected = false;
        m_masterSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
      } else {
        m_worldConnected = false;
        m_worldSystem = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
      }
      break;
    default:
      break;
    }

    if (ShouldForwardToUserland(packet)) {
      CPacket_Write packetWrite;
      packetWrite.WriteDataRaw(packet->data, packet->length);
      CPacket_Read packetRead(packetWrite);
      uint8 senderAddr[4] = {0, 0, 0, 0};
      uint16 senderPort = 0;
      ExtractSenderAddress(packet->systemAddress, senderAddr, senderPort);

      if (g_clientPacketInjector) {
        g_clientPacketInjector(packetRead, senderAddr, senderPort);
      }
      if (g_serverPacketInjector) {
        g_serverPacketInjector(packetRead, senderAddr, senderPort);
      }
    }

    peer->DeallocatePacket(packet);
  }
}

// Expose the engine interface implementation.
define_interface(CLTNetwork, ILTNetwork);
implements_also(CLTNetwork, Default, ILTNetwork, Client);
implements_also(CLTNetwork, Default, ILTNetwork, Server);
