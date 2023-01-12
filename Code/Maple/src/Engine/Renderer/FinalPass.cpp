//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "FinalPass.h"

#include "Engine/CaptureGraph.h"
#include "Engine/Renderer/Renderer.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"

#include "Scene/Scene.h"

#include "Engine/GBuffer.h"
#include "Math/MathUtils.h"
#include "RendererData.h"
#include <glm/glm.hpp>

#include "IoC/Registry.h"

namespace maple
{
	namespace final_screen_pass
	{
		inline auto system(ioc::Registry  registry,
		                   const component::FinalPass &           finalData,
		                   capture_graph::component::RenderGraph &graph,
		                   const component::RendererData &        renderData)
		{
			float gamma      = 2.2;

			finalData.finalDescriptorSet->setUniform("UniformBuffer", "gamma", &gamma);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "toneMapIndex", &finalData.toneMapIndex);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "exposure", &finalData.exposure);
			finalData.finalDescriptorSet->setTexture("uScreenSampler", renderData.gbuffer->getBuffer(GBufferTextures::SCREEN));

			finalData.finalDescriptorSet->update(renderData.commandBuffer);

			PipelineInfo pipelineDesc{};
			pipelineDesc.shader       = finalData.finalShader;
			pipelineDesc.pipelineName = "FinalScreenPass";

			pipelineDesc.polygonMode         = PolygonMode::Fill;
			pipelineDesc.cullMode            = CullMode::None;
			pipelineDesc.transparencyEnabled = false;

			if (finalData.renderTarget)
				pipelineDesc.colorTargets[0] = finalData.renderTarget;
			else
				pipelineDesc.swapChainTarget = true;

			auto pipeline = Pipeline::get(pipelineDesc, {finalData.finalDescriptorSet}, graph);
			pipeline->bind(renderData.commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, {finalData.finalDescriptorSet});
			Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), renderData.screenQuad.get());

			pipeline->end(renderData.commandBuffer);
		}
	}        // namespace final_screen_pass

	namespace final_screen_pass
	{
		auto registerFinalPass(SystemQueue &renderer, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<component::FinalPass>([](component::FinalPass &data) {
				data.finalShader = Shader::create("shaders/ScreenPass.shader");
				DescriptorInfo descriptorInfo{};
				descriptorInfo.layoutIndex = 0;
				descriptorInfo.shader      = data.finalShader.get();
				data.finalDescriptorSet    = DescriptorSet::create(descriptorInfo);
			});
			builder->registerWithinQueue<final_screen_pass::system>(renderer);
		}
	};        // namespace final_screen_pass
};            // namespace maple
