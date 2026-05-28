#pragma once

#include "iltsharedgamestate.h"

#include <mutex>
#include <vector>

/// Default ILTSharedGameState implementation backed by engine-owned memory.
class CLTSharedGameState final : public ILTSharedGameState {
public:
  declare_interface(CLTSharedGameState);

  LTRESULT Init(uint32 nBytes) override;
  bool IsInitialized() const override;
  uint32 GetSize() const override;
  LTRESULT Reset() override;

  void Lock() override;
  void Unlock() override;

  LTRESULT ReadBytes(uint32 nByteOffset, void *pDest, uint32 nBytes) override;
  LTRESULT WriteBytes(uint32 nByteOffset, const void *pSrc, uint32 nBytes) override;
  LTRESULT FillBytes(uint32 nByteOffset, uint8 nValue, uint32 nBytes) override;
  void *GetPointer(uint32 nByteOffset, uint32 nBytes) override;

private:
  bool IsValidRange(uint32 nByteOffset, uint32 nBytes) const;

  std::vector<uint8> m_data;
  mutable std::recursive_mutex m_mutex;
};
