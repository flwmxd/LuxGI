//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/AccessFlags.h"
#include "RHI/Definitions.h"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
namespace maple
{
	class Texture;

	class MAPLE_EXPORT Renderer
	{
	  public:
		static auto bindDescriptorSets(Pipeline *pipeline, const CommandBuffer *cmdBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void;
		static auto bindDescriptorSets(Pipeline *pipeline, const CommandBuffer *cmdBuffer, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void;
		static auto drawIndexed(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) -> void;
		static auto drawArrays(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) -> void;
		static auto dispatch(const CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void;
		static auto dispatch(const CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z, Pipeline *pipeline, const void *pushConsts, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void;
		static auto memoryBarrier(const CommandBuffer *commandBuffer, uint32_t flags) -> void;
		static auto memoryBarrier(const CommandBuffer *commandBuffer, ShaderType fromStage, ShaderType toStage, AccessFlags from, AccessFlags to) -> void;
		static auto imageBarrier(const CommandBuffer *cmd, const ImageMemoryBarrier &barriers) -> void;
		static auto drawMesh(const CommandBuffer *cmdBuffer, Pipeline *pipeline, Mesh *mesh) -> void;
	};
};        // namespace maple
