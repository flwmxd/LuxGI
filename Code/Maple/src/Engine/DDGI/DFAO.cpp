//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DFAO.h"
#include "RHI/Shader.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "GlobalDistanceField.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/GBuffer.h"
#include <imgui.h>
#include "ImGui/ImGuiHelpers.h"

namespace maple::sdf::ao
{
	namespace global::component 
	{
		struct DFAOBlur 
		{
			Shader::Ptr shader;
			DescriptorSet::Ptr sets;
		};
	}

	namespace render 
	{
		inline auto system(const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
			const maple::component::RendererData& renderData,
			const maple::component::CameraView& cameraView,
			const maple::component::WindowSize& winSize,
			const maple::global::component::Profiler& profiler,
			global::component::DFAO& dfao)
		{
			if (dfao.outColor[0] == nullptr)
			{
				dfao.outColor[0] = Texture2D::create(winSize.width, winSize.height, nullptr, { TextureFormat::R8 });
				dfao.outColor[1] = Texture2D::create(winSize.width, winSize.height, nullptr, { TextureFormat::R8 });
			}

			dfao.sets->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
			dfao.sets->setTexture("uDepthSampler", renderData.gbuffer->getDepthBuffer());
			dfao.sets->setTexture("outColor", dfao.outColor[0]);
			dfao.sets->setTexture("uGlobalSDF", sdfPublic.texture);
			dfao.sets->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);
			dfao.sets->setUniform("UniformBufferObject", "data", &sdfPublic.sdfCommonData);
			dfao.sets->setUniform("UniformBufferObject", "viewProjInv", glm::value_ptr(glm::inverse(cameraView.projView)));
			dfao.sets->setUniform("UniformBufferObject", "step", &dfao.step);
			dfao.sets->setUniform("UniformBufferObject", "dist", &dfao.dist);
			dfao.sets->setUniform("UniformBufferObject", "intensity", &dfao.intensity);
			dfao.sets->setUniform("UniformBufferObject", "numFrames", &profiler.frameCount);

			PipelineInfo info{};
			info.shader = dfao.shader;
			auto     pip = Pipeline::get(info);
			uint32_t dispatchGroupsX = static_cast<uint32_t>(ceil(float(winSize.width) / float(8)));
			uint32_t dispatchGroupsY = static_cast<uint32_t>(ceil(float(winSize.height) / float(8)));
			Renderer::dispatch(renderData.commandBuffer, dispatchGroupsX, dispatchGroupsY, 1, pip.get(), nullptr, { dfao.sets });
		}
	}

	namespace dfao_blur
	{
		inline auto system(global::component::DFAOBlur& dfaoBlur,
			const global::component::DFAO& dfao, 
			const maple::component::RendererData& renderData)
		{
			if (dfao.outColor[0] == nullptr)
				return;
			dfaoBlur.sets->setTexture("inputColor", dfao.outColor[0]);
			dfaoBlur.sets->setTexture("outColor", dfao.outColor[1]);
			dfaoBlur.sets->update(renderData.commandBuffer);

			auto commandBuffer = renderData.commandBuffer;

			PipelineInfo pipeInfo;
			pipeInfo.pipelineName = "DFAO-Blur";
			pipeInfo.shader = dfaoBlur.shader;
			auto pipeline = Pipeline::get(pipeInfo);
			uint32_t dispatchGroupsX = static_cast<uint32_t>(ceil(float(dfao.outColor[1]->getWidth()) / float(8)));
			uint32_t dispatchGroupsY = static_cast<uint32_t>(ceil(float(dfao.outColor[1]->getHeight()) / float(8)));
			Renderer::dispatch(renderData.commandBuffer, dispatchGroupsX, dispatchGroupsY, 1, pipeline.get(), nullptr, { dfaoBlur.sets });
		}
	}        // namespace ssao_blur_pass
	namespace on_imgui
	{
		inline auto system(global::component::DFAO& dfao)
		{
			if (ImGui::Begin("DFAO Debugger."))
			{
				ImGui::Columns(2);
				ImGuiHelper::property("Steps", dfao.step, 0,20);
				ImGuiHelper::property("Distant", dfao.dist, 0.f, 15.f);
				ImGuiHelper::property("Intensity", dfao.intensity, 0.f, 10);
				ImGui::Columns(1);
			}
			ImGui::End();
		}
	}        // namespace on_imgui


	auto registerSDFAO(SystemQueue& begin, SystemQueue& render, std::shared_ptr<SystemBuilder> builder) -> void 
	{
		builder->registerGlobalComponent<global::component::DFAO>([](global::component::DFAO & field) {
			field.shader = Shader::create("shaders/SDF/SDFAO.shader");
			field.sets = DescriptorSet::create({0,field.shader.get()});
		});

		builder->registerGlobalComponent<global::component::DFAOBlur>([](global::component::DFAOBlur& field) {
			field.shader = Shader::create("shaders/SDF/SDFAOBlur.shader");
			field.sets = DescriptorSet::create({ 0,field.shader.get() });
		});

		builder->registerWithinQueue<render::system>(render);
		builder->registerWithinQueue<dfao_blur::system>(render);
		builder->registerOnImGui<on_imgui::system>();
	}
}