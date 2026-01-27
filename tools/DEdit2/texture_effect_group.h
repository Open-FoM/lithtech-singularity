#pragma once

#include <array>
#include <cstdint>
#include <string>

struct TextureEffectGroupStage
{
	bool enabled = false;
	bool overridden = false;
	uint32_t channel = 0;
	uint32_t override_stage = 0;
	std::string script;
	std::array<float, 6> defaults{};
};

struct TextureEffectGroup
{
	std::array<TextureEffectGroupStage, 2> stages{};
};

bool LoadTextureEffectGroup(const std::string& path, TextureEffectGroup& out, std::string& error);
bool SaveTextureEffectGroup(const std::string& path, const TextureEffectGroup& group, std::string& error);
