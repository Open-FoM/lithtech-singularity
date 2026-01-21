#include "editor_state.h"

#include "bdefs.h"
#include "ltanode.h"
#include "ltanodereader.h"
#include "ltareader.h"
#include "ltaloadonlyalloc.h"
#include "ltautil.h"
#include "ltproperty.h"
#include "lt_stream.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
namespace fs = std::filesystem;

constexpr size_t kMaxProjectNodes = 25000;
constexpr uint32_t kWorldFileVersion = 85;

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

	bool ReadBytes(void* out, size_t len)
	{
		if (pos + len > size)
		{
			error = true;
			if (out != nullptr)
			{
				std::memset(out, 0, len);
			}
			return false;
		}
		if (out != nullptr && len > 0)
		{
			std::memcpy(out, data + pos, len);
		}
		pos += len;
		return true;
	}

	template <typename T>
	bool ReadPod(T& out)
	{
		return ReadBytes(&out, sizeof(T));
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

std::string ToLower(std::string value)
{
	for (char& ch : value)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return value;
}

std::string TrimString(const std::string& value)
{
	size_t start = 0;
	while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
	{
		++start;
	}
	size_t end = value.size();
	while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
	{
		--end;
	}
	return value.substr(start, end - start);
}

bool StartsWith(const std::string& value, const char* prefix)
{
	const size_t prefix_len = std::strlen(prefix);
	return value.size() >= prefix_len && value.compare(0, prefix_len, prefix) == 0;
}

bool TryParseFloat(const char* text, float& out)
{
	if (text == nullptr)
	{
		return false;
	}
	char* end = nullptr;
	out = std::strtof(text, &end);
	return end != text;
}

bool TryParseBool(const char* text, bool& out)
{
	if (text == nullptr)
	{
		return false;
	}
	const std::string value = ToLower(TrimString(text));
	if (value == "true" || value == "1" || value == "yes")
	{
		out = true;
		return true;
	}
	if (value == "false" || value == "0" || value == "no")
	{
		out = false;
		return true;
	}
	return false;
}

bool LoadBinaryFile(const std::string& path, std::vector<uint8_t>& out_data, std::string& error)
{
	return ReadFileToBuffer(path, out_data, error);
}

std::string MapBinaryObjectType(const std::string& raw_type)
{
	if (raw_type == "WorldProperties" || raw_type == "World")
	{
		return "World";
	}
	if (raw_type == "DirLight" || raw_type == "ObjectLight")
	{
		return "Light";
	}
	return raw_type.empty() ? "Entity" : raw_type;
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

bool ReadPropFloat(BinaryReader& reader, uint16_t prop_len, float& out)
{
	if (prop_len < sizeof(float))
	{
		reader.Skip(prop_len);
		return false;
	}
	if (!reader.ReadPod(out))
	{
		return false;
	}
	const size_t remaining = prop_len - sizeof(float);
	if (remaining > 0)
	{
		reader.Skip(remaining);
	}
	return !reader.error;
}

bool ReadPropBool(BinaryReader& reader, uint16_t prop_len, bool& out)
{
	if (prop_len < sizeof(uint8_t))
	{
		reader.Skip(prop_len);
		return false;
	}
	uint8_t value = 0;
	if (!reader.ReadPod(value))
	{
		return false;
	}
	out = value != 0;
	const size_t remaining = prop_len - sizeof(uint8_t);
	if (remaining > 0)
	{
		reader.Skip(remaining);
	}
	return !reader.error;
}

void ApplyBinaryProperty(
	NodeProperties& props,
	TreeNode& node,
	const std::string& name,
	const std::string* str_value,
	const float* vec_value,
	const float* float_value,
	const bool* bool_value)
{
	if (name.empty())
	{
		return;
	}
	const std::string key = ToLower(name);
	const bool has_string = (str_value != nullptr);
	const bool has_vec = (vec_value != nullptr);
	const bool has_float = (float_value != nullptr);
	const bool has_bool = (bool_value != nullptr);

	if (key == "classname" || key == "class")
	{
		if (has_string && !str_value->empty())
		{
			props.class_name = *str_value;
		}
		return;
	}

	if ((key == "name" || key == "objectname") && has_string && !str_value->empty())
	{
		node.name = *str_value;
		return;
	}

	if (key == "pos" || key == "position" || key == "m_pos")
	{
		if (has_vec)
		{
			props.position[0] = vec_value[0];
			props.position[1] = vec_value[1];
			props.position[2] = vec_value[2];
		}
		return;
	}

	if (key == "rot" || key == "rotation" || key == "m_rot")
	{
		if (has_vec)
		{
			props.rotation[0] = vec_value[0];
			props.rotation[1] = vec_value[1];
			props.rotation[2] = vec_value[2];
		}
		return;
	}

	if (key == "scale" || key == "m_scale")
	{
		if (has_vec)
		{
			props.scale[0] = vec_value[0];
			props.scale[1] = vec_value[1];
			props.scale[2] = vec_value[2];
		}
		return;
	}

	if (key == "dims" || key == "size")
	{
		if (has_vec)
		{
			props.size[0] = vec_value[0];
			props.size[1] = vec_value[1];
			props.size[2] = vec_value[2];
		}
		else if (has_float)
		{
			props.size[0] = *float_value;
			props.size[1] = *float_value;
			props.size[2] = *float_value;
		}
		return;
	}

	if (key == "color" || key == "lightcolor" || key == "innercolor" || key == "outercolor")
	{
		if (has_vec)
		{
			float vec3[3] = {vec_value[0], vec_value[1], vec_value[2]};
			if (vec3[0] > 1.0f || vec3[1] > 1.0f || vec3[2] > 1.0f)
			{
				vec3[0] /= 255.0f;
				vec3[1] /= 255.0f;
				vec3[2] /= 255.0f;
			}
			props.color[0] = vec3[0];
			props.color[1] = vec3[1];
			props.color[2] = vec3[2];
		}
		return;
	}

	if (key == "ambient")
	{
		if (has_vec)
		{
			props.ambient[0] = vec_value[0];
			props.ambient[1] = vec_value[1];
			props.ambient[2] = vec_value[2];
		}
		return;
	}

	if (key == "visible" || key == "hidden" || key == "locked" || key == "frozen")
	{
		bool resolved = false;
		bool value = false;
		if (has_bool)
		{
			value = *bool_value;
			resolved = true;
		}
		else if (has_float)
		{
			value = (*float_value != 0.0f);
			resolved = true;
		}
		else if (has_string)
		{
			resolved = TryParseBool(str_value->c_str(), value);
		}
		if (resolved)
		{
			if (key == "visible")
			{
				props.visible = value;
			}
			else if (key == "hidden")
			{
				props.visible = !value;
			}
			else
			{
				props.locked = value;
			}
		}
		return;
	}

	if (key == "intensity" || key == "brightness" || key == "brightscale" || key == "objectbrightscale")
	{
		if (has_float)
		{
			props.intensity = *float_value;
		}
		return;
	}

	if (key == "range" || key == "radius" || key == "lightradius" || key == "lightattenuation")
	{
		if (has_float)
		{
			props.range = *float_value;
		}
		return;
	}

	if (key == "model" || key == "modelname" || key == "filename" || key == "texture" || key == "sound")
	{
		if (has_string && !str_value->empty())
		{
			props.resource = *str_value;
		}
		return;
	}

	if (key == "gravity")
	{
		if (has_float)
		{
			props.gravity = *float_value;
		}
		return;
	}

	if (key == "fogdensity")
	{
		if (has_float)
		{
			props.fog_density = *float_value;
		}
	}
}

bool ParseVector3Elements(CLTANode* value, float out[3])
{
	if (value == nullptr)
	{
		return false;
	}

	int count = 0;
	for (uint32_t i = 0; i < value->GetNumElements(); ++i)
	{
		CLTANode* elem = value->GetElement(i);
		if (!elem || !elem->IsAtom())
		{
			continue;
		}
		float parsed = 0.0f;
		if (TryParseFloat(elem->GetValue(), parsed))
		{
			out[count++] = parsed;
			if (count == 3)
			{
				return true;
			}
		}
	}
	return false;
}

bool ParseVector3(CLTANode* value, float out[3])
{
	if (value == nullptr)
	{
		return false;
	}

	if (value->IsAtom())
	{
		return false;
	}

	if (ParseVector3Elements(value, out))
	{
		return true;
	}

	if (!value->IsList() || value->GetNumElements() < 1)
	{
		return false;
	}

	CLTANode* first = value->GetElement(0);
	if (first && first->IsAtom())
	{
		const char* tag = first->GetValue();
		if (tag != nullptr)
		{
			const bool is_vector = (std::strcmp(tag, "vector") == 0);
			const bool is_angles = (std::strcmp(tag, "eulerangles") == 0 || std::strcmp(tag, "rotation") == 0);
			if ((is_vector || is_angles) && value->GetNumElements() >= 2)
			{
				CLTANode* nested = value->GetElement(1);
				if (nested && nested->IsList())
				{
					return ParseVector3Elements(nested, out);
				}
			}
		}
	}

	if (value->GetNumElements() == 1)
	{
		CLTANode* nested = value->GetElement(0);
		if (nested && nested->IsList())
		{
			return ParseVector3Elements(nested, out);
		}
	}

	return false;
}

bool ParseNodeListVector(CLTANode* value, float out[3])
{
	if (value == nullptr)
	{
		return false;
	}

	if (ParseVector3Elements(value, out))
	{
		return true;
	}

	if (!value->IsList() || value->GetNumElements() < 2)
	{
		return false;
	}

	CLTANode* nested = value->GetElement(1);
	if (nested == nullptr)
	{
		return false;
	}

	return ParseVector3(nested, out);
}

struct BrushGeometry
{
	bool valid = false;
	float centroid[3] = {0.0f, 0.0f, 0.0f};
	float bounds_min[3] = {0.0f, 0.0f, 0.0f};
	float bounds_max[3] = {0.0f, 0.0f, 0.0f};
	std::vector<float> vertices;
	std::vector<uint32_t> indices;
};

bool ParsePointListGeometry(CLTANode* pointlist, BrushGeometry& out)
{
	if (!pointlist || !pointlist->IsList())
	{
		return false;
	}

	out.vertices.clear();
	uint32_t start = 0;
	if (pointlist->GetNumElements() > 0)
	{
		CLTANode* first = pointlist->GetElement(0);
		if (first && first->IsAtom() && std::strcmp(first->GetValue(), "pointlist") == 0)
		{
			start = 1;
		}
	}

	bool has_point = false;
	double sum[3] = {0.0, 0.0, 0.0};
	int count = 0;

	for (uint32_t i = start; i < pointlist->GetNumElements(); ++i)
	{
		CLTANode* entry = pointlist->GetElement(i);
		if (!entry || !entry->IsList())
		{
			continue;
		}
		float pos[3] = {0.0f, 0.0f, 0.0f};
		if (!ParseVector3Elements(entry, pos))
		{
			continue;
		}
		out.vertices.push_back(pos[0]);
		out.vertices.push_back(pos[1]);
		out.vertices.push_back(pos[2]);
		if (!has_point)
		{
			out.bounds_min[0] = pos[0];
			out.bounds_min[1] = pos[1];
			out.bounds_min[2] = pos[2];
			out.bounds_max[0] = pos[0];
			out.bounds_max[1] = pos[1];
			out.bounds_max[2] = pos[2];
			has_point = true;
		}
		else
		{
			out.bounds_min[0] = std::min(out.bounds_min[0], pos[0]);
			out.bounds_min[1] = std::min(out.bounds_min[1], pos[1]);
			out.bounds_min[2] = std::min(out.bounds_min[2], pos[2]);
			out.bounds_max[0] = std::max(out.bounds_max[0], pos[0]);
			out.bounds_max[1] = std::max(out.bounds_max[1], pos[1]);
			out.bounds_max[2] = std::max(out.bounds_max[2], pos[2]);
		}
		sum[0] += pos[0];
		sum[1] += pos[1];
		sum[2] += pos[2];
		++count;
	}

	if (!has_point || count <= 0)
	{
		return false;
	}

	out.centroid[0] = static_cast<float>(sum[0] / count);
	out.centroid[1] = static_cast<float>(sum[1] / count);
	out.centroid[2] = static_cast<float>(sum[2] / count);
	return true;
}

bool ParseIndexList(CLTANode* list, std::vector<uint32_t>& out)
{
	out.clear();
	if (!list || !list->IsList() || list->GetNumElements() < 2)
	{
		return false;
	}

	uint32_t start = 0;
	if (list->GetNumElements() > 0)
	{
		CLTANode* first = list->GetElement(0);
		if (first && first->IsAtom())
		{
			start = 1;
		}
	}

	for (uint32_t i = start; i < list->GetNumElements(); ++i)
	{
		CLTANode* elem = list->GetElement(i);
		if (!elem || !elem->IsAtom())
		{
			continue;
		}
		const char* text = elem->GetValue();
		if (!text || text[0] == '\0')
		{
			continue;
		}
		char* end = nullptr;
		long parsed = std::strtol(text, &end, 10);
		if (end == text || parsed < 0)
		{
			continue;
		}
		out.push_back(static_cast<uint32_t>(parsed));
	}

	return out.size() >= 3;
}

void CollectBrushGeometry(CLTANode* polyhedronlist, std::vector<BrushGeometry>& out)
{
	out.clear();
	if (!polyhedronlist || !polyhedronlist->IsList())
	{
		return;
	}

	uint32_t start = 0;
	if (polyhedronlist->GetNumElements() > 0)
	{
		CLTANode* first = polyhedronlist->GetElement(0);
		if (first && first->IsAtom() && std::strcmp(first->GetValue(), "polyhedronlist") == 0)
		{
			start = 1;
		}
	}

	for (uint32_t i = start; i < polyhedronlist->GetNumElements(); ++i)
	{
		CLTANode* poly = polyhedronlist->GetElement(i);
		BrushGeometry geometry;
		if (poly && poly->IsList())
		{
			CLTANode* pointlist = CLTAUtil::ShallowFindList(poly, "pointlist");
			if (pointlist && ParsePointListGeometry(pointlist, geometry))
			{
				geometry.valid = true;
			}

			CLTANode* polylist = CLTAUtil::ShallowFindList(poly, "polylist");
			if (polylist && polylist->IsList())
			{
				uint32_t poly_start = 0;
				if (polylist->GetNumElements() > 0)
				{
					CLTANode* first = polylist->GetElement(0);
					if (first && first->IsAtom() && std::strcmp(first->GetValue(), "polylist") == 0)
					{
						poly_start = 1;
					}
				}
				std::vector<uint32_t> face_indices;
				const size_t vertex_count = geometry.vertices.size() / 3;
				for (uint32_t p = poly_start; p < polylist->GetNumElements(); ++p)
				{
					CLTANode* editpoly = polylist->GetElement(p);
					if (!editpoly || !editpoly->IsList())
					{
						continue;
					}
					CLTANode* face = CLTAUtil::ShallowFindList(editpoly, "f");
					if (!ParseIndexList(face, face_indices))
					{
						continue;
					}
					if (face_indices.size() < 3)
					{
						continue;
					}
					const uint32_t i0 = face_indices[0];
					for (size_t f = 1; f + 1 < face_indices.size(); ++f)
					{
						const uint32_t i1 = face_indices[f];
						const uint32_t i2 = face_indices[f + 1];
						if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
						{
							continue;
						}
						geometry.indices.push_back(i0);
						geometry.indices.push_back(i1);
						geometry.indices.push_back(i2);
					}
				}
			}
		}
		out.push_back(std::move(geometry));
	}
}

bool ExtractStringValue(CLTANode* value, std::string& out)
{
	if (value == nullptr)
	{
		return false;
	}

	if (value->IsAtom())
	{
		out = value->GetValue();
		return true;
	}

	if (value->IsList() && value->GetNumElements() >= 2)
	{
		CLTANode* candidate = value->GetElement(1);
		if (candidate && candidate->IsAtom())
		{
			out = candidate->GetValue();
			return true;
		}
	}

	if (value->IsList() && value->GetNumElements() == 1)
	{
		CLTANode* candidate = value->GetElement(0);
		if (candidate && candidate->IsAtom())
		{
			out = candidate->GetValue();
			return true;
		}
	}

	return false;
}

const char* GetChildAtomValue(CLTANode* node, const char* key)
{
	if (node == nullptr || key == nullptr)
	{
		return nullptr;
	}
	CLTANode* child = CLTAUtil::ShallowFindList(node, key);
	if (!child || !child->IsList() || child->GetNumElements() < 2)
	{
		return nullptr;
	}
	CLTANode* value = child->GetElement(1);
	if (!value || !value->IsAtom())
	{
		return nullptr;
	}
	return value->GetValue();
}

const char* FindListValueRecursive(CLTANode* node, const char* key, int depth_limit)
{
	if (node == nullptr || key == nullptr || depth_limit < 0)
	{
		return nullptr;
	}

	if (node->IsList() && node->GetNumElements() >= 2)
	{
		CLTANode* first = node->GetElement(0);
		if (first && first->IsAtom() && std::strcmp(first->GetValue(), key) == 0)
		{
			CLTANode* value = node->GetElement(1);
			if (value && value->IsAtom())
			{
				return value->GetValue();
			}
		}
	}

	if (!node->IsList())
	{
		return nullptr;
	}

	for (uint32_t i = 0; i < node->GetNumElements(); ++i)
	{
		CLTANode* child = node->GetElement(i);
		if (child && child->IsList())
		{
			const char* found = FindListValueRecursive(child, key, depth_limit - 1);
			if (found != nullptr)
			{
				return found;
			}
		}
	}

	return nullptr;
}

bool IsNodeList(CLTANode* node)
{
	if (!node || !node->IsList() || node->GetNumElements() == 0)
	{
		return false;
	}
	CLTANode* first = node->GetElement(0);
	if (first && first->IsAtom())
	{
		const char* value = first->GetValue();
		if (value && (std::strcmp(value, "worldnode") == 0 || std::strcmp(value, "node") == 0))
		{
			return true;
		}
	}
	return CLTAUtil::ShallowFindList(node, "childlist") != nullptr;
}

std::string GetSceneNodeLabel(CLTANode* node)
{
	if (const char* label = GetChildAtomValue(node, "label"))
	{
		if (label[0] != '\0')
		{
			return label;
		}
	}

	if (const char* name = FindListValueRecursive(node, "name", 4))
	{
		if (name[0] != '\0')
		{
			return name;
		}
	}

	if (const char* node_id = GetChildAtomValue(node, "nodeid"))
	{
		return std::string("Node ") + node_id;
	}

	return "Node";
}

std::string GetSceneNodeType(CLTANode* node)
{
	if (const char* type = GetChildAtomValue(node, "type"))
	{
		if (type[0] != '\0')
		{
			return type;
		}
	}
	return {};
}

void ApplyFlags(CLTANode* node, NodeProperties& props)
{
	CLTANode* flags = CLTAUtil::ShallowFindList(node, "flags");
	if (!flags || !flags->IsList())
	{
		return;
	}

	for (uint32_t i = 1; i < flags->GetNumElements(); ++i)
	{
		CLTANode* elem = flags->GetElement(i);
		if (!elem || !elem->IsAtom())
		{
			continue;
		}
		const std::string flag = ToLower(elem->GetValue());
		if (flag == "hidden")
		{
			props.visible = false;
		}
		else if (flag == "visible")
		{
			props.visible = true;
		}
		else if (flag == "frozen" || flag == "locked")
		{
			props.locked = true;
		}
	}
}

bool IsTypedPropertyToken(const char* value)
{
	if (value == nullptr)
	{
		return false;
	}
	return std::strcmp(value, "string") == 0 ||
		std::strcmp(value, "vector") == 0 ||
		std::strcmp(value, "rotation") == 0 ||
		std::strcmp(value, "longint") == 0 ||
		std::strcmp(value, "real") == 0 ||
		std::strcmp(value, "bool") == 0 ||
		std::strcmp(value, "color") == 0;
}

bool ParsePropertyEntry(CLTANode* entry, std::string& out_name, CLTANode*& out_value)
{
	out_name.clear();
	out_value = nullptr;

	if (!entry || !entry->IsList() || entry->GetNumElements() == 0)
	{
		return false;
	}

	CLTANode* first = entry->GetElement(0);
	if (first && first->IsAtom())
	{
		const char* first_value = first->GetValue();
		if (first_value == nullptr)
		{
			return false;
		}
		if (IsTypedPropertyToken(first_value))
		{
			if (entry->GetNumElements() >= 2)
			{
				CLTANode* name_node = entry->GetElement(1);
				if (name_node && name_node->IsAtom())
				{
					out_name = name_node->GetValue();
				}
			}
			CLTANode* data = CLTAUtil::ShallowFindList(entry, "data");
			if (data && data->IsList() && data->GetNumElements() >= 2)
			{
				out_value = data->GetElement(1);
			}
			if (!out_name.empty() && out_value != nullptr)
			{
				return true;
			}
			out_name.clear();
			out_value = nullptr;
		}
		if (std::strcmp(first_value, "propid") == 0)
		{
			return false;
		}
		if (std::strcmp(first_value, "prop") == 0 || std::strcmp(first_value, "property") == 0)
		{
			if (const char* name = GetChildAtomValue(entry, "name"))
			{
				out_name = name;
			}
			if (out_name.empty())
			{
				if (const char* name = GetChildAtomValue(entry, "propname"))
				{
					out_name = name;
				}
			}
			CLTANode* data = CLTAUtil::ShallowFindList(entry, "data");
			if (!data)
			{
				data = CLTAUtil::ShallowFindList(entry, "value");
			}
			if (!data)
			{
				data = CLTAUtil::ShallowFindList(entry, "val");
			}
			if (data && data->IsList() && data->GetNumElements() >= 2)
			{
				out_value = data->GetElement(1);
			}
			if (out_name.empty() || out_value == nullptr)
			{
				return false;
			}
			return true;
		}

		if (entry->GetNumElements() >= 2)
		{
			CLTANode* value = entry->GetElement(1);
			if (value)
			{
				out_name = first_value;
				out_value = value;
				return true;
			}
		}
	}

	return false;
}

std::string StringifyNodeValue(CLTANode* value)
{
	if (value == nullptr)
	{
		return {};
	}

	if (value->IsAtom())
	{
		return value->GetValue();
	}

	float vec3[3] = {0.0f, 0.0f, 0.0f};
	if (ParseVector3(value, vec3))
	{
		char buffer[128];
		std::snprintf(buffer, sizeof(buffer), "%.3f %.3f %.3f", vec3[0], vec3[1], vec3[2]);
		return buffer;
	}

	std::string out = "(";
	const uint32_t max_elems = std::min<uint32_t>(value->GetNumElements(), 6);
	for (uint32_t i = 0; i < max_elems; ++i)
	{
		CLTANode* elem = value->GetElement(i);
		if (!elem)
		{
			continue;
		}
		if (i > 0)
		{
			out += " ";
		}
		if (elem->IsAtom())
		{
			out += elem->GetValue();
		}
		else
		{
			out += "...";
		}
	}
	if (value->GetNumElements() > max_elems)
	{
		out += " ...";
	}
	out += ")";
	return out;
}

std::string StringifyListElements(CLTANode* list, uint32_t start_index)
{
	if (list == nullptr || !list->IsList() || start_index >= list->GetNumElements())
	{
		return {};
	}
	std::string out = "(";
	const uint32_t available = list->GetNumElements() - start_index;
	const uint32_t max_elems = std::min<uint32_t>(available, 6);
	for (uint32_t i = 0; i < max_elems; ++i)
	{
		CLTANode* elem = list->GetElement(start_index + i);
		if (!elem)
		{
			continue;
		}
		if (i > 0)
		{
			out += " ";
		}
		if (elem->IsAtom())
		{
			out += elem->GetValue();
		}
		else
		{
			out += "...";
		}
	}
	if (available > max_elems)
	{
		out += " ...";
	}
	out += ")";
	return out;
}

void AppendRawNodeFields(CLTANode* node, NodeProperties& props)
{
	if (!node || !node->IsList())
	{
		return;
	}
	for (uint32_t i = 0; i < node->GetNumElements(); ++i)
	{
		CLTANode* entry = node->GetElement(i);
		if (!entry || !entry->IsList() || entry->GetNumElements() < 2)
		{
			continue;
		}
		CLTANode* key_node = entry->GetElement(0);
		if (!key_node || !key_node->IsAtom())
		{
			continue;
		}
		const char* key_text = key_node->GetValue();
		if (!key_text || key_text[0] == '\0')
		{
			continue;
		}
		if (std::strcmp(key_text, "childlist") == 0 || std::strcmp(key_text, "properties") == 0)
		{
			continue;
		}
		std::string value_text;
		if (entry->GetNumElements() > 2)
		{
			value_text = StringifyListElements(entry, 1);
		}
		else
		{
			value_text = StringifyNodeValue(entry->GetElement(1));
		}
		props.raw_properties.emplace_back(key_text, value_text);
	}
}

bool TryGetRawPropertyInt(const NodeProperties& props, const char* key, int& out)
{
	if (key == nullptr)
	{
		return false;
	}
	for (const auto& entry : props.raw_properties)
	{
		if (entry.first == key)
		{
			const char* text = entry.second.c_str();
			if (!text || text[0] == '\0')
			{
				return false;
			}
			char* end = nullptr;
			long parsed = std::strtol(text, &end, 10);
			if (end == text)
			{
				return false;
			}
			out = static_cast<int>(parsed);
			return true;
		}
	}
	return false;
}

bool ApplyProperty(NodeProperties& props, TreeNode& node, const std::string& name, CLTANode* value)
{
	if (name.empty() || value == nullptr)
	{
		return false;
	}

	const std::string key = ToLower(name);
	std::string string_value;
	ExtractStringValue(value, string_value);
	string_value = TrimString(string_value);

	if (key == "classname" || key == "class")
	{
		if (!string_value.empty())
		{
			props.class_name = string_value;
		}
		return true;
	}

	if ((key == "name" || key == "objectname") && !string_value.empty())
	{
		if (node.name.empty() || node.name == "Node" || StartsWith(node.name, "Node "))
		{
			node.name = string_value;
		}
		return true;
	}

	float vec3[3] = {0.0f, 0.0f, 0.0f};
	if (key == "pos" || key == "position" || key == "m_pos")
	{
		if (ParseVector3(value, vec3))
		{
			props.position[0] = vec3[0];
			props.position[1] = vec3[1];
			props.position[2] = vec3[2];
		}
		return true;
	}

	if (key == "rot" || key == "rotation" || key == "m_rot")
	{
		if (ParseVector3(value, vec3))
		{
			props.rotation[0] = vec3[0];
			props.rotation[1] = vec3[1];
			props.rotation[2] = vec3[2];
		}
		return true;
	}

	if (key == "scale" || key == "m_scale")
	{
		if (ParseVector3(value, vec3))
		{
			props.scale[0] = vec3[0];
			props.scale[1] = vec3[1];
			props.scale[2] = vec3[2];
		}
		return true;
	}

	if (key == "dims" || key == "size")
	{
		if (ParseVector3(value, vec3))
		{
			props.size[0] = vec3[0];
			props.size[1] = vec3[1];
			props.size[2] = vec3[2];
		}
		return true;
	}

	if (key == "color" || key == "lightcolor" || key == "innercolor" || key == "outercolor")
	{
		if (ParseVector3(value, vec3))
		{
			if (vec3[0] > 1.0f || vec3[1] > 1.0f || vec3[2] > 1.0f)
			{
				vec3[0] /= 255.0f;
				vec3[1] /= 255.0f;
				vec3[2] /= 255.0f;
			}
			props.color[0] = vec3[0];
			props.color[1] = vec3[1];
			props.color[2] = vec3[2];
		}
		return true;
	}

	if (key == "ambient")
	{
		if (ParseVector3(value, vec3))
		{
			props.ambient[0] = vec3[0];
			props.ambient[1] = vec3[1];
			props.ambient[2] = vec3[2];
		}
		return true;
	}

	if (key == "visible" || key == "hidden" || key == "locked" || key == "frozen")
	{
		bool parsed = false;
		bool bool_value = false;
		if (!string_value.empty())
		{
			parsed = TryParseBool(string_value.c_str(), bool_value);
		}
		if (!parsed)
		{
			float parsed_num = 0.0f;
			if (TryParseFloat(string_value.c_str(), parsed_num))
			{
				bool_value = (parsed_num != 0.0f);
				parsed = true;
			}
			else if (ParseVector3(value, vec3))
			{
				bool_value = (vec3[0] != 0.0f);
				parsed = true;
			}
		}
		if (parsed)
		{
			if (key == "visible")
			{
				props.visible = bool_value;
			}
			else if (key == "hidden")
			{
				props.visible = !bool_value;
			}
			else
			{
				props.locked = bool_value;
			}
		}
		return true;
	}

	if (key == "intensity" || key == "brightness" || key == "brightscale" || key == "objectbrightscale")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.intensity = parsed;
		}
		return true;
	}

	if (key == "range" || key == "radius" || key == "lightradius")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.range = parsed;
		}
		return true;
	}

	if (key == "model" || key == "modelname" || key == "filename" || key == "texture" || key == "sound")
	{
		if (!string_value.empty())
		{
			props.resource = string_value;
		}
		return true;
	}

	if (key == "gravity")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.gravity = parsed;
		}
		return true;
	}

	if (key == "fogdensity")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.fog_density = parsed;
		}
		return true;
	}

	if (key == "backgroundcolor")
	{
		if (ParseVector3(value, vec3))
		{
			if (vec3[0] > 1.0f || vec3[1] > 1.0f || vec3[2] > 1.0f)
			{
				vec3[0] /= 255.0f;
				vec3[1] /= 255.0f;
				vec3[2] /= 255.0f;
			}
			props.background_color[0] = vec3[0];
			props.background_color[1] = vec3[1];
			props.background_color[2] = vec3[2];
		}
		return true;
	}

	if (key == "fogcolor")
	{
		if (ParseVector3(value, vec3))
		{
			if (vec3[0] > 1.0f || vec3[1] > 1.0f || vec3[2] > 1.0f)
			{
				vec3[0] /= 255.0f;
				vec3[1] /= 255.0f;
				vec3[2] /= 255.0f;
			}
			props.color[0] = vec3[0];
			props.color[1] = vec3[1];
			props.color[2] = vec3[2];
		}
		return true;
	}

	if (key == "fogenable")
	{
		bool parsed = false;
		bool bool_value = false;
		if (!string_value.empty())
		{
			parsed = TryParseBool(string_value.c_str(), bool_value);
			if (!parsed)
			{
				float parsed_num = 0.0f;
				if (TryParseFloat(string_value.c_str(), parsed_num))
				{
					bool_value = (parsed_num != 0.0f);
					parsed = true;
				}
			}
		}
		if (parsed)
		{
			props.fog_enabled = bool_value;
		}
		return true;
	}

	if (key == "fognearz")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.fog_near = parsed;
		}
		return true;
	}

	if (key == "fogfarz")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.fog_far = parsed;
		}
		return true;
	}

	if (key == "farz")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.far_z = parsed;
		}
		return true;
	}

	if (key == "skypantexture")
	{
		if (!string_value.empty())
		{
			props.sky_pan_texture = string_value;
		}
		return true;
	}

	if (key == "skypanenable")
	{
		bool parsed = false;
		bool bool_value = false;
		if (!string_value.empty())
		{
			parsed = TryParseBool(string_value.c_str(), bool_value);
			if (!parsed)
			{
				float parsed_num = 0.0f;
				if (TryParseFloat(string_value.c_str(), parsed_num))
				{
					bool_value = (parsed_num != 0.0f);
					parsed = true;
				}
			}
		}
		if (parsed)
		{
			props.sky_pan_enabled = bool_value;
		}
		return true;
	}

	if (key == "skypanscalex")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.sky_pan_scale[0] = parsed;
		}
		return true;
	}

	if (key == "skypanscalez")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.sky_pan_scale[1] = parsed;
		}
		return true;
	}

	if (key == "skypanautopanx")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.sky_pan_auto_pan[0] = parsed;
		}
		return true;
	}

	if (key == "skypanautopanz")
	{
		float parsed = 0.0f;
		if (TryParseFloat(string_value.c_str(), parsed))
		{
			props.sky_pan_auto_pan[1] = parsed;
		}
		return true;
	}

	if (key == "ambientlight")
	{
		if (ParseVector3(value, vec3))
		{
			if (vec3[0] > 1.0f || vec3[1] > 1.0f || vec3[2] > 1.0f)
			{
				vec3[0] /= 255.0f;
				vec3[1] /= 255.0f;
				vec3[2] /= 255.0f;
			}
			props.ambient[0] = vec3[0];
			props.ambient[1] = vec3[1];
			props.ambient[2] = vec3[2];
		}
		return true;
	}

	if (key == "castshadows" || key == "castshadowmesh")
	{
		bool parsed = false;
		bool bool_value = false;
		if (!string_value.empty())
		{
			parsed = TryParseBool(string_value.c_str(), bool_value);
			if (!parsed)
			{
				float parsed_num = 0.0f;
				if (TryParseFloat(string_value.c_str(), parsed_num))
				{
					bool_value = (parsed_num != 0.0f);
					parsed = true;
				}
			}
		}
		if (parsed)
		{
			props.cast_shadows = bool_value;
		}
		return true;
	}

	return false;
}

void ApplyPropertyList(NodeProperties& props, TreeNode& node, CLTANode* prop_list)
{
	if (!prop_list || !prop_list->IsList())
	{
		return;
	}

	for (uint32_t i = 0; i < prop_list->GetNumElements(); ++i)
	{
		CLTANode* entry = prop_list->GetElement(i);
		std::string prop_name;
		CLTANode* value = nullptr;
		if (ParsePropertyEntry(entry, prop_name, value))
		{
			props.raw_properties.emplace_back(prop_name, StringifyNodeValue(value));
			ApplyProperty(props, node, prop_name, value);
		}
	}
}

int ExtractPropId(CLTANode* properties_node)
{
	if (!properties_node || !properties_node->IsList())
	{
		return -1;
	}
	CLTANode* propid_node = CLTAUtil::ShallowFindList(properties_node, "propid");
	if (!propid_node || !propid_node->IsList() || propid_node->GetNumElements() < 2)
	{
		return -1;
	}
	CLTANode* value = propid_node->GetElement(1);
	if (!value || !value->IsAtom())
	{
		return -1;
	}
	const char* text = value->GetValue();
	if (!text)
	{
		return -1;
	}
	char* end = nullptr;
	long parsed = std::strtol(text, &end, 10);
	if (end == text)
	{
		return -1;
	}
	return static_cast<int>(parsed);
}

void CollectGlobalPropLists(CLTANode* global_node, std::vector<CLTANode*>& out_lists)
{
	out_lists.clear();
	if (!global_node || !global_node->IsList())
	{
		return;
	}

	for (uint32_t i = 0; i < global_node->GetNumElements(); ++i)
	{
		CLTANode* entry = global_node->GetElement(i);
		if (!entry || !entry->IsList() || entry->GetNumElements() == 0)
		{
			continue;
		}
		CLTANode* first = entry->GetElement(0);
		if (first && first->IsAtom() && std::strcmp(first->GetValue(), "proplist") == 0)
		{
			if (entry->GetNumElements() >= 2)
			{
				CLTANode* list = entry->GetElement(1);
				if (list && list->IsList())
				{
					out_lists.push_back(list);
					continue;
				}
			}
			out_lists.push_back(entry);
		}
	}
}

std::string InferAssetType(const fs::path& path)
{
	std::string ext = ToLower(path.extension().string());
	if (ext == ".lta" || ext == ".ltc" || ext == ".tbw" || ext == ".ed")
	{
		return "World";
	}
	if (ext == ".dtx" || ext == ".dds" || ext == ".png" || ext == ".tga" || ext == ".jpg" || ext == ".jpeg" ||
		ext == ".bmp" || ext == ".gif")
	{
		return "Texture";
	}
	if (ext == ".abc" || ext == ".ltm" || ext == ".lmo")
	{
		return "Model";
	}
	if (ext == ".prefab")
	{
		return "Prefab";
	}
	if (ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac")
	{
		return "Sound";
	}
	if (ext == ".txt" || ext == ".ini" || ext == ".cfg")
	{
		return "Config";
	}
	if (ext == ".lua" || ext == ".js")
	{
		return "Script";
	}
	return "Asset";
}

bool ShouldSkipProjectEntry(const fs::path& path, bool is_dir)
{
	const std::string name = path.filename().string();
	if (name.empty())
	{
		return false;
	}
	if (name[0] == '.')
	{
		return true;
	}
	if (is_dir && (name == "build" || name == "cmake"))
	{
		return true;
	}
	return false;
}

std::string MakeRelativePathString(const fs::path& path, const fs::path& root)
{
	std::error_code ec;
	fs::path relative = fs::relative(path, root, ec);
	if (ec)
	{
		return path.string();
	}
	return relative.generic_string();
}

int AddProjectEntry(
	const fs::path& path,
	const fs::path& root,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	size_t& total_nodes,
	std::string& error)
{
	if (total_nodes >= kMaxProjectNodes)
	{
		error = "Project tree truncated (too many entries).";
		return -1;
	}

	std::error_code ec;
	const bool is_dir = fs::is_directory(path, ec);
	if (ec)
	{
		return -1;
	}
	if (ShouldSkipProjectEntry(path, is_dir))
	{
		return -1;
	}

	TreeNode node;
	node.name = path.filename().string();
	if (node.name.empty())
	{
		node.name = path.string();
	}
	node.is_folder = is_dir;
	nodes.push_back(node);

	NodeProperties node_props;
	if (is_dir)
	{
		node_props = MakeProps("Folder");
	}
	else
	{
		node_props = MakeProps(InferAssetType(path).c_str());
	}
	node_props.resource = MakeRelativePathString(path, root);
	props.push_back(node_props);

	const int node_id = static_cast<int>(nodes.size() - 1);
	++total_nodes;

	if (!is_dir)
	{
		return node_id;
	}

	std::vector<fs::directory_entry> entries;
	for (const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec))
	{
		if (ec)
		{
			continue;
		}
		if (entry.is_symlink(ec))
		{
			continue;
		}
		entries.push_back(entry);
	}

	std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b)
		{
			const bool a_dir = a.is_directory();
			const bool b_dir = b.is_directory();
			if (a_dir != b_dir)
			{
				return a_dir > b_dir;
			}
			const std::string a_name = ToLower(a.path().filename().string());
			const std::string b_name = ToLower(b.path().filename().string());
			return a_name < b_name;
		});

	for (const auto& entry : entries)
	{
		const int child_id = AddProjectEntry(entry.path(), root, nodes, props, total_nodes, error);
		if (child_id >= 0)
		{
			nodes[node_id].children.push_back(child_id);
		}
	}

	return node_id;
}

int BuildSceneNodeRecursive(
	CLTANode* node,
	const std::vector<CLTANode*>& global_prop_lists,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	bool is_root)
{
	if (!node)
	{
		return -1;
	}

	TreeNode tree_node;
	tree_node.name = GetSceneNodeLabel(node);

	NodeProperties node_props;
	const std::string raw_type = GetSceneNodeType(node);
	if (is_root && IsNodeList(node))
	{
		CLTANode* first = node->GetElement(0);
		if (first && first->IsAtom() && std::strcmp(first->GetValue(), "worldnode") == 0)
		{
			node_props = MakeProps("World");
		}
	}

	if (node_props.type.empty())
	{
		if (!raw_type.empty())
		{
			node_props = MakeProps(raw_type.c_str());
		}
		else
		{
			node_props = MakeProps("Entity");
		}
	}

	if (const char* class_name = FindListValueRecursive(node, "classname", 4))
	{
		node_props.class_name = class_name;
	}
	else if (const char* class_name = FindListValueRecursive(node, "class", 4))
	{
		node_props.class_name = class_name;
	}
	else if (!raw_type.empty())
	{
		node_props.class_name = raw_type;
	}

	if (!is_root && (node_props.type == "World" || node_props.type == "Entity"))
	{
		const std::string& source = node_props.class_name.empty() ? raw_type : node_props.class_name;
		if (!source.empty())
		{
			const std::string mapped = MapBinaryObjectType(source);
			if (!mapped.empty() && mapped != "World")
			{
				node_props.type = mapped;
			}
		}
	}

	ApplyFlags(node, node_props);

	CLTANode* properties_node = CLTAUtil::ShallowFindList(node, "properties");
	if (properties_node && properties_node->IsList())
	{
		int prop_id = ExtractPropId(properties_node);
		if (prop_id >= 0 && prop_id < static_cast<int>(global_prop_lists.size()))
		{
			ApplyPropertyList(node_props, tree_node, global_prop_lists[static_cast<size_t>(prop_id)]);
		}
		else
		{
			ApplyPropertyList(node_props, tree_node, properties_node);
		}
	}
	if (properties_node && properties_node->IsList())
	{
		if (const char* prop_class_name = GetChildAtomValue(properties_node, "name"))
		{
			if (prop_class_name[0] != '\0')
			{
				const std::string raw_type_lower = ToLower(raw_type);
				const std::string class_lower = ToLower(node_props.class_name);
				if (node_props.class_name.empty() || class_lower == raw_type_lower)
				{
					node_props.class_name = prop_class_name;
				}
			}
		}
	}
	if (!is_root)
	{
		const std::string type_lower = ToLower(node_props.type);
		if (type_lower == "object" || type_lower == "entity")
		{
			const std::string& source = node_props.class_name.empty() ? raw_type : node_props.class_name;
			if (!source.empty())
			{
				const std::string mapped = MapBinaryObjectType(source);
				if (!mapped.empty() && mapped != "World")
				{
					node_props.type = mapped;
				}
			}
		}
	}

	float vec3[3] = {0.0f, 0.0f, 0.0f};
	CLTANode* position = CLTAUtil::ShallowFindList(node, "position");
	if (!position)
	{
		position = CLTAUtil::ShallowFindList(node, "pos");
	}
	if (position && ParseNodeListVector(position, vec3))
	{
		node_props.position[0] = vec3[0];
		node_props.position[1] = vec3[1];
		node_props.position[2] = vec3[2];
	}

	CLTANode* orientation = CLTAUtil::ShallowFindList(node, "orientation");
	if (orientation && ParseNodeListVector(orientation, vec3))
	{
		node_props.rotation[0] = vec3[0];
		node_props.rotation[1] = vec3[1];
		node_props.rotation[2] = vec3[2];
	}
	else
	{
		CLTANode* rotation = CLTAUtil::ShallowFindList(node, "rotation");
		if (rotation && ParseNodeListVector(rotation, vec3))
		{
			node_props.rotation[0] = vec3[0];
			node_props.rotation[1] = vec3[1];
			node_props.rotation[2] = vec3[2];
		}
	}

	CLTANode* scale = CLTAUtil::ShallowFindList(node, "scale");
	if (scale && ParseNodeListVector(scale, vec3))
	{
		node_props.scale[0] = vec3[0];
		node_props.scale[1] = vec3[1];
		node_props.scale[2] = vec3[2];
	}

	AppendRawNodeFields(node, node_props);

	nodes.push_back(tree_node);
	props.push_back(node_props);
	const int node_id = static_cast<int>(nodes.size() - 1);

	CLTANode* childlist = CLTAUtil::ShallowFindList(node, "childlist");
	if (childlist && childlist->IsList() && childlist->GetNumElements() >= 2)
	{
		CLTANode* list = childlist->GetElement(1);
		if (list && list->IsList())
		{
			for (uint32_t i = 0; i < list->GetNumElements(); ++i)
			{
				CLTANode* child = list->GetElement(i);
				if (!IsNodeList(child))
				{
					continue;
				}
				const int child_id = BuildSceneNodeRecursive(child, global_prop_lists, nodes, props, false);
				if (child_id >= 0)
				{
					nodes[node_id].children.push_back(child_id);
				}
			}
		}
	}

	nodes[node_id].is_folder = !nodes[node_id].children.empty();
	return node_id;
}
} // namespace

NodeProperties MakeProps(const char* type)
{
	NodeProperties props;
	props.type = type;
	if (props.type == "World")
	{
		props.gravity = -9.8f;
		props.fog_density = 0.005f;
	}
	else if (props.type == "Light")
	{
		props.intensity = 5.0f;
		props.range = 25.0f;
		props.cast_shadows = true;
	}
	else if (props.type == "Terrain")
	{
		props.height_scale = 15.0f;
	}
	else if (props.type == "Texture")
	{
		props.srgb = true;
		props.mipmaps = true;
		props.compression_mode = 1;
	}
	else if (props.type == "Sound")
	{
		props.volume = 0.8f;
		props.loop = false;
	}
	return props;
}

std::vector<TreeNode> BuildProjectTree(
	const std::string& root_path,
	std::vector<NodeProperties>& out_props,
	std::string& error)
{
	error.clear();
	out_props.clear();

	if (root_path.empty())
	{
		return {};
	}

	std::error_code ec;
	fs::path root = fs::path(root_path);
	if (!fs::exists(root, ec))
	{
		error = "Project root does not exist.";
		return {};
	}

	std::vector<TreeNode> nodes;
	size_t total_nodes = 0;
	AddProjectEntry(root, root, nodes, out_props, total_nodes, error);

	if (nodes.empty() || (nodes.size() == 1 && nodes[0].children.empty()))
	{
		error = "No project content found.";
	}
	return nodes;
}

std::vector<TreeNode> BuildSceneTree(
	const std::string& world_file,
	std::vector<NodeProperties>& out_props,
	std::string& error)
{
	error.clear();
	out_props.clear();

	if (world_file.empty())
	{
		return {};
	}

	fs::path world_path = fs::path(world_file);
	if (!fs::exists(world_path))
	{
		error = "World file does not exist.";
		return {};
	}

	const std::string ext = ToLower(world_path.extension().string());
	if (ext == ".dat")
	{
		BspWorldView view;
		std::string view_error;
		if (!GetBspWorldView(world_file, view, view_error))
		{
			error = view_error.empty() ? "Failed to load compiled world." : view_error;
			return {};
		}
		return BuildSceneTreeFromBsp(view, out_props, error);
	}

	CLTALoadOnlyAlloc allocator(512 * 1024);
	CLTAReader reader;
	if (!reader.Open(world_path.string().c_str(), CLTAUtil::IsFileCompressed(world_path.string().c_str())))
	{
		error = "Failed to open world file.";
		return {};
	}

	CLTANode* hierarchy = CLTANodeReader::LoadNode(&reader, "nodehierarchy", &allocator);
	reader.Close();
	if (!hierarchy)
	{
		error = "World file missing nodehierarchy.";
		allocator.FreeAllMemory();
		return {};
	}

	std::vector<CLTANode*> global_prop_lists;
	CLTAReader prop_reader;
	if (prop_reader.Open(world_path.string().c_str(), CLTAUtil::IsFileCompressed(world_path.string().c_str())))
	{
		if (CLTANode* global_props = CLTANodeReader::LoadNode(&prop_reader, "globalproplist", &allocator))
		{
			CollectGlobalPropLists(global_props, global_prop_lists);
		}
		prop_reader.Close();
	}
	std::vector<BrushGeometry> brush_geometry;
	CLTAReader poly_reader;
	if (poly_reader.Open(world_path.string().c_str(), CLTAUtil::IsFileCompressed(world_path.string().c_str())))
	{
		if (CLTANode* polyhedronlist = CLTANodeReader::LoadNode(&poly_reader, "polyhedronlist", &allocator))
		{
			CollectBrushGeometry(polyhedronlist, brush_geometry);
		}
		poly_reader.Close();
	}

	CLTANode* root = CLTAUtil::ShallowFindList(hierarchy, "worldnode");
	if (!root)
	{
		for (uint32_t i = 0; i < hierarchy->GetNumElements(); ++i)
		{
			CLTANode* candidate = hierarchy->GetElement(i);
			if (IsNodeList(candidate))
			{
				root = candidate;
				break;
			}
		}
	}

	std::vector<TreeNode> nodes;
	if (!root)
	{
		error = "World file has no root node.";
		allocator.FreeAllMemory();
		return {};
	}

	BuildSceneNodeRecursive(root, global_prop_lists, nodes, out_props, true);
	if (!brush_geometry.empty())
	{
		for (auto& props : out_props)
		{
			int brush_index = -1;
			if (!TryGetRawPropertyInt(props, "brushindex", brush_index))
			{
				continue;
			}
			if (brush_index < 0 || static_cast<size_t>(brush_index) >= brush_geometry.size())
			{
				continue;
			}
			const BrushGeometry& bounds = brush_geometry[static_cast<size_t>(brush_index)];
			if (!bounds.valid)
			{
				continue;
			}
			props.brush_index = brush_index;
			props.brush_vertices = bounds.vertices;
			props.brush_indices = bounds.indices;
			char buffer[128];
			std::snprintf(buffer, sizeof(buffer), "%.3f %.3f %.3f",
				bounds.bounds_min[0], bounds.bounds_min[1], bounds.bounds_min[2]);
			props.raw_properties.emplace_back("bounds_min", buffer);
			std::snprintf(buffer, sizeof(buffer), "%.3f %.3f %.3f",
				bounds.bounds_max[0], bounds.bounds_max[1], bounds.bounds_max[2]);
			props.raw_properties.emplace_back("bounds_max", buffer);
			std::snprintf(buffer, sizeof(buffer), "%.3f %.3f %.3f",
				bounds.centroid[0], bounds.centroid[1], bounds.centroid[2]);
			props.raw_properties.emplace_back("centroid", buffer);

			if (props.position[0] == 0.0f && props.position[1] == 0.0f && props.position[2] == 0.0f)
			{
				props.position[0] = bounds.centroid[0];
				props.position[1] = bounds.centroid[1];
				props.position[2] = bounds.centroid[2];
			}
		}
	}

	allocator.FreeAllMemory();

	if (nodes.empty())
	{
		error = "World file contained no nodes.";
	}

	return nodes;
}

std::vector<TreeNode> BuildSceneTreeFromBsp(
	const BspWorldView& view,
	std::vector<NodeProperties>& out_props,
	std::string& error,
	const BspTreeBuildOptions& options)
{
	error.clear();
	out_props.clear();

	std::vector<TreeNode> nodes;
	auto add_node = [&](const std::string& name, bool is_folder, const char* type_name, int parent_id) -> int
	{
		TreeNode node;
		node.name = name;
		node.is_folder = is_folder;
		node.deleted = false;
		const int node_id = static_cast<int>(nodes.size());
		nodes.push_back(std::move(node));

		NodeProperties props = MakeProps(type_name != nullptr ? type_name : "Entity");
		if (type_name != nullptr)
		{
			props.class_name = type_name;
		}
		out_props.push_back(std::move(props));

		if (parent_id >= 0 && parent_id < static_cast<int>(nodes.size()))
		{
			nodes[parent_id].children.push_back(node_id);
			nodes[parent_id].is_folder = true;
		}
		return node_id;
	};

	const std::string root_name = view.world_name.empty() ? "World" : view.world_name;
	const int root_id = add_node(root_name, true, "World", -1);
	out_props[root_id].class_name = "World";
	out_props[root_id].raw_properties.emplace_back("world_bounds_min",
		std::to_string(view.world_bounds.min[0]) + " " +
		std::to_string(view.world_bounds.min[1]) + " " +
		std::to_string(view.world_bounds.min[2]));
	out_props[root_id].raw_properties.emplace_back("world_bounds_max",
		std::to_string(view.world_bounds.max[0]) + " " +
		std::to_string(view.world_bounds.max[1]) + " " +
		std::to_string(view.world_bounds.max[2]));

	int models_root_id = -1;
	if (!view.worldmodels.empty())
	{
		models_root_id = add_node("WorldModels", true, "Folder", root_id);
	}

	std::vector<size_t> model_indices(view.worldmodels.size());
	for (size_t i = 0; i < model_indices.size(); ++i)
	{
		model_indices[i] = i;
	}
	std::sort(model_indices.begin(), model_indices.end(),
		[&view](size_t a, size_t b)
		{
			const auto& ma = view.worldmodels[a];
			const auto& mb = view.worldmodels[b];
			if (ma.name != mb.name)
			{
				return ma.name < mb.name;
			}
			return ma.id < mb.id;
		});

	for (size_t model_index : model_indices)
	{
		const auto& model = view.worldmodels[model_index];
		std::string model_name = model.name;
		if (model_name.empty())
		{
			model_name = "WorldModel " + std::to_string(model.id);
		}
		const int model_id = add_node(model_name, true, "WorldModel", models_root_id >= 0 ? models_root_id : root_id);
		out_props[model_id].class_name = "WorldModel";
		out_props[model_id].raw_properties.emplace_back("model_id", std::to_string(model.id));
		out_props[model_id].raw_properties.emplace_back("model_bounds_min",
			std::to_string(model.bounds.min[0]) + " " +
			std::to_string(model.bounds.min[1]) + " " +
			std::to_string(model.bounds.min[2]));
		out_props[model_id].raw_properties.emplace_back("model_bounds_max",
			std::to_string(model.bounds.max[0]) + " " +
			std::to_string(model.bounds.max[1]) + " " +
			std::to_string(model.bounds.max[2]));

		if (!model.surfaces.empty())
		{
			const int surfaces_root_id = add_node("Surfaces", true, "Folder", model_id);
			std::vector<size_t> surface_indices(model.surfaces.size());
			for (size_t i = 0; i < surface_indices.size(); ++i)
			{
				surface_indices[i] = i;
			}
			std::sort(surface_indices.begin(), surface_indices.end(),
				[&model](size_t a, size_t b)
				{
					const auto& sa = model.surfaces[a];
					const auto& sb = model.surfaces[b];
					if (sa.material != sb.material)
					{
						return sa.material < sb.material;
					}
					if (sa.render_group != sb.render_group)
					{
						return sa.render_group < sb.render_group;
					}
					if (sa.lightmap_index != sb.lightmap_index)
					{
						return sa.lightmap_index < sb.lightmap_index;
					}
					return sa.id < sb.id;
				});

			std::unordered_map<std::string, int> group_ids;
			group_ids.reserve(model.surfaces.size());

			for (size_t surface_index : surface_indices)
			{
				const auto& surface = model.surfaces[surface_index];
				std::string group_name;
				if (options.grouping == BspTreeBuildOptions::SurfaceGrouping::RenderGroup)
				{
					group_name = "RenderGroup " + std::to_string(surface.render_group);
				}
				else if (options.grouping == BspTreeBuildOptions::SurfaceGrouping::Lightmap)
				{
					group_name = "Lightmap " + std::to_string(surface.lightmap_index);
				}
				else
				{
					group_name = surface.material;
					if (group_name.empty())
					{
						if (surface.material_id != 0)
						{
							group_name = "Material " + std::to_string(surface.material_id);
						}
						else
						{
							group_name = "Material";
						}
					}
				}

				int group_id = -1;
				auto found = group_ids.find(group_name);
				if (found == group_ids.end())
				{
					group_id = add_node(group_name, true, "Folder", surfaces_root_id);
					group_ids.emplace(group_name, group_id);
				}
				else
				{
					group_id = found->second;
				}

				std::string surface_name = "Surface " + std::to_string(surface.id);
				if (surface.poly_count > 0)
				{
					surface_name += " (" + std::to_string(surface.poly_count) + " polys)";
				}
				if (surface.render_group >= 0)
				{
					surface_name += " RG" + std::to_string(surface.render_group);
				}
				if (surface.lightmap_index >= 0)
				{
					surface_name += " LM" + std::to_string(surface.lightmap_index);
				}

				const int surface_id = add_node(surface_name, false, "Surface", group_id);
				NodeProperties& surface_props = out_props[surface_id];
				surface_props.class_name = "Surface";
				surface_props.resource = surface.material;
				surface_props.raw_properties.emplace_back("surface_id", std::to_string(surface.id));
				surface_props.raw_properties.emplace_back("material_id", std::to_string(surface.material_id));
				surface_props.raw_properties.emplace_back("render_group", std::to_string(surface.render_group));
				surface_props.raw_properties.emplace_back("lightmap_index", std::to_string(surface.lightmap_index));
				surface_props.raw_properties.emplace_back("surface_flags", std::to_string(surface.surface_flags));
				surface_props.raw_properties.emplace_back("poly_count", std::to_string(surface.poly_count));
				surface_props.raw_properties.emplace_back("centroid",
					std::to_string(surface.centroid[0]) + " " +
					std::to_string(surface.centroid[1]) + " " +
					std::to_string(surface.centroid[2]));
				surface_props.raw_properties.emplace_back("bounds_min",
					std::to_string(surface.bounds.min[0]) + " " +
					std::to_string(surface.bounds.min[1]) + " " +
					std::to_string(surface.bounds.min[2]));
				surface_props.raw_properties.emplace_back("bounds_max",
					std::to_string(surface.bounds.max[0]) + " " +
					std::to_string(surface.bounds.max[1]) + " " +
					std::to_string(surface.bounds.max[2]));
			}
		}
	}

	int objects_root_id = -1;
	if (!view.objects.empty())
	{
		objects_root_id = add_node("Objects", true, "Folder", root_id);
	}

	if (!view.objects.empty())
	{
		std::vector<size_t> object_indices(view.objects.size());
		for (size_t i = 0; i < object_indices.size(); ++i)
		{
			object_indices[i] = i;
		}
		std::sort(object_indices.begin(), object_indices.end(),
			[&view](size_t a, size_t b)
			{
				const auto& oa = view.objects[a];
				const auto& ob = view.objects[b];
				if (oa.class_name != ob.class_name)
				{
					return oa.class_name < ob.class_name;
				}
				if (oa.name != ob.name)
				{
					return oa.name < ob.name;
				}
				return oa.id < ob.id;
			});

		std::unordered_map<std::string, int> class_groups;
		class_groups.reserve(view.objects.size());

		for (size_t object_index : object_indices)
		{
			const auto& object = view.objects[object_index];
			std::string class_name = object.class_name.empty() ? "Entity" : object.class_name;
			std::string object_type = MapBinaryObjectType(class_name);
			int class_id = -1;
			auto found = class_groups.find(class_name);
			if (found == class_groups.end())
			{
				class_id = add_node(class_name, true, "Folder", objects_root_id >= 0 ? objects_root_id : root_id);
				class_groups.emplace(class_name, class_id);
			}
			else
			{
				class_id = found->second;
			}

			std::string object_name = object.name;
			if (object_name.empty())
			{
				object_name = class_name + " " + std::to_string(object.id);
			}

			const int object_id = add_node(object_name, false, object_type.c_str(), class_id);
			NodeProperties& object_props = out_props[object_id];
			object_props.class_name = class_name;
			object_props.position[0] = object.position[0];
			object_props.position[1] = object.position[1];
			object_props.position[2] = object.position[2];
			object_props.rotation[0] = object.rotation[0];
			object_props.rotation[1] = object.rotation[1];
			object_props.rotation[2] = object.rotation[2];
			object_props.raw_properties.emplace_back("object_id", std::to_string(object.id));
		}
	}

	return nodes;
}

const char* GetNodeName(const std::vector<TreeNode>& nodes, int node_id)
{
	if (node_id < 0 || node_id >= static_cast<int>(nodes.size()))
	{
		return "None";
	}
	if (nodes[node_id].deleted)
	{
		return "None";
	}
	return nodes[node_id].name.c_str();
}

int AddChildNode(
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int parent_id,
	const char* name,
	bool is_folder,
	const char* type_name)
{
	TreeNode node;
	node.name = name;
	node.is_folder = is_folder;
	node.deleted = false;
	const int new_id = static_cast<int>(nodes.size());
	nodes.push_back(std::move(node));
	props.push_back(MakeProps(type_name));
	if (!is_folder && type_name)
	{
		props.back().resource = name;
	}
	if (parent_id >= 0 && parent_id < static_cast<int>(nodes.size()))
	{
		nodes[parent_id].children.push_back(new_id);
	}
	return new_id;
}
