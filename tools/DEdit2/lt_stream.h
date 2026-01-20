#pragma once

#include <string>
#include <vector>

#include "ltcodes.h"
#include "genltstream.h"

bool InitClientFileMgr();
void TermClientFileMgr();
void SetClientFileMgrTrees(const std::vector<std::string>& trees);
const std::vector<std::string>& GetClientFileMgrTrees();
ILTStream* OpenFileStream(const std::string& path);
bool ReadFileToBuffer(const std::string& path, std::vector<uint8_t>& out_data, std::string& error);
