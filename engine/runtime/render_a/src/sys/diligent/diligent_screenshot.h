/**
 * diligent_screenshot.h
 *
 * Backbuffer capture for the Diligent renderer. Reads the current swap-chain
 * backbuffer back to the CPU and writes it as a PNG. Used by the legacy
 * RenderStruct::MakeScreenShot callback and by the agent control surface's
 * capture_screenshot tool.
 *
 * Must be called on the render/main thread (it touches the immediate context).
 */
#ifndef LTJS_DILIGENT_SCREENSHOT_H
#define LTJS_DILIGENT_SCREENSHOT_H

/// Capture the current backbuffer to `path` as a PNG. Returns true on success.
/// `out_width`/`out_height` (optional) receive the captured image dimensions.
bool diligent_CaptureBackbufferToPng(const char *path, int *out_width, int *out_height);

#endif // LTJS_DILIGENT_SCREENSHOT_H
