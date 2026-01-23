/**
 * diligent_buffers.h
 *
 * This header defines the Buffers portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_BUFFERS_H
#define LTJS_DILIGENT_BUFFERS_H

#include "ltbasedefs.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"

/// Creates a GPU buffer with the specified bind flags and optional initial data.
Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_buffer(
	uint32 size,
	Diligent::BIND_FLAGS bind_flags,
	const void* data,
	uint32 stride);

/// Creates a dynamic vertex buffer suitable for frequent updates.
Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_dynamic_vertex_buffer(uint32 size, uint32 stride);

#endif
