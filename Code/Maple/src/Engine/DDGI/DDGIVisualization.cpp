//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIVisualization.h"
#include "DDGIRenderer.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RendererData.h"
#include "RHI/DescriptorSet.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Pipeline.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/Transform.h"

namespace maple
{
	namespace ddgi
	{
		namespace component
		{
			struct VisualizationPipeline
			{
				Shader::Ptr   shader;
				Pipeline::Ptr pipeline;
				Mesh::Ptr     sphere;
				Texture::Ptr  nullTexture;

				std::vector<DescriptorSet::Ptr> descriptorSets;
			};
		}        // namespace component

		namespace delegates
		{
			inline auto initVisualization(ddgi::component::DDGIVisualization& viusal, Entity entity, const maple::component::RendererData& renderData)
			{
				auto& pipeline = entity.addComponent<component::VisualizationPipeline>();

				pipeline.shader = Shader::create("shaders/DDGI/ProbeVisualization.shader");
				pipeline.sphere = Mesh::createSphere();
				uint32_t white = -1;
				pipeline.nullTexture = Texture2D::create(1, 1, &white);
				for (uint32_t i = 0; i < 2; i++)
				{
					pipeline.descriptorSets.emplace_back(DescriptorSet::create({ i, pipeline.shader.get() }));
				}
			}
		}        // namespace delegates

		namespace debug_ddgi
		{
			inline auto system(
				const maple::component::RendererData& renderData,
				const maple::component::CameraView& cameraView,
				ioc::Registry registry)
			{
				auto view = registry.getRegistry().view<
					component::DDGIVisualization,
					component::VisualizationPipeline,
					component::IrradianceVolume,
					ddgi::component::DDGIUniform>();

				for (auto [entity, visual, pipeline, ddgipipe, uniform] : view.each())
				{
					auto probeCount = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
					if (probeCount > 27000)
						return;
					if (visual.enable)
					{
						PipelineInfo info;
						info.shader = pipeline.shader;
						info.pipelineName = "Probe-Visualization";
						info.polygonMode = PolygonMode::Fill;
						info.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
						info.clearTargets = false;
						info.swapChainTarget = false;
						info.depthTarget = renderData.gbuffer->getDepthBuffer();
						info.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::SCREEN);
						pipeline.pipeline = Pipeline::get(info);

						pipeline.descriptorSets[0]->setUniform("UniformBufferObject", "viewProj", &cameraView.projView);
						pipeline.descriptorSets[1]->setUniform("DDGIUBO", "ddgi", &uniform);
						pipeline.descriptorSets[1]->setTexture("uIrradiance", ddgipipe.currentIrrdance == nullptr ? renderData.unitTexture : ddgipipe.currentIrrdance);
						pipeline.descriptorSets[1]->setTexture("uDepth", ddgipipe.currentDepth == nullptr ? renderData.unitTexture : ddgipipe.currentDepth);

						pipeline.descriptorSets[0]->update(renderData.commandBuffer);
						pipeline.descriptorSets[1]->update(renderData.commandBuffer);

						pipeline.pipeline->bind(renderData.commandBuffer);

						if (auto push = pipeline.shader->getPushConstant(0))
						{
							push->setData(&visual.scale);
						}

						Renderer::bindDescriptorSets(pipeline.pipeline.get(), renderData.commandBuffer, 0, pipeline.descriptorSets);

						pipeline.shader->bindPushConstants(renderData.commandBuffer, pipeline.pipeline.get());
						pipeline.sphere->getIndexBuffer()->bind(renderData.commandBuffer);
						pipeline.sphere->getVertexBuffer()->bind(renderData.commandBuffer, pipeline.pipeline.get());

						pipeline.pipeline->drawIndexed(
							renderData.commandBuffer,
							pipeline.sphere->getIndexBuffer()->getCount(), probeCount, 0, 0, 0);

						pipeline.pipeline->end(renderData.commandBuffer);
					}
				}
			}
		}        // namespace debug_ddgi

		auto registerDDGIVisualization(SystemQueue& begin, SystemQueue& render, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->onConstruct<ddgi::component::DDGIVisualization, delegates::initVisualization>();
			builder->registerWithinQueue<debug_ddgi::system>(render);
		}
	}        // namespace ddgi
}        // namespace maple