#include "bdefs.h"

#include "diligent_screenshot.h"

#include "diligent_state.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"

#include <cstdint>
#include <vector>

// This translation unit owns the stb_image_write implementation.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool diligent_CaptureBackbufferToPng(const char *path, int *out_width, int *out_height) {
  using namespace Diligent;

  if (path == nullptr || !g_diligent_state.swap_chain || !g_diligent_state.render_device ||
      !g_diligent_state.immediate_context) {
    return false;
  }

  ITextureView *backbuffer_rtv = g_diligent_state.swap_chain->GetCurrentBackBufferRTV();
  if (backbuffer_rtv == nullptr) {
    return false;
  }
  ITexture *source = backbuffer_rtv->GetTexture();
  if (source == nullptr) {
    return false;
  }

  const TextureDesc &source_desc = source->GetDesc();
  const Uint32 width = source_desc.Width;
  const Uint32 height = source_desc.Height;
  if (width == 0 || height == 0) {
    return false;
  }

  // A CPU-readable staging copy of the backbuffer.
  TextureDesc staging_desc = source_desc;
  staging_desc.Name = "agent_screenshot_staging";
  staging_desc.Usage = USAGE_STAGING;
  staging_desc.CPUAccessFlags = CPU_ACCESS_READ;
  staging_desc.BindFlags = BIND_NONE;
  staging_desc.MiscFlags = MISC_TEXTURE_FLAG_NONE;
  staging_desc.MipLevels = 1;

  RefCntAutoPtr<ITexture> staging;
  g_diligent_state.render_device->CreateTexture(staging_desc, nullptr, &staging);
  if (!staging) {
    return false;
  }

  CopyTextureAttribs copy;
  copy.pSrcTexture = source;
  copy.pDstTexture = staging;
  copy.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  copy.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  g_diligent_state.immediate_context->CopyTexture(copy);

  // Ensure the copy has completed before mapping for read.
  g_diligent_state.immediate_context->WaitForIdle();

  MappedTextureSubresource mapped{};
  g_diligent_state.immediate_context->MapTextureSubresource(staging, 0, 0, MAP_READ, MAP_FLAG_NONE, nullptr, mapped);
  if (mapped.pData == nullptr) {
    return false;
  }

  // The backbuffer is commonly BGRA on Vulkan/Metal; PNG wants RGBA, so swizzle
  // when needed and pack tightly (the mapped rows may be padded via Stride).
  const bool is_bgra =
      source_desc.Format == TEX_FORMAT_BGRA8_UNORM || source_desc.Format == TEX_FORMAT_BGRA8_UNORM_SRGB;

  std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * height * 4);
  const auto *src_bytes = static_cast<const std::uint8_t *>(mapped.pData);
  for (Uint32 y = 0; y < height; ++y) {
    const std::uint8_t *row = src_bytes + static_cast<std::size_t>(y) * mapped.Stride;
    std::uint8_t *dst = rgba.data() + static_cast<std::size_t>(y) * width * 4;
    for (Uint32 x = 0; x < width; ++x) {
      const std::uint8_t *px = row + static_cast<std::size_t>(x) * 4;
      if (is_bgra) {
        dst[0] = px[2];
        dst[1] = px[1];
        dst[2] = px[0];
        dst[3] = px[3];
      } else {
        dst[0] = px[0];
        dst[1] = px[1];
        dst[2] = px[2];
        dst[3] = px[3];
      }
      dst += 4;
    }
  }

  g_diligent_state.immediate_context->UnmapTextureSubresource(staging, 0, 0);

  const int written = stbi_write_png(path, static_cast<int>(width), static_cast<int>(height), 4, rgba.data(),
                                     static_cast<int>(width) * 4);

  if (out_width != nullptr) {
    *out_width = static_cast<int>(width);
  }
  if (out_height != nullptr) {
    *out_height = static_cast<int>(height);
  }
  return written != 0;
}
