//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <list>
#include <memory>

#include "Engine/Core.h"
#include "Engine/Renderer/RenderGraph.h"
#include "Engine/Timestep.h"
#include "Event/EventDispatcher.h"
#include "ImGui/ImGuiSystem.h"
#include "Others/Timer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/RenderDevice.h"
#include "Scene/Component/AppState.h"
#include "Scene/SceneManager.h"
#include "Window/NativeWindow.h"

namespace maple
{
	class SystemBuilder;

	class MAPLE_EXPORT AppDelegate
	{
	public:
		virtual auto onInit() -> void = 0;
		virtual auto onDestory() -> void = 0;
		auto         getScene()->Scene*;
	};

	class DefaultDelegate final : public AppDelegate
	{
	public:
		virtual auto onInit() -> void override {};
		virtual auto onDestory() -> void override {};
	};

	class MAPLE_EXPORT Application
	{
	public:
		Application(AppDelegate* appDelegate);

		auto start()->int32_t;
		auto setSceneActive(bool active) -> void;
		auto postOnMainThread(const std::function<bool()>& mainCallback)->std::future<bool>;
		auto executeAll() -> void;

		virtual auto init() -> void;
		virtual auto onUpdate(const Timestep& delta) -> void;
		virtual auto onRender() -> void;
		virtual auto beginScene() -> void;
		virtual auto onImGui(const Timestep& delta) -> void;
		virtual auto onSceneCreated(Scene* scene) -> void;
		virtual auto onWindowResized(uint32_t w, uint32_t h) -> void;

		inline static auto& getEventDispatcher()
		{
			return get()->dispatcher;
		}

		inline static auto& getWindow()
		{
			return get()->window;
		}

		inline static auto& getSceneManager()
		{
			return get()->sceneManager;
		}
		inline auto isSceneActive() const
		{
			return sceneActive;
		}

		inline auto& getEditorState() const
		{
			return state;
		}

		inline static auto& getRenderGraph()
		{
			return get()->renderGraph;
		}

		template <class T>
		inline auto getAppDelegate()
		{
			return std::static_pointer_cast<T>(appDelegate);
		}

		inline static auto& getGraphicsContext()
		{
			return get()->graphicsContext;
		}

		inline static auto& getRenderDevice()
		{
			return get()->renderDevice;
		}

		inline static auto& getTimer()
		{
			return get()->timer;
		}

		inline static auto get() -> Application*
		{
			return app;
		}

		inline static auto getCurrentScene() -> Scene*
		{
			return get()->getSceneManager()->getCurrentScene();
		}

		inline static auto getBuilder()
		{
			return get()->builder;
		}

		auto setEditorState(EditorState state) -> void;

		static Application* app;

	protected:
		std::unique_ptr<NativeWindow>    window;
		std::unique_ptr<SceneManager>	 sceneManager;
		std::shared_ptr<ImGuiSystem>     imGuiManager;
		std::shared_ptr<AppDelegate>     appDelegate;
		std::shared_ptr<RenderDevice>    renderDevice;
		std::shared_ptr<GraphicsContext> graphicsContext;
		std::shared_ptr<RenderGraph>     renderGraph;
		std::shared_ptr<SystemBuilder>   builder;

		EventDispatcher dispatcher;
		Timer           timer;
		uint64_t        updates = 0;
		uint64_t        frames = 0;
		float           secondTimer = 0.0f;
		bool            sceneActive = true;
		bool            editor = false;
		bool 			 minimized = false;
		EditorState     state = EditorState::Paused;
		std::mutex      executeMutex;

		std::queue<std::pair<std::promise<bool>, std::function<bool()>>> eventQueue;
	};

};        // namespace maple
