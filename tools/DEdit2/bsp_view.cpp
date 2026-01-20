#include "bsp_view.h"

#include "bdefs.h"
#include "ltproperty.h"
#include "lt_stream.h"

#include "de_world.h"
#include "iltstream.h"
#include "loadstatus.h"
#include "world_shared_bsp.h"
#include "world_tree.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstddef>
#include <memory>
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

struct SurfaceAccum
{
	uint32_t poly_count = 0;
	LTVector bounds_min;
	LTVector bounds_max;
	LTVector centroid_sum;
	bool has_bounds = false;
};

void UpdateBounds(const LTVector& point, LTVector& min_v, LTVector& max_v, bool& has_bounds)
{
	if (!has_bounds)
	{
		min_v = point;
		max_v = point;
		has_bounds = true;
		return;
	}

	min_v.x = std::min(min_v.x, point.x);
	min_v.y = std::min(min_v.y, point.y);
	min_v.z = std::min(min_v.z, point.z);
	max_v.x = std::max(max_v.x, point.x);
	max_v.y = std::max(max_v.y, point.y);
	max_v.z = std::max(max_v.z, point.z);
}

bool LoadWorldGeometry(const std::string& world_path, BspWorldView& out_view, std::string& out_error)
{
	out_error.clear();

	struct StreamDeleter
	{
		void operator()(ILTStream* stream) const
		{
			if (stream)
			{
				stream->Release();
			}
		}
	};

	std::unique_ptr<ILTStream, StreamDeleter> stream(OpenFileStream(world_path));
	if (!stream)
	{
		out_error = "Failed to open world file.";
		return false;
	}
	ILTStream* pStream = stream.get();

	uint32 version = 0;
	uint32 object_data_pos = 0;
	uint32 blind_object_data_pos = 0;
	uint32 lightgrid_pos = 0;
	uint32 collision_data_pos = 0;
	uint32 particle_blocker_data_pos = 0;
	uint32 render_data_pos = 0;
	if (!IWorldSharedBSP::ReadWorldHeader(stream.get(),
		version,
		object_data_pos,
		blind_object_data_pos,
		lightgrid_pos,
		collision_data_pos,
		particle_blocker_data_pos,
		render_data_pos))
	{
		out_error = "Unsupported world version.";
		return false;
	}
	(void)object_data_pos;
	(void)blind_object_data_pos;
	(void)lightgrid_pos;
	(void)collision_data_pos;
	(void)particle_blocker_data_pos;
	(void)render_data_pos;

	out_view.version = version;

	uint32 info_len = 0;
	stream->Read(&info_len, sizeof(info_len));
	if (stream->ErrorStatus() != LT_OK)
	{
		out_error = "World file header is invalid.";
		return false;
	}

	if (info_len > 0)
	{
		std::string info;
		info.resize(info_len);
		stream->Read(info.data(), info_len);
		if (stream->ErrorStatus() != LT_OK)
		{
			out_error = "World file header is invalid.";
			return false;
		}
	}

	LTVector world_min{};
	LTVector world_max{};
	LTVector world_offset{};
	*stream >> world_min >> world_max >> world_offset;
	if (stream->ErrorStatus() != LT_OK)
	{
		out_error = "World file header is invalid.";
		return false;
	}
	(void)world_offset;

	out_view.world_bounds.min[0] = world_min.x;
	out_view.world_bounds.min[1] = world_min.y;
	out_view.world_bounds.min[2] = world_min.z;
	out_view.world_bounds.max[0] = world_max.x;
	out_view.world_bounds.max[1] = world_max.y;
	out_view.world_bounds.max[2] = world_max.z;

	WorldTree world_tree;
	if (!world_tree.LoadLayout(stream.get()))
	{
		out_error = "World file world tree is invalid.";
		return false;
	}

	uint32 num_world_models = 0;
	STREAM_READ(num_world_models);
	if (stream->ErrorStatus() != LT_OK)
	{
		out_error = "World file data is invalid.";
		return false;
	}

	out_view.worldmodels.clear();
	out_view.worldmodels.reserve(num_world_models);

	for (uint32 model_index = 0; model_index < num_world_models; ++model_index)
	{
		uint32 dummy = 0;
		STREAM_READ(dummy);

		auto bsp = std::make_unique<WorldBsp>();
		const ELoadWorldStatus load_status = bsp->Load(stream.get(), true);
		if (load_status != LoadWorld_Ok)
		{
			out_error = "Failed to load world model geometry.";
			return false;
		}

		BspWorldModelView model_view{};
		model_view.id = model_index;
		model_view.name = bsp->m_WorldName;
		if (model_view.name.empty())
		{
			model_view.name = "WorldModel " + std::to_string(model_index);
		}

		model_view.bounds.min[0] = bsp->m_MinBox.x;
		model_view.bounds.min[1] = bsp->m_MinBox.y;
		model_view.bounds.min[2] = bsp->m_MinBox.z;
		model_view.bounds.max[0] = bsp->m_MaxBox.x;
		model_view.bounds.max[1] = bsp->m_MaxBox.y;
		model_view.bounds.max[2] = bsp->m_MaxBox.z;

		std::vector<SurfaceAccum> surface_accum;
		surface_accum.resize(bsp->m_nSurfaces);

		for (uint32 poly_index = 0; poly_index < bsp->m_nPolies; ++poly_index)
		{
			WorldPoly* poly = bsp->m_Polies[poly_index];
			if (!poly)
			{
				continue;
			}

			Surface* surface = poly->GetSurface();
			if (!surface)
			{
				continue;
			}

			const ptrdiff_t surface_index = surface - bsp->m_Surfaces;
			if (surface_index < 0 || surface_index >= static_cast<ptrdiff_t>(surface_accum.size()))
			{
				continue;
			}

			SurfaceAccum& accum = surface_accum[static_cast<size_t>(surface_index)];
			accum.poly_count++;
			accum.centroid_sum += poly->GetCenter();

			if (poly->GetNumVertices() > 0)
			{
				LTVector poly_min = poly->GetVertex(0);
				LTVector poly_max = poly_min;
				for (uint32 v = 1; v < poly->GetNumVertices(); ++v)
				{
					const LTVector& vert = poly->GetVertex(v);
					poly_min.x = std::min(poly_min.x, vert.x);
					poly_min.y = std::min(poly_min.y, vert.y);
					poly_min.z = std::min(poly_min.z, vert.z);
					poly_max.x = std::max(poly_max.x, vert.x);
					poly_max.y = std::max(poly_max.y, vert.y);
					poly_max.z = std::max(poly_max.z, vert.z);
				}
				UpdateBounds(poly_min, accum.bounds_min, accum.bounds_max, accum.has_bounds);
				UpdateBounds(poly_max, accum.bounds_min, accum.bounds_max, accum.has_bounds);
			}
		}

		model_view.surfaces.reserve(bsp->m_nSurfaces);
		for (uint32 surface_index = 0; surface_index < bsp->m_nSurfaces; ++surface_index)
		{
			const Surface& surface = bsp->m_Surfaces[surface_index];
			const SurfaceAccum& accum = surface_accum[surface_index];

			BspSurfaceView surface_view{};
			surface_view.id = surface_index;
			surface_view.material_id = surface.m_iTexture;
			surface_view.surface_flags = surface.m_Flags;
			surface_view.poly_count = accum.poly_count;
			if (bsp->m_TextureNames)
			{
				surface_view.material = bsp->m_TextureNames[surface.m_iTexture];
			}
			if (accum.poly_count > 0)
			{
				const float inv = 1.0f / static_cast<float>(accum.poly_count);
				surface_view.centroid[0] = accum.centroid_sum.x * inv;
				surface_view.centroid[1] = accum.centroid_sum.y * inv;
				surface_view.centroid[2] = accum.centroid_sum.z * inv;
			}
			if (accum.has_bounds)
			{
				surface_view.bounds.min[0] = accum.bounds_min.x;
				surface_view.bounds.min[1] = accum.bounds_min.y;
				surface_view.bounds.min[2] = accum.bounds_min.z;
				surface_view.bounds.max[0] = accum.bounds_max.x;
				surface_view.bounds.max[1] = accum.bounds_max.y;
				surface_view.bounds.max[2] = accum.bounds_max.z;
			}

			model_view.surfaces.push_back(std::move(surface_view));
		}

		out_view.worldmodels.push_back(std::move(model_view));
	}

	if (stream->ErrorStatus() != LT_OK)
	{
		out_error = "World file data is invalid.";
		return false;
	}

	return true;
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

	if (!LoadWorldGeometry(world_path, out_view, out_error))
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
