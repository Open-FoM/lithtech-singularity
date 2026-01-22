#include "diligent_buffers.h"
#include "diligent_state.h"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_buffer(
	uint32 size,
	Diligent::BIND_FLAGS bind_flags,
	const void* data,
	uint32 stride)
{
	if (!g_diligent_state.render_device || size == 0)
	{
		return {};
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_buffer";
	desc.Size = size;
	desc.BindFlags = bind_flags;
	desc.Usage = data ? Diligent::USAGE_IMMUTABLE : Diligent::USAGE_DEFAULT;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
	desc.ElementByteStride = stride;

	Diligent::BufferData buffer_data;
	if (data)
	{
		buffer_data.pData = data;
		buffer_data.DataSize = size;
	}

	Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
	g_diligent_state.render_device->CreateBuffer(desc, data ? &buffer_data : nullptr, &buffer);
	return buffer;
}

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_dynamic_vertex_buffer(uint32 size, uint32 stride)
{
	if (!g_diligent_state.render_device || size == 0)
	{
		return {};
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_dynamic_vertex_buffer";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = stride;

	Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &buffer);
	return buffer;
}
