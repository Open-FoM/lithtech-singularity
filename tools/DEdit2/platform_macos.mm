#include "platform_macos.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>
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

	SDL_MetalView metal_view = SDL_Metal_CreateView(window);
	if (metal_view == nullptr)
	{
		return nullptr;
	}

	void* layer_ptr = SDL_Metal_GetLayer(metal_view);
	if (layer_ptr != nullptr)
	{
		CAMetalLayer* layer = (__bridge CAMetalLayer*)layer_ptr;
		if (layer.device == nil)
		{
			layer.device = MTLCreateSystemDefaultDevice();
		}
	}

	return metal_view;
}

void DestroyMetalView(void* view)
{
	if (view != nullptr)
	{
		SDL_Metal_DestroyView(static_cast<SDL_MetalView>(view));
	}
}

void* GetMetalLayer(void* view)
{
	if (view == nullptr)
	{
		return nullptr;
	}

	return SDL_Metal_GetLayer(static_cast<SDL_MetalView>(view));
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
