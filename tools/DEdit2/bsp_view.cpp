#include "bsp_view.h"

#include "bdefs.h"
#include "ltproperty.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

struct BinaryReader
{
	const uint8_t* data = nullptr;
	size_t size = 0;
	size_t pos = 0;
	bool error = false;

	explicit BinaryReader(const std::vector<uint8_t>& buffer)
		: data(buffer.data())
		, size(buffer.size())
	{
	}

	template <typename T>
	bool ReadPod(T& out)
	{
		if (pos + sizeof(T) > size)
		{
			error = true;
			out = {};
			return false;
		}
		std::memcpy(&out, data + pos, sizeof(T));
		pos += sizeof(T);
		return true;
	}

	bool ReadBytes(void* out, size_t len)
	{
		if (pos + len > size)
		{
			error = true;
			if (out)
			{
				std::memset(out, 0, len);
			}
			return false;
		}
		if (out && len > 0)
		{
			std::memcpy(out, data + pos, len);
		}
		pos += len;
		return true;
	}

	bool ReadString(std::string& out)
	{
		uint16_t len = 0;
		if (!ReadPod(len))
		{
			return false;
		}
		if (len == 0)
		{
			out.clear();
			return true;
		}
		if (pos + len > size)
		{
			error = true;
			out.clear();
			return false;
		}
		out.assign(reinterpret_cast<const char*>(data + pos), len);
		pos += len;
		return true;
	}

	bool Skip(size_t len)
	{
		if (pos + len > size)
		{
			error = true;
			return false;
		}
		pos += len;
		return true;
	}

	bool Seek(size_t offset)
	{
		if (offset > size)
		{
			error = true;
			return false;
		}
		pos = offset;
		return true;
	}

	size_t Tell() const
	{
		return pos;
	}
};

bool LoadBinaryFile(const std::string& path, std::vector<uint8_t>& out_data, std::string& error)
{
	out_data.clear();
	error.clear();

	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file)
	{
		error = "Failed to open world file.";
		return false;
	}

	const std::streamsize size = file.tellg();
	if (size <= 0)
	{
		error = "World file is empty.";
		return false;
	}

	file.seekg(0, std::ios::beg);
	out_data.resize(static_cast<size_t>(size));
	if (!file.read(reinterpret_cast<char*>(out_data.data()), size))
	{
		error = "Failed to read world file.";
		out_data.clear();
		return false;
	}

	return true;
}

bool ReadPropString(BinaryReader& reader, uint16_t prop_len, std::string& out)
{
	const size_t start = reader.Tell();
	if (!reader.ReadString(out))
	{
		return false;
	}
	const size_t consumed = reader.Tell() - start;
	if (prop_len > consumed)
	{
		reader.Skip(prop_len - consumed);
	}
	return !reader.error;
}

bool ReadPropVector(BinaryReader& reader, uint16_t prop_len, float out[3])
{
	if (prop_len < sizeof(float) * 3)
	{
		reader.Skip(prop_len);
		return false;
	}
	for (int i = 0; i < 3; ++i)
	{
		if (!reader.ReadPod(out[i]))
		{
			return false;
		}
	}
	const size_t remaining = prop_len - (sizeof(float) * 3);
	if (remaining > 0)
	{
		reader.Skip(remaining);
	}
	return !reader.error;
}
} // namespace

bool GetBspWorldView(const std::string& world_path, BspWorldView& out_view, std::string& out_error)
{
	out_view = {};
	out_error.clear();

	const fs::path path(world_path);
	if (!fs::exists(path))
	{
		out_error = "World file does not exist.";
		return false;
	}

	std::vector<uint8_t> buffer;
	if (!LoadBinaryFile(world_path, buffer, out_error))
	{
		return false;
	}

	BinaryReader reader(buffer);
	uint32_t version = 0;
	if (!reader.ReadPod(version))
	{
		out_error = "World file header is incomplete.";
		return false;
	}
	if (version != 85)
	{
		out_error = "Unsupported world version.";
		return false;
	}

	uint32_t object_data_pos = 0;
	uint32_t blind_object_data_pos = 0;
	uint32_t lightgrid_pos = 0;
	uint32_t collision_data_pos = 0;
	uint32_t particle_blocker_data_pos = 0;
	uint32_t render_data_pos = 0;

	reader.ReadPod(object_data_pos);
	reader.ReadPod(blind_object_data_pos);
	reader.ReadPod(lightgrid_pos);
	reader.ReadPod(collision_data_pos);
	reader.ReadPod(particle_blocker_data_pos);
	reader.ReadPod(render_data_pos);

	uint32_t dummy = 0;
	for (int i = 0; i < 8; ++i)
	{
		reader.ReadPod(dummy);
	}

	if (reader.error)
	{
		out_error = "World file header is invalid.";
		return false;
	}

	if (object_data_pos >= buffer.size())
	{
		out_error = "World file missing object data.";
		return false;
	}
	if (!reader.Seek(object_data_pos))
	{
		out_error = "Failed to read object data.";
		return false;
	}

	uint32_t num_objects = 0;
	if (!reader.ReadPod(num_objects))
	{
		out_error = "World file object data is invalid.";
		return false;
	}

	std::string world_name = path.stem().string();
	for (uint32_t obj_index = 0; obj_index < num_objects; ++obj_index)
	{
		uint16_t object_len = 0;
		if (!reader.ReadPod(object_len))
		{
			out_error = "World file object data is incomplete.";
			return false;
		}
		const size_t object_start = reader.Tell();

		std::string object_type;
		if (!reader.ReadString(object_type))
		{
			out_error = "World file object type is invalid.";
			return false;
		}

		uint32_t prop_count = 0;
		if (!reader.ReadPod(prop_count))
		{
			out_error = "World file object data is invalid.";
			return false;
		}

		const bool is_world_props = (object_type == "WorldProperties");
		BspObjectView obj{};
		obj.id = obj_index;
		obj.class_name = object_type;

		for (uint32_t prop_index = 0; prop_index < prop_count; ++prop_index)
		{
			std::string prop_name;
			if (!reader.ReadString(prop_name))
			{
				out_error = "World file property data is invalid.";
				return false;
			}

			uint8_t prop_code = 0;
			uint32_t prop_flags = 0;
			uint16_t prop_len = 0;
			if (!reader.ReadPod(prop_code) ||
				!reader.ReadPod(prop_flags) ||
				!reader.ReadPod(prop_len))
			{
				out_error = "World file property data is invalid.";
				return false;
			}

			const std::string key = prop_name;
			if (prop_code == PT_STRING)
			{
				std::string value;
				if (!ReadPropString(reader, prop_len, value))
				{
					out_error = "World file string property is invalid.";
					return false;
				}
				if (key == "Name" || key == "name")
				{
					if (is_world_props && !value.empty())
					{
						world_name = value;
					}
					else if (!value.empty())
					{
						obj.name = value;
					}
				}
			}
			else if (prop_code == PT_VECTOR || prop_code == PT_COLOR)
			{
				float vec[3] = {0.0f, 0.0f, 0.0f};
				if (!ReadPropVector(reader, prop_len, vec))
				{
					out_error = "World file vector property is invalid.";
					return false;
				}
				if (key == "Pos" || key == "pos" || key == "Position")
				{
					obj.position[0] = vec[0];
					obj.position[1] = vec[1];
					obj.position[2] = vec[2];
				}
			}
			else if (prop_code == PT_ROTATION)
			{
				float vec[3] = {0.0f, 0.0f, 0.0f};
				if (!ReadPropVector(reader, prop_len, vec))
				{
					out_error = "World file rotation property is invalid.";
					return false;
				}
				obj.rotation[0] = vec[0];
				obj.rotation[1] = vec[1];
				obj.rotation[2] = vec[2];
			}
			else
			{
				reader.Skip(prop_len);
			}

			(void)prop_flags;
			if (reader.error)
			{
				out_error = "World file property data is invalid.";
				return false;
			}
		}

		if (!is_world_props)
		{
			out_view.objects.push_back(std::move(obj));
		}

		const size_t object_end = object_start + object_len;
		if (reader.Tell() < object_end)
		{
			reader.Seek(object_end);
		}
		if (reader.error)
		{
			out_error = "World file object data is invalid.";
			return false;
		}
	}

	const auto is_default_world_name = [](const std::string& name)
	{
		return name.empty() || name.rfind("WorldProperties", 0) == 0;
	};
	if (is_default_world_name(world_name))
	{
		world_name = path.stem().string();
	}
	out_view.world_name = world_name.empty() ? "World" : world_name;
	return true;
}
