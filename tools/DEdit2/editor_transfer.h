#pragma once

#include <cstdint>

// Name used for DEdit-style drag/drop or clipboard payloads.
constexpr const char* kDEditTransferFormatName = "DEditTransfer";

extern uint32_t g_EditorTransferFormat;

void RegisterEditorTransferFormat();

// All the DirectEditor transfer types.
#define DT_TEXTUREDRAG 0

#define TRANSFERNAME_LEN 200

class CEditorTransfer
{
public:
	uint32_t m_TransferType = 0;
	char m_StrData[TRANSFERNAME_LEN] = {};
};
