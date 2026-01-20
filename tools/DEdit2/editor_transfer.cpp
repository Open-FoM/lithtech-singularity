#include "editor_transfer.h"

#include <cstdint>

uint32_t g_EditorTransferFormat = 0;

namespace
{
uint32_t HashString32(const char* text)
{
	const uint32_t prime = 16777619u;
	uint32_t hash = 2166136261u;
	if (!text)
	{
		return hash;
	}
	for (const char* p = text; *p; ++p)
	{
		hash ^= static_cast<uint8_t>(*p);
		hash *= prime;
	}
	return hash;
}
} // namespace

void RegisterEditorTransferFormat()
{
	if (g_EditorTransferFormat != 0)
	{
		return;
	}
	g_EditorTransferFormat = HashString32(kDEditTransferFormatName);
}
