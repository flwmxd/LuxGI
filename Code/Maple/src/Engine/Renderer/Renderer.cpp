//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Renderer.h"
#include "Application.h"
#include "Engine/Mesh.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"

namespace maple
{
	auto Renderer::bindDescriptorSets(Pipeline *pipeline, const CommandBuffer *cmdBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		RenderDevice::bindDescriptorSets(pipeline, cmdBuffer, dynamicOffset, descriptorSets);
	}

	auto Renderer::bindDescriptorSets(Pipeline *pipeline, const CommandBuffer *cmdBuffer, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		Application::getRenderDevice()->bindDescriptorSets(pipeline, cmdBuffer, descriptorSets);
	}

	auto Renderer::drawIndexed(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start) -> void
	{
		RenderDevice::drawIndexed(commandBuffer, type, count, start);
	}

	auto Renderer::drawArrays(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start /*= 0*/) -> void
	{
		RenderDevice::drawArrays(commandBuffer, type, count, start);
	}

	auto Renderer::dispatch(const CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void
	{
		Application::getRenderDevice()->dispatch(commandBuffer, x, y, z);
	}

	auto Renderer::dispatch(const CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z, Pipeline *pipeline, const void *pushConsts, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		for (auto &set : descriptorSets)
		{
			set->update(commandBuffer);
		}
		pipeline->bind(commandBuffer);
		if (pushConsts != nullptr)
		{
			pipeline->getShader()->getPushConstant(0)->setData(pushConsts);
			pipeline->getShader()->bindPushConstants(commandBuffer, pipeline);
		}
		Renderer::bindDescriptorSets(pipeline, commandBuffer, 0, descriptorSets);
		Application::getRenderDevice()->dispatch(commandBuffer, x, y, z);
		pipeline->end(commandBuffer);
	}

	auto Renderer::memoryBarrier(const CommandBuffer *commandBuffer, uint32_t flags) -> void
	{
		Application::getRenderDevice()->memoryBarrier(commandBuffer, flags);
	}

	auto Renderer::memoryBarrier(const CommandBuffer *commandBuffer, ShaderType fromStage, ShaderType toStage, AccessFlags from, AccessFlags to) -> void
	{
		Application::getRenderDevice()->memoryBarrier(commandBuffer, fromStage, toStage, from, to);
	}

	auto Renderer::imageBarrier(const CommandBuffer *commandBuffer, const ImageMemoryBarrier &barriers) -> void
	{
		Application::getRenderDevice()->imageBarrier(commandBuffer, barriers);
	}

	auto Renderer::drawMesh(const CommandBuffer *cmdBuffer, Pipeline *pipeline, Mesh *mesh) -> void
	{
		mesh->getVertexBuffer()->bind(cmdBuffer, pipeline);
		mesh->getIndexBuffer()->bind(cmdBuffer);
		RenderDevice::drawIndexed(cmdBuffer, DrawType::Triangle, mesh->getIndexBuffer()->getCount());
		mesh->getVertexBuffer()->unbind();
		mesh->getIndexBuffer()->unbind();
	}

};        // namespace maple
