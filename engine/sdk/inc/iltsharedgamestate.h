#pragma once

#include "ltbasetypes.h"
#include "ltmodule.h"

/// Engine-owned byte-addressable state shared by game modules in the same process.
///
/// FoM uses this service to keep the historical client/server shared-state
/// contract without relying on platform IPC objects. Accessors are synchronized
/// internally, except for GetPointer(), which intentionally preserves the
/// legacy raw-pointer escape hatch for callers that already manage lifetime.
class ILTSharedGameState : public IBase {
public:
  interface_version(ILTSharedGameState, 0);

  virtual LTRESULT Init(uint32 nBytes) = 0;
  virtual bool IsInitialized() const = 0;
  virtual uint32 GetSize() const = 0;
  virtual LTRESULT Reset() = 0;

  virtual void Lock() = 0;
  virtual void Unlock() = 0;

  virtual LTRESULT ReadBytes(uint32 nByteOffset, void *pDest, uint32 nBytes) = 0;
  virtual LTRESULT WriteBytes(uint32 nByteOffset, const void *pSrc, uint32 nBytes) = 0;
  virtual LTRESULT FillBytes(uint32 nByteOffset, uint8 nValue, uint32 nBytes) = 0;
  virtual void *GetPointer(uint32 nByteOffset, uint32 nBytes) = 0;
};
