#include "texture_effect_group.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace
{
constexpr uint32_t kEffectGroupVersion = 1;
constexpr uint32_t kNumStages = 2;
constexpr uint32_t kNumVars = 6;

constexpr uint32_t kStageDisabled = 0;
constexpr uint32_t kStageOverridden = 1;
constexpr uint32_t kStageEvaluated = 2;

bool ReadExact(std::ifstream& file, void* data, size_t size)
{
	file.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size));
	return file.good();
}

bool WriteExact(std::ofstream& file, const void* data, size_t size)
{
	file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
	return file.good();
}
} // namespace

bool LoadTextureEffectGroup(const std::string& path, TextureEffectGroup& out, std::string& error)
{
	error.clear();
	out = TextureEffectGroup{};

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		error = "Failed to open effect group file.";
		return false;
	}

	uint32_t version = 0;
	if (!ReadExact(file, &version, sizeof(version)) || version != kEffectGroupVersion)
	{
		error = "Effect group version mismatch.";
		return false;
	}

	uint32_t stage_count = 0;
	if (!ReadExact(file, &stage_count, sizeof(stage_count)))
	{
		error = "Failed to read effect group stage count.";
		return false;
	}
	stage_count = std::min(stage_count, kNumStages);

	for (uint32_t stage_index = 0; stage_index < stage_count; ++stage_index)
	{
		uint32_t stage_type = 0;
		if (!ReadExact(file, &stage_type, sizeof(stage_type)))
		{
			error = "Failed to read stage type.";
			return false;
		}

		if (stage_type == kStageDisabled)
		{
			continue;
		}

		TextureEffectGroupStage& stage = out.stages[stage_index];
		stage.enabled = true;

		uint32_t channel = 0;
		if (!ReadExact(file, &channel, sizeof(channel)))
		{
			error = "Failed to read stage channel.";
			return false;
		}
		stage.channel = channel;

		if (stage_type == kStageOverridden)
		{
			uint32_t override_stage = 0;
			if (!ReadExact(file, &override_stage, sizeof(override_stage)))
			{
				error = "Failed to read override stage.";
				return false;
			}
			stage.overridden = true;
			stage.override_stage = override_stage;
			continue;
		}

		if (stage_type != kStageEvaluated)
		{
			continue;
		}

		uint16_t script_len = 0;
		if (!ReadExact(file, &script_len, sizeof(script_len)))
		{
			error = "Failed to read script name length.";
			return false;
		}
		if (script_len > 0)
		{
			stage.script.resize(script_len);
			if (!ReadExact(file, stage.script.data(), script_len))
			{
				error = "Failed to read script name.";
				return false;
			}
		}

		uint32_t default_count = 0;
		if (!ReadExact(file, &default_count, sizeof(default_count)))
		{
			error = "Failed to read default count.";
			return false;
		}
		for (uint32_t i = 0; i < default_count; ++i)
		{
			float value = 0.0f;
			if (!ReadExact(file, &value, sizeof(value)))
			{
				error = "Failed to read default value.";
				return false;
			}
			if (i < kNumVars)
			{
				stage.defaults[i] = value;
			}
		}
	}

	return true;
}

bool SaveTextureEffectGroup(const std::string& path, const TextureEffectGroup& group, std::string& error)
{
	error.clear();
	std::error_code ec;
	const std::filesystem::path out_path(path);
	if (!out_path.parent_path().empty())
	{
		std::filesystem::create_directories(out_path.parent_path(), ec);
	}

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		error = "Failed to open effect group file for writing.";
		return false;
	}

	const uint32_t version = kEffectGroupVersion;
	if (!WriteExact(file, &version, sizeof(version)))
	{
		error = "Failed to write effect group version.";
		return false;
	}

	const uint32_t stage_count = kNumStages;
	if (!WriteExact(file, &stage_count, sizeof(stage_count)))
	{
		error = "Failed to write stage count.";
		return false;
	}

	for (uint32_t stage_index = 0; stage_index < kNumStages; ++stage_index)
	{
		const TextureEffectGroupStage& stage = group.stages[stage_index];
		if (!stage.enabled)
		{
			const uint32_t stage_type = kStageDisabled;
			if (!WriteExact(file, &stage_type, sizeof(stage_type)))
			{
				error = "Failed to write disabled stage.";
				return false;
			}
			continue;
		}

		const uint32_t stage_type = stage.overridden ? kStageOverridden : kStageEvaluated;
		if (!WriteExact(file, &stage_type, sizeof(stage_type)))
		{
			error = "Failed to write stage type.";
			return false;
		}

		if (!WriteExact(file, &stage.channel, sizeof(stage.channel)))
		{
			error = "Failed to write stage channel.";
			return false;
		}

		if (stage.overridden)
		{
			if (!WriteExact(file, &stage.override_stage, sizeof(stage.override_stage)))
			{
				error = "Failed to write override stage.";
				return false;
			}
			continue;
		}

		const uint16_t script_len = static_cast<uint16_t>(stage.script.size());
		if (!WriteExact(file, &script_len, sizeof(script_len)))
		{
			error = "Failed to write script length.";
			return false;
		}
		if (script_len > 0 && !WriteExact(file, stage.script.data(), script_len))
		{
			error = "Failed to write script.";
			return false;
		}

		const uint32_t defaults = kNumVars;
		if (!WriteExact(file, &defaults, sizeof(defaults)))
		{
			error = "Failed to write default count.";
			return false;
		}
		for (uint32_t i = 0; i < kNumVars; ++i)
		{
			if (!WriteExact(file, &stage.defaults[i], sizeof(stage.defaults[i])))
			{
				error = "Failed to write defaults.";
				return false;
			}
		}
	}

	return true;
}
