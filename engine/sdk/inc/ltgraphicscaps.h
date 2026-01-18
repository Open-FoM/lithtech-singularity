#ifndef _LT_GRAPHICS_CAPS_H_
#define _LT_GRAPHICS_CAPS_H_

#include "ltbasetypes.h"

static const uint32 LT_MAX_ADAPTER_QUEUES = 16;
static const uint32 LT_MAX_SHADING_RATES = 9;

// Feature state mirrors Diligent::DEVICE_FEATURE_STATE.
enum LTDeviceFeatureState : uint8
{
	LT_DEVICE_FEATURE_DISABLED = 0,
	LT_DEVICE_FEATURE_ENABLED = 1,
	LT_DEVICE_FEATURE_OPTIONAL = 2
};

// Major/minor version pair (e.g., API or shader language version).
struct LTGraphicsVersion
{
	uint32 Major = 0;
	uint32 Minor = 0;
};

// Maximum supported shader versions for each language.
struct LTGraphicsShaderVersionInfo
{
	LTGraphicsVersion HLSL{};
	LTGraphicsVersion GLSL{};
	LTGraphicsVersion GLESSL{};
	LTGraphicsVersion MSL{};
};

// Normalized device coordinate attributes.
struct LTGraphicsNDCAttribs
{
	float MinZ = 0.0f;
	float ZtoDepthScale = 0.0f;
	float YtoVScale = 0.0f;
};

// Device feature set (enabled/supported state per feature).
struct LTDeviceFeatures
{
	LTDeviceFeatureState SeparablePrograms = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ShaderResourceQueries = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState WireframeFill = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState MultithreadedResourceCreation = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ComputeShaders = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState GeometryShaders = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState Tessellation = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState MeshShaders = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState RayTracing = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState BindlessResources = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState OcclusionQueries = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState BinaryOcclusionQueries = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TimestampQueries = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState PipelineStatisticsQueries = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState DurationQueries = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState DepthBiasClamp = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState DepthClamp = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState IndependentBlend = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState DualSourceBlend = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState MultiViewport = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TextureCompressionBC = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TextureCompressionETC2 = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState VertexPipelineUAVWritesAndAtomics = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState PixelUAVWritesAndAtomics = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TextureUAVExtendedFormats = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ShaderFloat16 = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ResourceBuffer16BitAccess = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState UniformBuffer16BitAccess = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ShaderInputOutput16 = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ShaderInt8 = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ResourceBuffer8BitAccess = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState UniformBuffer8BitAccess = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ShaderResourceStaticArrays = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState ShaderResourceRuntimeArrays = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState WaveOp = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState InstanceDataStepRate = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState NativeFence = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TileShaders = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TransferQueueTimestampQueries = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState VariableRateShading = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState SparseResources = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState SubpassFramebufferFetch = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TextureComponentSwizzle = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState TextureSubresourceViews = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState NativeMultiDraw = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState AsyncShaderCompilation = LT_DEVICE_FEATURE_DISABLED;
	LTDeviceFeatureState FormattedBuffers = LT_DEVICE_FEATURE_DISABLED;
};

// GPU memory characteristics (bytes unless otherwise noted).
struct LTAdapterMemoryInfo
{
	uint64 LocalMemory = 0;
	uint64 HostVisibleMemory = 0;
	uint64 UnifiedMemory = 0;
	uint64 MaxMemoryAllocation = 0;
	// Bitmask of CPU access flags.
	uint32 UnifiedMemoryCPUAccess = 0;
	// Bitmask of bind flags supported by memoryless textures.
	uint32 MemorylessTextureBindFlags = 0;
};

// Texture limits and support flags.
struct LTTextureProperties
{
	uint32 MaxTexture1DDimension = 0;
	uint32 MaxTexture1DArraySlices = 0;
	uint32 MaxTexture2DDimension = 0;
	uint32 MaxTexture2DArraySlices = 0;
	uint32 MaxTexture3DDimension = 0;
	uint32 MaxTextureCubeDimension = 0;
	uint8 Texture2DMSSupported = 0;
	uint8 Texture2DMSArraySupported = 0;
	uint8 TextureViewSupported = 0;
	uint8 CubemapArraysSupported = 0;
	uint8 TextureView2DOn3DSupported = 0;
};

// Texture sampler limits and support flags.
struct LTSamplerProperties
{
	uint8 BorderSamplingModeSupported = 0;
	uint8 MaxAnisotropy = 1;
	uint8 LODBiasSupported = 0;
};

// Wave/subgroup properties.
struct LTWaveOpProperties
{
	uint32 MinSize = 0;
	uint32 MaxSize = 0;
	// Bitmask of supported shader stages.
	uint32 SupportedStages = 0;
	// Bitmask of supported wave operations.
	uint32 Features = 0;
};

// Buffer alignment properties.
struct LTBufferProperties
{
	uint32 ConstantBufferOffsetAlignment = 0;
	uint32 StructuredBufferOffsetAlignment = 0;
};

// Ray tracing limits and capability flags.
struct LTRayTracingProperties
{
	uint32 MaxRecursionDepth = 0;
	uint32 ShaderGroupHandleSize = 0;
	uint32 MaxShaderRecordStride = 0;
	uint32 ShaderGroupBaseAlignment = 0;
	uint32 MaxRayGenThreads = 0;
	uint32 MaxInstancesPerTLAS = 0;
	uint32 MaxPrimitivesPerBLAS = 0;
	uint32 MaxGeometriesPerBLAS = 0;
	uint32 VertexBufferAlignment = 0;
	uint32 IndexBufferAlignment = 0;
	uint32 TransformBufferAlignment = 0;
	uint32 BoxBufferAlignment = 0;
	uint32 ScratchBufferAlignment = 0;
	uint32 InstanceBufferAlignment = 0;
	uint32 CapFlags = 0;
};

// Mesh shader limits.
struct LTMeshShaderProperties
{
	uint32 MaxThreadGroupCountX = 0;
	uint32 MaxThreadGroupCountY = 0;
	uint32 MaxThreadGroupCountZ = 0;
	uint32 MaxThreadGroupTotalCount = 0;
};

// Compute shader limits.
struct LTComputeShaderProperties
{
	uint32 SharedMemorySize = 0;
	uint32 MaxThreadGroupInvocations = 0;
	uint32 MaxThreadGroupSizeX = 0;
	uint32 MaxThreadGroupSizeY = 0;
	uint32 MaxThreadGroupSizeZ = 0;
	uint32 MaxThreadGroupCountX = 0;
	uint32 MaxThreadGroupCountY = 0;
	uint32 MaxThreadGroupCountZ = 0;
};

// Shading rate mode entry (rate + supported sample counts bitmask).
struct LTShadingRateMode
{
	uint32 Rate = 0;
	uint32 SampleBits = 0;
};

// Variable rate shading properties.
struct LTShadingRateProperties
{
	LTShadingRateMode ShadingRates[LT_MAX_SHADING_RATES]{};
	uint8 NumShadingRates = 0;
	uint32 CapFlags = 0;
	uint32 Combiners = 0;
	uint32 Format = 0;
	uint32 ShadingRateTextureAccess = 0;
	uint32 BindFlags = 0;
	uint32 MinTileSize[2]{};
	uint32 MaxTileSize[2]{};
	uint32 MaxSabsampledArraySlices = 0;
};

// Draw command capability flags and limits.
struct LTDrawCommandProperties
{
	uint32 CapFlags = 0;
	uint32 MaxIndexValue = 0;
	uint32 MaxDrawIndirectCount = 0;
};

// Sparse resource support and limits.
struct LTSparseResourceProperties
{
	uint64 AddressSpaceSize = 0;
	uint64 ResourceSpaceSize = 0;
	uint32 CapFlags = 0;
	uint32 StandardBlockSize = 0;
	uint32 BufferBindFlags = 0;
};

// Command queue properties.
struct LTCommandQueueInfo
{
	// Bitmask of supported queue types.
	uint32 QueueType = 0;
	uint32 MaxDeviceContexts = 0;
	uint32 TextureCopyGranularity[3]{};
};

// Graphics adapter information.
struct LTGraphicsAdapterInfo
{
	// Human-readable adapter description.
	char Description[128]{};
	// Adapter type and vendor enums (mirror Diligent ADAPTER_TYPE/ADAPTER_VENDOR values).
	uint32 Type = 0;
	uint32 Vendor = 0;
	uint32 VendorId = 0;
	uint32 DeviceId = 0;
	uint32 NumOutputs = 0;
	LTAdapterMemoryInfo Memory{};
	LTRayTracingProperties RayTracing{};
	LTWaveOpProperties WaveOp{};
	LTBufferProperties Buffer{};
	LTTextureProperties Texture{};
	LTSamplerProperties Sampler{};
	LTMeshShaderProperties MeshShader{};
	LTShadingRateProperties ShadingRate{};
	LTComputeShaderProperties ComputeShader{};
	LTDrawCommandProperties DrawCommand{};
	LTSparseResourceProperties SparseResources{};
	LTDeviceFeatures Features{};
	LTCommandQueueInfo Queues[LT_MAX_ADAPTER_QUEUES]{};
	uint32 NumQueues = 0;
};

// Render device information.
struct LTGraphicsDeviceInfo
{
	// Backend type enum (mirrors Diligent RENDER_DEVICE_TYPE values).
	uint32 Type = 0;
	LTGraphicsVersion APIVersion{};
	LTDeviceFeatures Features{};
	LTGraphicsNDCAttribs NDC{};
	LTGraphicsShaderVersionInfo MaxShaderVersion{};
};

class LTGraphicsCaps
{
public:

	LTGraphicsCaps():
	  Size(sizeof(LTGraphicsCaps)),
	  Version(1),
	  VertexShaderVersion(0),
	  PixelShaderVersion(0)
	  {
	  }

	// Structure size in bytes for versioning.
	uint32 Size;
	// Structure version; increments when layout changes.
	uint32 Version;
	// Legacy fields: packed as (Major << 16) | Minor for the active backend.
	uint32 VertexShaderVersion;
	uint32 PixelShaderVersion;
	LTGraphicsDeviceInfo RenderDevice{};
	LTGraphicsAdapterInfo Adapter{};
};

#endif
