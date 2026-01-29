#include "platform_macos.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

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

bool OpenFileDialog(
    const std::string& initial_dir,
    const std::vector<std::string>& filter_extensions,
    const std::string& filter_description,
    std::string& out_path)
{
	@autoreleasepool
	{
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		panel.canChooseFiles = YES;
		panel.canChooseDirectories = NO;
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

		// Set allowed file types
		if (!filter_extensions.empty())
		{
			NSMutableArray<UTType*>* types = [NSMutableArray array];
			for (const auto& ext : filter_extensions)
			{
				NSString* extStr = [NSString stringWithUTF8String:ext.c_str()];
				UTType* type = [UTType typeWithFilenameExtension:extStr];
				if (type != nil)
				{
					[types addObject:type];
				}
			}
			if (types.count > 0)
			{
				panel.allowedContentTypes = types;
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

bool SaveFileDialog(
    const std::string& initial_dir,
    const std::string& default_name,
    const std::string& filter_extension,
    const std::string& filter_description,
    std::string& out_path)
{
	@autoreleasepool
	{
		NSSavePanel* panel = [NSSavePanel savePanel];
		panel.canCreateDirectories = YES;
		panel.treatsFilePackagesAsDirectories = YES;

		if (!initial_dir.empty())
		{
			NSString* initialPath = [NSString stringWithUTF8String:initial_dir.c_str()];
			if (initialPath != nil)
			{
				panel.directoryURL = [NSURL fileURLWithPath:initialPath];
			}
		}

		if (!default_name.empty())
		{
			NSString* name = [NSString stringWithUTF8String:default_name.c_str()];
			panel.nameFieldStringValue = name;
		}

		// Set allowed file type
		if (!filter_extension.empty())
		{
			NSString* extStr = [NSString stringWithUTF8String:filter_extension.c_str()];
			UTType* type = [UTType typeWithFilenameExtension:extStr];
			if (type != nil)
			{
				panel.allowedContentTypes = @[type];
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

void OpenPathInFileManager(const std::string& path)
{
	@autoreleasepool
	{
		NSString* pathStr = [NSString stringWithUTF8String:path.c_str()];
		if (pathStr != nil)
		{
			NSURL* url = [NSURL fileURLWithPath:pathStr];
			[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[url]];
		}
	}
}
