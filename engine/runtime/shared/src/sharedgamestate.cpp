#include "bdefs.h"

#include "sharedgamestate.h"

#include "ltcodes.h"
#include "ltmodule.h"

#include <algorithm>
#include <cstring>

namespace {
bool HasOverflow(uint32 nByteOffset, uint32 nBytes) { return nByteOffset + nBytes < nByteOffset; }
} // namespace

LTRESULT CLTSharedGameState::Init(uint32 nBytes) {
  if (nBytes == 0) {
    return LT_INVALIDPARAMS;
  }

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_data.empty()) {
    m_data.assign(nBytes, 0);
    return LT_OK;
  }

  if (m_data.size() != nBytes) {
    return LT_ERROR;
  }

  return LT_OK;
}

bool CLTSharedGameState::IsInitialized() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return !m_data.empty();
}

uint32 CLTSharedGameState::GetSize() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return static_cast<uint32>(m_data.size());
}

LTRESULT CLTSharedGameState::Reset() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_data.empty()) {
    return LT_ERROR;
  }

  std::fill(m_data.begin(), m_data.end(), 0);
  return LT_OK;
}

void CLTSharedGameState::Lock() { m_mutex.lock(); }

void CLTSharedGameState::Unlock() { m_mutex.unlock(); }

LTRESULT CLTSharedGameState::ReadBytes(uint32 nByteOffset, void *pDest, uint32 nBytes) {
  if ((pDest == nullptr) || (nBytes == 0)) {
    return LT_INVALIDPARAMS;
  }

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!IsValidRange(nByteOffset, nBytes)) {
    std::memset(pDest, 0, nBytes);
    return LT_ERROR;
  }

  std::memcpy(pDest, m_data.data() + nByteOffset, nBytes);
  return LT_OK;
}

LTRESULT CLTSharedGameState::WriteBytes(uint32 nByteOffset, const void *pSrc, uint32 nBytes) {
  if ((pSrc == nullptr) || (nBytes == 0)) {
    return LT_INVALIDPARAMS;
  }

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!IsValidRange(nByteOffset, nBytes)) {
    return LT_ERROR;
  }

  std::memcpy(m_data.data() + nByteOffset, pSrc, nBytes);
  return LT_OK;
}

LTRESULT CLTSharedGameState::FillBytes(uint32 nByteOffset, uint8 nValue, uint32 nBytes) {
  if (nBytes == 0) {
    return LT_INVALIDPARAMS;
  }

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!IsValidRange(nByteOffset, nBytes)) {
    return LT_ERROR;
  }

  std::memset(m_data.data() + nByteOffset, nValue, nBytes);
  return LT_OK;
}

void *CLTSharedGameState::GetPointer(uint32 nByteOffset, uint32 nBytes) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!IsValidRange(nByteOffset, nBytes)) {
    return nullptr;
  }

  return m_data.data() + nByteOffset;
}

bool CLTSharedGameState::IsValidRange(uint32 nByteOffset, uint32 nBytes) const {
  return !m_data.empty() && !HasOverflow(nByteOffset, nBytes) && (nByteOffset + nBytes <= m_data.size());
}

define_interface(CLTSharedGameState, ILTSharedGameState);
