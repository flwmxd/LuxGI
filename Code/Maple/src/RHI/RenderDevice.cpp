//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderDevice.h"
#include "Application.h"
#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/VulkanRenderDevice.h"
#endif

#ifdef MAPLE_OPENGL
#	include "RHI/OpenGL/GLRenderDevice.h"
#endif

#include "RHI/FrameBuffer.h"
#include "RHI/RenderPass.h"

namespace maple
{
	auto RenderDevice::clear(uint32_t bufferMask) -> void
	{
		Application::getRenderDevice()->clearInternal(bufferMask);
	}

	auto RenderDevice::present() -> void
	{
		Application::getRenderDevice()->presentInternal();
	}

	auto RenderDevice::present(const CommandBuffer *commandBuffer) -> void
	{
		Application::getRenderDevice()->presentInternal(commandBuffer);
	}

	auto RenderDevice::bindDescriptorSets(Pipeline *pipeline, const CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		Application::getRenderDevice()->bindDescriptorSetsInternal(pipeline, commandBuffer, dynamicOffset, descriptorSets);
	}

	auto RenderDevice::draw(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType datayType, const void *indices) -> void
	{
		Application::getRenderDevice()->drawInternal(commandBuffer, type, count, datayType, indices);
	}

	auto RenderDevice::drawIndexed(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start) -> void
	{
		Application::getRenderDevice()->drawIndexedInternal(commandBuffer, type, count, start);
	}

	auto RenderDevice::drawArrays(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start /*= 0*/) -> void
	{
		Application::getRenderDevice()->drawArraysInternal(commandBuffer, type, count, start);
	}

	auto RenderDevice::setStencilOp(StencilType fail, StencilType zfail, StencilType zpass) -> void
	{
		Application::getRenderDevice()->setStencilOpInternal(fail, zfail, zpass);
	}

	auto RenderDevice::setStencilFunction(StencilType type, uint32_t ref, uint32_t mask) -> void
	{
		Application::getRenderDevice()->setStencilFunctionInternal(type, ref, mask);
	}

	auto RenderDevice::setStencilMask(uint32_t mask) -> void
	{
		Application::getRenderDevice()->setStencilMaskInternal(mask);
	}

	auto RenderDevice::setStencilTest(bool enable) -> void
	{
		Application::getRenderDevice()->setStencilTestInternal(enable);
	}

	auto RenderDevice::setDepthTest(bool enable, StencilType func) -> void
	{
		Application::getRenderDevice()->setDepthTestingInternal(enable, func);
	}

	auto RenderDevice::create() -> std::shared_ptr<RenderDevice>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanRenderDevice>();
#endif        // MAPLE_VULKAN

#ifdef MAPLE_OPENGL
		return std::make_shared<GLRenderDevice>();
#endif
	}
}        // namespace maple
