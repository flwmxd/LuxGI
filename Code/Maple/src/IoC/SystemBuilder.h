//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Engine/Profiler.h"

#include <Scene/Entity/Entity.h>
#include <Engine/JobSystem.h>

#include "SystemAssembler.h"
#include "Registry.h"

namespace maple
{
	namespace ioc
	{
		template <>
		struct DependencyBuilder<maple::Entity>
		{
			DependencyBuilder(entt::registry& r, entt::entity entity) :
				r(r), entity(entity)
			{}
			entt::registry& r;
			entt::entity    entity;

			inline auto available()
			{
				return true;
			}

			inline auto build() -> maple::Entity
			{
				return { entity, r };
			}
		};
	}        // namespace ecs

	struct SystemQueue
	{
		SystemQueue(const std::string& name) :
			name(name) {};

		std::string                    name;
		std::vector<ioc::SystemInfo*> jobs;
		//SysytemId -> Dependency
		std::unordered_map<uint32_t, ioc::SystemInfo> systemInfos;
	};

	class MAPLE_EXPORT SystemBuilder
	{
	public:
		using Ptr = std::shared_ptr<SystemBuilder>;

		inline SystemBuilder() :
			gameStartQueue("GameStart"),
			gameEndedQueue("GameEnded"),
			updateQueue("Update"),
			imGuiQueue("ImGui"),
			factoryQueue("Factory"),
			frameEndQueue("FrmeEnd") {};

		inline auto registerQueue(SystemQueue& queue)
		{
			graph.emplace_back(&queue);
		}

		template <typename... Components>
		inline auto registerGlobalComponent(const std::function<void(Components &...)>& onInit = nullptr) -> void
		{
			if (onInit != nullptr)
				onInit(getGlobalComponent<Components>()...);
			else
				(getGlobalComponent<Components>(), ...);
		}

		template <auto System>
		inline auto registerFactorySystem() -> void
		{
			expand(ioc::SystemFunction<System>{}, factoryQueue);
		}

		template <auto System>
		inline auto registerSystem() -> void
		{
			expand(ioc::SystemFunction<System>{}, updateQueue);
		}

		template <auto System>
		inline auto registerSystemInFrameEnd() -> void
		{
			expand(ioc::SystemFunction<System>{}, frameEndQueue);
		}

		template <auto System>
		inline auto registerGameStart() -> void
		{
			expand(ioc::SystemFunction<System>{}, gameStartQueue);
		}

		template <auto System>
		inline auto registerGameEnded() -> void
		{
			expand(ioc::SystemFunction<System>{}, gameEndedQueue);
		}

		template <auto System>
		inline auto registerOnImGui() -> void
		{
			expand(ioc::SystemFunction<System>{}, imGuiQueue);
		}

		template <auto System>
		inline auto registerWithinQueue(SystemQueue& queue)
		{
			expand(ioc::SystemFunction<System>{}, queue);
		}

		template <typename R, typename... T>
		inline auto addDependency() -> void
		{
			(registry.on_construct<R>().connect<&entt::registry::get_or_emplace<T>>(), ...);
		}

		auto clear() -> void;
		auto removeAllChildren(entt::entity entity, bool root = true) -> void;
		auto removeEntity(entt::entity entity) -> void;

		auto create()->Entity;
		auto create(const std::string& name)->Entity;
		auto getEntityByName(const std::string& name)->Entity;

		inline auto& getRegistry()
		{
			return registry;
		}

		template <typename... Components>
		inline auto addGlobalComponent()
		{
			(registry.template emplace<Components>(), ...);
		}

		template <typename Component, typename... Args>
		inline auto& getGlobalComponent(Args &&...args)
		{
			if (auto ret = registry.ctx().find<Component>(); ret != nullptr)
			{
				return *ret;
			}
			return registry.ctx().emplace<Component>(std::forward<Args>(args)...);
		}

		template <typename TComponent, auto Candidate>
		inline auto onConstruct(bool connect = true)
		{
			if (connect)
				registry.template on_construct<TComponent>().connect<&delegateComponent<Candidate>>();
			else
				registry.template on_construct<TComponent>().disconnect<&delegateComponent<Candidate>>();
		}

		template <typename TComponent, auto Candidate>
		inline auto onUpdate(bool connect = true)
		{
			if (connect)
				registry.template on_update<TComponent>().connect<&delegateComponent<Candidate>>();
			else
				registry.template on_update<TComponent>().disconnect<&delegateComponent<Candidate>>();
		}

		template <typename TComponent, auto Candidate>
		inline auto onDestory(bool connect = true)
		{
			if (connect)
				registry.template on_destroy<TComponent>().connect<&delegateComponent<Candidate>>();
			else
				registry.template on_destroy<TComponent>().disconnect<&delegateComponent<Candidate>>();
		}

		//these two will be re-factored in the future because of the builder could belong to scene ?
		inline auto onGameStart()
		{
			for (auto& job : gameStartQueue.jobs)
			{
				job->systemCall(registry);
			}
		}

		inline auto onGameEnded()
		{
			for (auto& job : gameEndedQueue.jobs)
			{
				job->systemCall(registry);
			}
		}

	private:
		friend class Application;

		inline auto flushJobs(SystemQueue& queue)
		{
			for (auto& func : queue.jobs)
			{
				func->systemCall(registry);
			}
		}

		template <auto Candidate>
		static auto delegateComponent(entt::registry& registry, entt::entity entity)
		{
			auto call = ioc::SystemAssembler::template assembleSystem(ioc::SystemFunction<Candidate>{});
			call(
				ioc::SystemFunction<Candidate>{},
				ioc::SystemAssembler::template reflectVariables(registry, entity, ioc::SystemFunction<Candidate>{})
			);
		}

		inline auto onUpdate(float dt)
		{
			flushJobs(updateQueue);
		}

		inline auto executeImGui()
		{
			flushJobs(imGuiQueue);
		}

		inline auto execute()
		{
			if (!factoryQueue.jobs.empty())
			{
				flushJobs(factoryQueue);
				factoryQueue.jobs.clear();
			}

			for (auto g : graph)
			{
				flushJobs(*g);
			}
			flushJobs(frameEndQueue);
		}

		template <auto System>
		inline auto expand(ioc::SystemFunction<System> system, SystemQueue& queue) -> void
		{
			build(system, queue);
		}

		template <typename TSystem>
		inline auto build(TSystem, SystemQueue& queue) -> void
		{
			const auto     id = queue.jobs.size();
			constexpr auto functionName = ioc::SystemAssembler::template getSystemShortName(TSystem{});
			queue.systemInfos[id].systemName = functionName;
			queue.systemInfos[id].systemId = id;
			queue.systemInfos[id].systemCall = [](entt::registry& reg) {
				auto call = ioc::SystemAssembler::template assembleSystem(TSystem{});
				call(TSystem{}, ioc::SystemAssembler::template reflectVariables(reg, TSystem{}));
			};
			queue.jobs.emplace_back(&queue.systemInfos[id]);
		}

		SystemQueue gameStartQueue;

		SystemQueue gameEndedQueue;

		SystemQueue updateQueue;

		SystemQueue frameEndQueue;        //execute in end of every frame

		SystemQueue imGuiQueue;

		SystemQueue factoryQueue;

		std::vector<SystemQueue*> graph;

		entt::registry registry;
	};
};        // namespace maple
