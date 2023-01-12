//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Vk.h"

#include <assert.h>
#include <vulkan/vulkan.h>
#ifdef USE_VMA_ALLOCATOR
#	define VMA_IMPLEMENTATION
#	define VMA_STATIC_VULKAN_FUNCTIONS 0
#	include <vk_mem_alloc.h>
#endif

static PFN_vkCreateAccelerationStructureKHR                 pfn_vkCreateAccelerationStructureKHR                 = 0;
static PFN_vkDestroyAccelerationStructureKHR                pfn_vkDestroyAccelerationStructureKHR                = 0;
static PFN_vkCmdBuildAccelerationStructuresKHR              pfn_vkCmdBuildAccelerationStructuresKHR              = 0;
static PFN_vkCmdBuildAccelerationStructuresIndirectKHR      pfn_vkCmdBuildAccelerationStructuresIndirectKHR      = 0;
static PFN_vkBuildAccelerationStructuresKHR                 pfn_vkBuildAccelerationStructuresKHR                 = 0;
static PFN_vkCopyAccelerationStructureKHR                   pfn_vkCopyAccelerationStructureKHR                   = 0;
static PFN_vkCopyAccelerationStructureToMemoryKHR           pfn_vkCopyAccelerationStructureToMemoryKHR           = 0;
static PFN_vkCopyMemoryToAccelerationStructureKHR           pfn_vkCopyMemoryToAccelerationStructureKHR           = 0;
static PFN_vkWriteAccelerationStructuresPropertiesKHR       pfn_vkWriteAccelerationStructuresPropertiesKHR       = 0;
static PFN_vkCmdCopyAccelerationStructureKHR                pfn_vkCmdCopyAccelerationStructureKHR                = 0;
static PFN_vkCmdCopyAccelerationStructureToMemoryKHR        pfn_vkCmdCopyAccelerationStructureToMemoryKHR        = 0;
static PFN_vkCmdCopyMemoryToAccelerationStructureKHR        pfn_vkCmdCopyMemoryToAccelerationStructureKHR        = 0;
static PFN_vkGetAccelerationStructureDeviceAddressKHR       pfn_vkGetAccelerationStructureDeviceAddressKHR       = 0;
static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR    pfn_vkCmdWriteAccelerationStructuresPropertiesKHR    = 0;
static PFN_vkGetDeviceAccelerationStructureCompatibilityKHR pfn_vkGetDeviceAccelerationStructureCompatibilityKHR = 0;
static PFN_vkGetAccelerationStructureBuildSizesKHR          pfn_vkGetAccelerationStructureBuildSizesKHR          = 0;
static PFN_vkSetDebugUtilsObjectNameEXT                     pfn_vkSetDebugUtilsObjectNameEXT                     = 0;

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
    VkDevice                             device,
    const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
{
	assert(pfn_vkSetDebugUtilsObjectNameEXT);
	return pfn_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
    VkDevice                                    device,
    const VkAccelerationStructureCreateInfoKHR *pCreateInfo,
    const VkAllocationCallbacks *               pAllocator,
    VkAccelerationStructureKHR *                pAccelerationStructure)
{
	assert(pfn_vkCreateAccelerationStructureKHR);
	return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}
VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
    VkDevice                     device,
    VkAccelerationStructureKHR   accelerationStructure,
    const VkAllocationCallbacks *pAllocator)
{
	assert(pfn_vkDestroyAccelerationStructureKHR);
	pfn_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
}
VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR(
    VkCommandBuffer                                        commandBuffer,
    uint32_t                                               infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR *    pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos)
{
	assert(pfn_vkCmdBuildAccelerationStructuresKHR);
	pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}
VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresIndirectKHR(
    VkCommandBuffer                                    commandBuffer,
    uint32_t                                           infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
    const VkDeviceAddress *                            pIndirectDeviceAddresses,
    const uint32_t *                                   pIndirectStrides,
    const uint32_t *const *                            ppMaxPrimitiveCounts)
{
	assert(pfn_vkCmdBuildAccelerationStructuresIndirectKHR);
	pfn_vkCmdBuildAccelerationStructuresIndirectKHR(commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts);
}
VKAPI_ATTR VkResult VKAPI_CALL vkBuildAccelerationStructuresKHR(
    VkDevice                                               device,
    VkDeferredOperationKHR                                 deferredOperation,
    uint32_t                                               infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR *    pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos)
{
	assert(pfn_vkBuildAccelerationStructuresKHR);
	return pfn_vkBuildAccelerationStructuresKHR(device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureKHR(
    VkDevice                                  device,
    VkDeferredOperationKHR                    deferredOperation,
    const VkCopyAccelerationStructureInfoKHR *pInfo)
{
	assert(pfn_vkCopyAccelerationStructureKHR);
	return pfn_vkCopyAccelerationStructureKHR(device, deferredOperation, pInfo);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureToMemoryKHR(
    VkDevice                                          device,
    VkDeferredOperationKHR                            deferredOperation,
    const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
	assert(pfn_vkCopyAccelerationStructureToMemoryKHR);
	return pfn_vkCopyAccelerationStructureToMemoryKHR(device, deferredOperation, pInfo);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToAccelerationStructureKHR(
    VkDevice                                          device,
    VkDeferredOperationKHR                            deferredOperation,
    const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
	assert(pfn_vkCopyMemoryToAccelerationStructureKHR);
	return pfn_vkCopyMemoryToAccelerationStructureKHR(device, deferredOperation, pInfo);
}
VKAPI_ATTR VkResult VKAPI_CALL vkWriteAccelerationStructuresPropertiesKHR(
    VkDevice                          device,
    uint32_t                          accelerationStructureCount,
    const VkAccelerationStructureKHR *pAccelerationStructures,
    VkQueryType                       queryType,
    size_t                            dataSize,
    void *                            pData,
    size_t                            stride)
{
	assert(pfn_vkWriteAccelerationStructuresPropertiesKHR);
	return pfn_vkWriteAccelerationStructuresPropertiesKHR(device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride);
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureKHR(
    VkCommandBuffer                           commandBuffer,
    const VkCopyAccelerationStructureInfoKHR *pInfo)
{
	assert(pfn_vkCmdCopyAccelerationStructureKHR);
	pfn_vkCmdCopyAccelerationStructureKHR(commandBuffer, pInfo);
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureToMemoryKHR(
    VkCommandBuffer                                   commandBuffer,
    const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
	assert(pfn_vkCmdCopyAccelerationStructureToMemoryKHR);
	pfn_vkCmdCopyAccelerationStructureToMemoryKHR(commandBuffer, pInfo);
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToAccelerationStructureKHR(
    VkCommandBuffer                                   commandBuffer,
    const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
	assert(pfn_vkCmdCopyMemoryToAccelerationStructureKHR);
	pfn_vkCmdCopyMemoryToAccelerationStructureKHR(commandBuffer, pInfo);
}
VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(
    VkDevice                                           device,
    const VkAccelerationStructureDeviceAddressInfoKHR *pInfo)
{
	assert(pfn_vkGetAccelerationStructureDeviceAddressKHR);
	return pfn_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesKHR(
    VkCommandBuffer                   commandBuffer,
    uint32_t                          accelerationStructureCount,
    const VkAccelerationStructureKHR *pAccelerationStructures,
    VkQueryType                       queryType,
    VkQueryPool                       queryPool,
    uint32_t                          firstQuery)
{
	assert(pfn_vkCmdWriteAccelerationStructuresPropertiesKHR);
	pfn_vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceAccelerationStructureCompatibilityKHR(
    VkDevice                                     device,
    const VkAccelerationStructureVersionInfoKHR *pVersionInfo,
    VkAccelerationStructureCompatibilityKHR *    pCompatibility)
{
	assert(pfn_vkGetDeviceAccelerationStructureCompatibilityKHR);
	pfn_vkGetDeviceAccelerationStructureCompatibilityKHR(device, pVersionInfo, pCompatibility);
}
VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(
    VkDevice                                           device,
    VkAccelerationStructureBuildTypeKHR                buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo,
    const uint32_t *                                   pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR *         pSizeInfo)
{
	assert(pfn_vkGetAccelerationStructureBuildSizesKHR);
	pfn_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

static PFN_vkCmdTraceRaysKHR                                 pfn_vkCmdTraceRaysKHR                                 = 0;
static PFN_vkCreateRayTracingPipelinesKHR                    pfn_vkCreateRayTracingPipelinesKHR                    = 0;
static PFN_vkGetRayTracingShaderGroupHandlesKHR              pfn_vkGetRayTracingShaderGroupHandlesKHR              = 0;
static PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = 0;
static PFN_vkCmdTraceRaysIndirectKHR                         pfn_vkCmdTraceRaysIndirectKHR                         = 0;
static PFN_vkGetRayTracingShaderGroupStackSizeKHR            pfn_vkGetRayTracingShaderGroupStackSizeKHR            = 0;
static PFN_vkCmdSetRayTracingPipelineStackSizeKHR            pfn_vkCmdSetRayTracingPipelineStackSizeKHR            = 0;

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(
    VkCommandBuffer                        commandBuffer,
    const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable,
    uint32_t                               width,
    uint32_t                               height,
    uint32_t                               depth)
{
	assert(pfn_vkCmdTraceRaysKHR);
	pfn_vkCmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
    VkDevice                                 device,
    VkDeferredOperationKHR                   deferredOperation,
    VkPipelineCache                          pipelineCache,
    uint32_t                                 createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
    const VkAllocationCallbacks *            pAllocator,
    VkPipeline *                             pPipelines)
{
	assert(pfn_vkCreateRayTracingPipelinesKHR);
	return pfn_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(
    VkDevice   device,
    VkPipeline pipeline,
    uint32_t   firstGroup,
    uint32_t   groupCount,
    size_t     dataSize,
    void *     pData)
{
	assert(pfn_vkGetRayTracingShaderGroupHandlesKHR);
	return pfn_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(
    VkDevice   device,
    VkPipeline pipeline,
    uint32_t   firstGroup,
    uint32_t   groupCount,
    size_t     dataSize,
    void *     pData)
{
	assert(pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR);
	return pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}
VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysIndirectKHR(
    VkCommandBuffer                        commandBuffer,
    const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable,
    VkDeviceAddress                        indirectDeviceAddress)
{
	assert(pfn_vkCmdTraceRaysIndirectKHR);
	pfn_vkCmdTraceRaysIndirectKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, indirectDeviceAddress);
}
VKAPI_ATTR VkDeviceSize VKAPI_CALL vkGetRayTracingShaderGroupStackSizeKHR(
    VkDevice               device,
    VkPipeline             pipeline,
    uint32_t               group,
    VkShaderGroupShaderKHR groupShader)
{
	assert(pfn_vkGetRayTracingShaderGroupStackSizeKHR);
	return pfn_vkGetRayTracingShaderGroupStackSizeKHR(device, pipeline, group, groupShader);
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetRayTracingPipelineStackSizeKHR(
    VkCommandBuffer commandBuffer,
    uint32_t        pipelineStackSize)
{
	assert(pfn_vkCmdSetRayTracingPipelineStackSizeKHR);
	pfn_vkCmdSetRayTracingPipelineStackSizeKHR(commandBuffer, pipelineStackSize);
}

auto maple::loadVKRayTracingPipelineKHR(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr) -> int32_t
{
	pfn_vkCmdTraceRaysKHR                                 = (PFN_vkCmdTraceRaysKHR) getDeviceProcAddr(device, "vkCmdTraceRaysKHR");
	pfn_vkCreateRayTracingPipelinesKHR                    = (PFN_vkCreateRayTracingPipelinesKHR) getDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
	pfn_vkGetRayTracingShaderGroupHandlesKHR              = (PFN_vkGetRayTracingShaderGroupHandlesKHR) getDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
	pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR) getDeviceProcAddr(device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
	pfn_vkCmdTraceRaysIndirectKHR                         = (PFN_vkCmdTraceRaysIndirectKHR) getDeviceProcAddr(device, "vkCmdTraceRaysIndirectKHR");
	pfn_vkGetRayTracingShaderGroupStackSizeKHR            = (PFN_vkGetRayTracingShaderGroupStackSizeKHR) getDeviceProcAddr(device, "vkGetRayTracingShaderGroupStackSizeKHR");
	pfn_vkCmdSetRayTracingPipelineStackSizeKHR            = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR) getDeviceProcAddr(device, "vkCmdSetRayTracingPipelineStackSizeKHR");
	int success                                           = 1;
	success                                               = success && (pfn_vkCmdTraceRaysKHR != 0);
	success                                               = success && (pfn_vkCreateRayTracingPipelinesKHR != 0);
	success                                               = success && (pfn_vkGetRayTracingShaderGroupHandlesKHR != 0);
	success                                               = success && (pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR != 0);
	success                                               = success && (pfn_vkCmdTraceRaysIndirectKHR != 0);
	success                                               = success && (pfn_vkGetRayTracingShaderGroupStackSizeKHR != 0);
	success                                               = success && (pfn_vkCmdSetRayTracingPipelineStackSizeKHR != 0);
	return success;
}

auto maple::loadVKAccelerationStructureKHR(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr) -> int32_t
{
	pfn_vkCreateAccelerationStructureKHR                 = (PFN_vkCreateAccelerationStructureKHR) getDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
	pfn_vkDestroyAccelerationStructureKHR                = (PFN_vkDestroyAccelerationStructureKHR) getDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
	pfn_vkCmdBuildAccelerationStructuresKHR              = (PFN_vkCmdBuildAccelerationStructuresKHR) getDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
	pfn_vkCmdBuildAccelerationStructuresIndirectKHR      = (PFN_vkCmdBuildAccelerationStructuresIndirectKHR) getDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresIndirectKHR");
	pfn_vkBuildAccelerationStructuresKHR                 = (PFN_vkBuildAccelerationStructuresKHR) getDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR");
	pfn_vkCopyAccelerationStructureKHR                   = (PFN_vkCopyAccelerationStructureKHR) getDeviceProcAddr(device, "vkCopyAccelerationStructureKHR");
	pfn_vkCopyAccelerationStructureToMemoryKHR           = (PFN_vkCopyAccelerationStructureToMemoryKHR) getDeviceProcAddr(device, "vkCopyAccelerationStructureToMemoryKHR");
	pfn_vkCopyMemoryToAccelerationStructureKHR           = (PFN_vkCopyMemoryToAccelerationStructureKHR) getDeviceProcAddr(device, "vkCopyMemoryToAccelerationStructureKHR");
	pfn_vkWriteAccelerationStructuresPropertiesKHR       = (PFN_vkWriteAccelerationStructuresPropertiesKHR) getDeviceProcAddr(device, "vkWriteAccelerationStructuresPropertiesKHR");
	pfn_vkCmdCopyAccelerationStructureKHR                = (PFN_vkCmdCopyAccelerationStructureKHR) getDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR");
	pfn_vkCmdCopyAccelerationStructureToMemoryKHR        = (PFN_vkCmdCopyAccelerationStructureToMemoryKHR) getDeviceProcAddr(device, "vkCmdCopyAccelerationStructureToMemoryKHR");
	pfn_vkCmdCopyMemoryToAccelerationStructureKHR        = (PFN_vkCmdCopyMemoryToAccelerationStructureKHR) getDeviceProcAddr(device, "vkCmdCopyMemoryToAccelerationStructureKHR");
	pfn_vkGetAccelerationStructureDeviceAddressKHR       = (PFN_vkGetAccelerationStructureDeviceAddressKHR) getDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
	pfn_vkCmdWriteAccelerationStructuresPropertiesKHR    = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR) getDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
	pfn_vkGetDeviceAccelerationStructureCompatibilityKHR = (PFN_vkGetDeviceAccelerationStructureCompatibilityKHR) getDeviceProcAddr(device, "vkGetDeviceAccelerationStructureCompatibilityKHR");
	pfn_vkGetAccelerationStructureBuildSizesKHR          = (PFN_vkGetAccelerationStructureBuildSizesKHR) getDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
	pfn_vkSetDebugUtilsObjectNameEXT                     = (PFN_vkSetDebugUtilsObjectNameEXT) getDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");

    int32_t success                                      = 1;
	success                                              = success && (pfn_vkCreateAccelerationStructureKHR != 0);
	success                                              = success && (pfn_vkDestroyAccelerationStructureKHR != 0);
	success                                              = success && (pfn_vkCmdBuildAccelerationStructuresKHR != 0);
	success                                              = success && (pfn_vkCmdBuildAccelerationStructuresIndirectKHR != 0);
	success                                              = success && (pfn_vkBuildAccelerationStructuresKHR != 0);
	success                                              = success && (pfn_vkCopyAccelerationStructureKHR != 0);
	success                                              = success && (pfn_vkCopyAccelerationStructureToMemoryKHR != 0);
	success                                              = success && (pfn_vkCopyMemoryToAccelerationStructureKHR != 0);
	success                                              = success && (pfn_vkWriteAccelerationStructuresPropertiesKHR != 0);
	success                                              = success && (pfn_vkCmdCopyAccelerationStructureKHR != 0);
	success                                              = success && (pfn_vkCmdCopyAccelerationStructureToMemoryKHR != 0);
	success                                              = success && (pfn_vkCmdCopyMemoryToAccelerationStructureKHR != 0);
	success                                              = success && (pfn_vkGetAccelerationStructureDeviceAddressKHR != 0);
	success                                              = success && (pfn_vkCmdWriteAccelerationStructuresPropertiesKHR != 0);
	success                                              = success && (pfn_vkGetDeviceAccelerationStructureCompatibilityKHR != 0);
	success                                              = success && (pfn_vkGetAccelerationStructureBuildSizesKHR != 0);
	return success;
}
