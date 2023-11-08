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
	namespace render 
	{
		inline auto system(const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic, 
			const maple::component::RendererData& renderData,
			const maple::component::CameraView& cameraView,
			const maple::component::WindowSize& winSize, 
			global::component::DFAO& dfao)
		{
			if (dfao.outColor == nullptr) 
			{
				dfao.outColor = Texture2D::create(winSize.width, winSize.height, nullptr, { TextureFormat::R8 });
			}

			dfao.sets->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
			dfao.sets->setTexture("uDepthSampler", renderData.gbuffer->getDepthBuffer());
			dfao.sets->setTexture("outColor", dfao.outColor);
			dfao.sets->setTexture("uGlobalSDF", sdfPublic.texture);
			dfao.sets->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);
			dfao.sets->setUniform("UniformBufferObject", "data", &sdfPublic.sdfCommonData);
			dfao.sets->setUniform("UniformBufferObject", "viewProjInv", glm::value_ptr(glm::inverse(cameraView.projView)));
			dfao.sets->setUniform("UniformBufferObject", "step", &dfao.step);
			dfao.sets->setUniform("UniformBufferObject", "dist", &dfao.dist);

			PipelineInfo info{};
			info.shader = dfao.shader;
			auto     pip = Pipeline::get(info);
			uint32_t dispatchGroupsX = static_cast<uint32_t>(ceil(float(winSize.width) / float(8)));
			uint32_t dispatchGroupsY = static_cast<uint32_t>(ceil(float(winSize.height) / float(8)));
			Renderer::dispatch(renderData.commandBuffer, dispatchGroupsX, dispatchGroupsY, 1, pip.get(), nullptr, { dfao.sets });
		}
	}


	namespace on_imgui
	{
		inline auto system(global::component::DFAO& dfao)
		{
			if (ImGui::Begin("DFAO Debugger."))
			{
				ImGui::Columns(2);
				ImGuiHelper::property("Steps", dfao.step, 0,20);
				ImGuiHelper::property("Distant", dfao.dist, 0.f, 15.f);
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
		builder->registerWithinQueue<render::system>(render);
		builder->registerOnImGui<on_imgui::system>();
	}
}