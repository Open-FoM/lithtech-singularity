#pragma once

#include <string>

#include "ltcodes.h"
#include "ltlink.h"
#include "dtxmgr.h"

TextureData* LoadDtxTexture(const std::string& path, std::string* error);
TextureData* CreateSolidColorTexture(uint32 rgba);
