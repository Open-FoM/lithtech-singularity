#include "platform_macos.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

void* CreateMetalView(SDL_Window* window)
{
	if (window == nullptr)
	{
		return nullptr;
	}

	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	if (props == 0)
	{
		return nullptr;
	}

	void* window_ptr = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
	if (window_ptr == nullptr)
	{
		return nullptr;
	}

	NSWindow* ns_window = (__bridge NSWindow*)window_ptr;
	NSView* view = [ns_window contentView];
	if (view == nil)
	{
		return nullptr;
	}

	view.wantsLayer = YES;
	if (![view.layer isKindOfClass:[CAMetalLayer class]])
	{
		view.layer = [CAMetalLayer layer];
	}

	CAMetalLayer* layer = (CAMetalLayer*)view.layer;
	if (layer.device == nil)
	{
		layer.device = MTLCreateSystemDefaultDevice();
	}

	return (__bridge void*)view;
}

void DestroyMetalView(void* view)
{
	(void)view;
}

void* GetMetalLayer(void* view)
{
	if (view == nullptr)
	{
		return nullptr;
	}

	NSView* ns_view = (__bridge NSView*)view;
	return (__bridge void*)ns_view.layer;
}

bool OpenFolderDialog(const std::string& initial_dir, std::string& out_path)
{
	@autoreleasepool
	{
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		panel.canChooseFiles = NO;
		panel.canChooseDirectories = YES;
		panel.allowsMultipleSelection = NO;
		panel.treatsFilePackagesAsDirectories = YES;
		if (!initial_dir.empty())
		{
			NSString* initialPath = [NSString stringWithUTF8String:initial_dir.c_str()];
			if (initialPath != nil)
			{
				panel.directoryURL = [NSURL fileURLWithPath:initialPath];
			}
		}

		if ([panel runModal] == NSModalResponseOK)
		{
			NSURL* url = [panel URL];
			if (url != nil)
			{
				out_path = std::string([[url path] UTF8String]);
				return true;
			}
		}
	}

	return false;
}
