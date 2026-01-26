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

#include "ltmodule.h"

namespace {
constexpr int kNetNotInitialized = 73;
constexpr uint32 kFlagTimeoutMs = 0x1388;
} // namespace

CLTNetwork::CLTNetwork() = default;

const char *CLTNetwork::_InterfaceImplementation() { return "CLTNetwork"; }

bool CLTNetwork::IsMasterConnected() { return m_masterConnected; }

bool CLTNetwork::IsWorldConnected() { return m_worldConnected; }

/// Decomp refs:
/// - Docs/Notes/ClientNetworking.md (init sequence notes)
LTRESULT CLTNetwork::InitNetwork(uint16 localPort) {
  (void)localPort;

  if (!m_initialized) {
    m_initialized = true;
  }

  // Pseudocode (RakNet):
  // 1) Create two RakPeer instances (master/world).
  // 2) Load public key (fom_public.key) and configure security if required.
  // 3) Bind local socket on localPort (or 0 for ephemeral).
  // 4) Set MTU (FoM uses 0x578).
  // 5) Register packet handler callbacks / plugins.
  return LT_OK;
}

LTRESULT CLTNetwork::ShutdownNetwork() {
  if (!m_initialized) {
    return LT_ERROR;
  }

  m_initialized = false;
  m_masterConnected = false;
  m_worldConnected = false;
  m_masterAddress = {};
  m_worldAddress = {};
  m_lastMasterAddress = {};

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

  // Pseudocode (RakNet):
  // - worldPeer->Connect(address.ip, address.port, "37eG87Ph", 8, 0, 7, 500, 0, 0).
  // - Set m_worldConnected based on connect result / NEW_INCOMING_CONNECTION.
  m_worldAddress = address;
  m_worldConnected = true;
  return LT_OK;
}

LTRESULT CLTNetwork::Disconnect() {
  if (!m_initialized) {
    return LT_ERROR;
  }

  // Pseudocode (RakNet):
  // - masterPeer->Shutdown(timeoutMs, notify, orderingChannel)
  // - worldPeer->Shutdown(timeoutMs, notify, orderingChannel)
  // - Clear cached addresses.
  m_masterConnected = false;
  m_worldConnected = false;
  m_masterAddress = {};
  m_worldAddress = {};
  m_lastMasterAddress = {};

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
  (void)priority;
  (void)reliability;
  (void)orderingChannel;
  (void)destination;

  if (!m_initialized) {
    return LT_ERROR;
  }

  if (packet == nullptr) {
    return LT_ERROR;
  }

  // Pseudocode (RakNet):
  // - Validate packet has data (bit count > 0).
  // - Choose peer based on destination (1=master, 2=world).
  // - If peer not connected, return 0.
  // - Convert VariableSizedPacket to raw buffer + bit count.
  // - peer->Send(bitstream, priority, reliability, orderingChannel, ip, port, broadcast=false).
  //   (IDA: sub_74AF40 -> vtbl+0x44 -> sub_79B670 -> sub_7A1DF0)
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

  // Pseudocode (RakNet):
  // - Poll masterPeer->Receive and worldPeer->Receive.
  // - Dispatch incoming packets by ID.
  // - Update connection flags (NEW_INCOMING_CONNECTION / DISCONNECTION).
  TickFlags();
}

/// Decomp refs:
/// - IDA Object.lto: 0x767F20 (write bitstream helper)
int CLTNetwork::WriteBitStream(void *src, int maxBits, void *stream) {
  (void)src;
  (void)maxBits;
  (void)stream;

  // Pseudocode (RakNet):
  // - Write length bits.
  // - Write payload bytes.
  // - Apply FoM-specific string compression rules.
  return 0;
}

/// Decomp refs:
/// - IDA Object.lto: 0x767F50 (read bitstream helper)
int CLTNetwork::ReadBitStream(void *dst, int maxBits, void *stream) {
  (void)dst;
  (void)maxBits;
  (void)stream;

  // Pseudocode (RakNet):
  // - Read length bits.
  // - Clamp to maxBits.
  // - Read payload bytes.
  // - Apply FoM-specific decompression rules.
  return 0;
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

// Expose the engine interface implementation.
define_interface(CLTNetwork, ILTNetwork);
implements_also(CLTNetwork, Default, ILTNetwork, Client);
implements_also(CLTNetwork, Default, ILTNetwork, Server);
