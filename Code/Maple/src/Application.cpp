//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Application.h"

#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Profiler.h"
#include "Engine/Timestep.h"
#include "Engine/JobSystem.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "IoC/SystemBuilder.h"
#include "Scene/SystemBuilder.inl"

#include "Others/Console.h"
#include "Window/WindowWin.h"
#include "Window/MessageBox.h"


#include "Devices/Input.h"
#include "ImGui/ImGuiSystem.h"
#include "ImGui/ImNotification.h"

#include "Engine/Raytrace/RaytraceConfig.h"

#include "IO/Loader.h"

#include "RHI/Texture.h"
#include "Scene/SystemBuilder.inl"

#include <imgui.h>

//maple::Application* app;

namespace maple
{
	Application::Application(AppDelegate *app)
	{
		enableTracer();
		appDelegate     = std::shared_ptr<AppDelegate>(app);
		window          = NativeWindow::create(WindowInitData{1920, 1080, false, "Maple-Engine"});
		renderDevice    = RenderDevice::create();
		graphicsContext = GraphicsContext::create();

		sceneManager  = std::make_unique<SceneManager>();
		renderGraph   = std::make_shared<RenderGraph>();
		io::init();
		JobSystem::init(std::thread::hardware_concurrency() - 2);
	}

	auto Application::init() -> void
	{
		PROFILE_FUNCTION();
		builder = std::make_shared<SystemBuilder>();
		builder->addDependency<Camera, component::Transform>();
		builder->addDependency<component::Light, component::Transform>();
		builder->addDependency<component::MeshRenderer, component::Transform>();
		builder->addDependency<component::SkinnedMeshRenderer, component::Transform>();
		builder->addDependency<component::VolumetricCloud, component::Light>();
		builder->addDependency<component::Hierarchy, component::Transform>();

		builder->getGlobalComponent<global::component::DeltaTime>();
		builder->getGlobalComponent<global::component::AppState>();
		builder->getGlobalComponent<global::component::Bindless>();
		builder->getGlobalComponent<global::component::MaterialChanged>();
		builder->getGlobalComponent<global::component::GraphicsContext>().context = graphicsContext;
		builder->getGlobalComponent<global::component::RenderDevice>().device     = renderDevice.get();
		builder->getGlobalComponent<global::component::Profiler>();
		builder->getGlobalComponent<trace::global::component::RaytraceConfig>();

		Input::create();
		window->init();
		graphicsContext->init();
		renderDevice->init();

		timer.start();
		renderGraph->init(window->getWidth(), window->getHeight());

		imGuiManager = std::make_shared<ImGuiSystem>(false);
		imGuiManager->onInit();

		registerSystem(builder);

		appDelegate->onInit();
	}

	auto Application::start() -> int32_t
	{
		double lastFrameTime = 0;
		init();
		auto & profiler = builder->getGlobalComponent<global::component::Profiler>();
		while (!window->isClose())
		{
			PROFILE_FRAMEMARKER();
			Input::getInput()->resetPressed();
			Timestep timestep = timer.stop() / 1000000.f;
			if (!minimized)
				imGuiManager->newFrame(timestep);
			{
				sceneManager->apply();
				executeAll();
				onUpdate(timestep);
				onRender();
				frames++;
				profiler.frameCount++;
			}
			graphicsContext->clearUnused();
			lastFrameTime += timestep;
			if (lastFrameTime - secondTimer > 1.0f)        //tick later
			{
				secondTimer += 1.0f;
				profiler.fps = (float)frames / (lastFrameTime - secondTimer);
				window->setTitle("Maple-Engine - FPS:" + std::to_string(frames));
				frames  = 0;
				updates = 0;
			}
		}

		appDelegate->onDestory();
		return 0;
	}

	auto Application::setEditorState(EditorState state) -> void
	{
		this->state                                                           = state;
		builder->getGlobalComponent<global::component::AppState>().state = state;

		if (state == EditorState::Play)
		{
			builder->onGameStart();
		}
		else
		{
			builder->onGameEnded();
		}
	}

	//update all things
	auto Application::onUpdate(const Timestep &delta) -> void
	{
		PROFILE_FUNCTION();
		if (!minimized)
		{
			auto scene = sceneManager->getCurrentScene();
			scene->onUpdate(delta);
			onImGui(delta);
			imGuiManager->update();
			window->onUpdate();
			dispatcher.dispatchEvents();
			builder->onUpdate(delta);
			renderGraph->onUpdate(delta, scene);
		}
		else
		{
			window->onUpdate();
			dispatcher.dispatchEvents();
		}
	}

	auto Application::onRender() -> void
	{
		PROFILE_FUNCTION();
		if (!minimized)
		{
			renderDevice->begin();
			renderGraph->beginScene(sceneManager->getCurrentScene());
			builder->execute();
			imGuiManager->onRender(sceneManager->getCurrentScene());
			renderDevice->present(); 
			window->swapBuffers();
			renderGraph->pingPong();
		}
	}

	auto Application::beginScene() -> void
	{
	}

	auto Application::onImGui(const Timestep &delta) -> void
	{
		PROFILE_FUNCTION();
		builder->executeImGui();
		ImNotification::onImGui();
	}

	auto Application::setSceneActive(bool active) -> void
	{
		sceneActive = active;
	}

	auto Application::onSceneCreated(Scene *scene) -> void
	{
		if (state == EditorState::Paused) 
		{
			setEditorState(EditorState::Play);
			imGuiManager->onResize(window->getWidth(), window->getHeight());
		}
	}

	auto Application::onWindowResized(uint32_t w, uint32_t h) -> void
	{
		PROFILE_FUNCTION();
		
		minimized = w == 0 || h == 0;
		
		if (w == 0 || h == 0)
			return;

		graphicsContext->waitIdle();
		renderDevice->onResize(w, h);
		imGuiManager->onResize(w, h);
		if (!editor)
		{
			renderGraph->onResize(w, h);
		}
		graphicsContext->waitIdle();
	}

	auto Application::postOnMainThread(const std::function<bool()> &mainCallback) -> std::future<bool>
	{
		PROFILE_FUNCTION();
		std::promise<bool> promise;
		std::future<bool>  future = promise.get_future();

		std::lock_guard<std::mutex> locker(executeMutex);
		eventQueue.emplace(std::move(promise), mainCallback);
		return future;
	}

	auto Application::executeAll() -> void
	{
		PROFILE_FUNCTION();
		std::pair<std::promise<bool>, std::function<bool(void)>> func;
		for (;;)
		{
			{
				std::lock_guard<std::mutex> lock(executeMutex);
				if (eventQueue.empty())
					break;
				func = std::move(eventQueue.front());
				eventQueue.pop();
			}
			if (func.second)
			{
				func.first.set_value(func.second());
			}
		}
	}

	auto AppDelegate::getScene() -> Scene *
	{
		return Application::get()->getCurrentScene();
	}

	maple::Application *Application::app = nullptr;

};        // namespace maple
