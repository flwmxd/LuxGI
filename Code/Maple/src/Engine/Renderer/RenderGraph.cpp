//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderGraph.h"
#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/CaptureGraph.h"
#include "Engine/DDGI/DDGIRenderer.h"
#include "Engine/DDGI/DDGIVisualization.h"
#include "Engine/GBuffer.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Raytrace/RaytracedReflection.h"
#include "Engine/Raytrace/RaytracedShadow.h"
#include "Engine/VXGI/Voxelization.h"
#include "Engine/Vertex.h"

#include "Engine/DDGI/GlobalDistanceField.h"
#include "Engine/DDGI/GlobalSurfaceAtlas.h"

#include "RHI/CommandBuffer.h"
#include "RHI/GPUProfile.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/VertexBuffer.h"

#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"
#include "Engine/Renderer/BindlessModule.h"

#include "Math/BoundingBox.h"
#include "Math/MathUtils.h"

#include "Window/NativeWindow.h"

#include "AtmosphereRenderer.h"
#include "DeferredOffScreenRenderer.h"

#include "FinalPass.h"
#include "RendererData.h"
#include "ShadowRenderer.h"
#include "SkyboxRenderer.h"

#include "ImGui/ImGuiHelpers.h"
#include "Others/Randomizer.h"

#include "IoC/Registry.h"

namespace maple
{
	namespace on_begin_renderer
	{
		inline auto system(component::RendererData& renderer,
			global::component::RenderDevice& device)
		{
			if (renderer.gbuffer == nullptr)
				return;
			auto renderTargert = renderer.gbuffer->getBuffer(GBufferTextures::SCREEN);
			device.device->clearRenderTarget(renderer.gbuffer->getDepthBuffer(), renderer.commandBuffer, { 1, 1, 1, 1 });
			device.device->clearRenderTarget(renderTargert, renderer.commandBuffer);
			device.device->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::COLOR), renderer.commandBuffer, { 0, 0, 0, 0 });
			device.device->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::NORMALS), renderer.commandBuffer, { 0, 0, 0, 0 });
			device.device->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::PBR), renderer.commandBuffer, { 0, 0, 0, 0 });
			device.device->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::EMISSIVE), renderer.commandBuffer, { 0, 0, 0, 0 });
			device.device->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING), renderer.commandBuffer, { 0, 0, 0, 0 });
			renderer.numFrames++;
		}
	}        // namespace on_begin_renderer

	auto RenderGraph::init(uint32_t width, uint32_t height) -> void
	{
		auto builder = Application::getBuilder();
		gBuffer = std::make_shared<GBuffer>(width, height);
		auto& renderData = builder->getGlobalComponent<component::RendererData>();
		renderData.screenQuad = Mesh::createQuad(true);
		renderData.unitCube = TextureCube::create(1);
		renderData.unitTexture3D = Texture3D::create(1, 1, 1);
		renderData.unitTexture = Texture2D::create();
		renderData.unitTexture->buildTexture(TextureFormat::RGBA, 1, 1);
		renderData.gbuffer = gBuffer.get();
		renderData.gbuffer->resize(width, height);
		auto& winSize = builder->getGlobalComponent<component::WindowSize>();
		winSize.height = height;
		winSize.width = width;
		builder->getGlobalComponent<capture_graph::component::RenderGraph>();
		builder->getGlobalComponent<component::CameraView>();
		builder->getGlobalComponent<component::FinalPass>();

		static SystemQueue beginQ("BegineScene");
		static SystemQueue renderQ("OnRender");

		builder->registerQueue(beginQ);
		builder->registerQueue(renderQ);
		builder->registerWithinQueue<on_begin_renderer::system>(renderQ);

		raytracing::registerAccelerationStructureModule(beginQ, builder);
		bindless::registerBindlessModule(beginQ, renderQ, builder);
		shadow_map::registerShadowMap(beginQ, renderQ, builder);
		deferred_offscreen::registerGBufferRenderer(beginQ, renderQ, builder);
		sdf::registerGlobalDistanceRenderer(beginQ, renderQ, builder);
		sdf::registerGlobalSurfaceAtlas(beginQ, renderQ, builder);
		ddgi::registerDDGI(beginQ, renderQ, builder);
		raytraced_reflection::registerRaytracedReflection(beginQ, renderQ, builder);
		raytraced_shadow::registerRaytracedShadow(beginQ, renderQ, builder);
		deferred_lighting::registerDeferredLighting(beginQ, renderQ, builder);
		atmosphere_pass::registerAtmosphere(beginQ, renderQ, builder);
		skybox_renderer::registerSkyboxRenderer(beginQ, renderQ, builder);
		ddgi::registerDDGIVisualization(beginQ, renderQ, builder);
		sdf::registerSDFVisualizer(beginQ, renderQ, builder);
		final_screen_pass::registerFinalPass(renderQ, builder);
	}

	auto RenderGraph::beginScene(Scene* scene) -> void
	{
		PROFILE_FUNCTION();

		auto& renderData = Application::getBuilder()->getGlobalComponent<component::RendererData>();
		renderData.commandBuffer = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();

		for (auto& task : tasks)
		{
			task(renderData.commandBuffer);
		}
		tasks.clear();

		auto camera = scene->getCamera();
		if (camera.first == nullptr || camera.second == nullptr)
		{
			return;
		}

		auto& cameraView = Application::getBuilder()->getGlobalComponent<component::CameraView>();
		cameraView.proj = camera.first->getProjectionMatrix();
		cameraView.view = camera.second->getWorldMatrixInverse();
		cameraView.projView = cameraView.proj * cameraView.view;
		cameraView.nearPlane = camera.first->getNear();
		cameraView.farPlane = camera.first->getFar();
		cameraView.frustum = camera.first->getFrustum(cameraView.view);
		cameraView.cameraTransform = camera.second;
		cameraView.fov = camera.first->getFov();
		cameraView.aspect = camera.first->getAspectRatio();
		cameraView.cameraDelta = camera.second->getWorldPosition() - cameraView.prevPosition;
		cameraView.prevPosition = camera.second->getWorldPosition();
	}

	auto RenderGraph::onUpdate(const Timestep& step, Scene* scene) -> void
	{
		PROFILE_FUNCTION();
		auto& renderData = Application::getBuilder()->getGlobalComponent<component::RendererData>();
		renderData.renderDevice = Application::getRenderDevice().get();
	}

	auto RenderGraph::onResize(uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
		setScreenBufferSize(width, height);
		tasks.emplace_back([&, width, height](const CommandBuffer* command) {
			gBuffer->resize(width, height, command);
		});

		auto& winSize = Application::getBuilder()->getGlobalComponent<component::WindowSize>();
		winSize.height = height;
		winSize.width = width;
	}

	auto RenderGraph::onImGui() -> void
	{
		PROFILE_FUNCTION();
	}

	auto RenderGraph::setRenderTarget(Scene* scene, const std::shared_ptr<Texture>& texture, bool rebuildFramebuffer) -> void
	{
		PROFILE_FUNCTION();
		Application::getBuilder()->getGlobalComponent<component::FinalPass>().renderTarget = texture;
	}

	auto RenderGraph::pingPong() -> void
	{
		gBuffer->pingPong();
		auto& cameraView = Application::getBuilder()->getGlobalComponent<component::CameraView>();
		cameraView.projViewOld = cameraView.projView;
	}
};        // namespace maple
