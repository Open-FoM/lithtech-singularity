#ifndef LTJS_DILIGENT_BUFFERS_H
#define LTJS_DILIGENT_BUFFERS_H

#include "ltbasedefs.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_buffer(
	uint32 size,
	Diligent::BIND_FLAGS bind_flags,
	const void* data,
	uint32 stride);

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_dynamic_vertex_buffer(uint32 size, uint32 stride);

#endif
