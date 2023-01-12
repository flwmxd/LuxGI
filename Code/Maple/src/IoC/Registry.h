#pragma once
#include <entt.hpp>

namespace maple::ioc
{
	class Registry
	{
	public:
		constexpr Registry(entt::registry& res) :
			res(&res) {}

		template <typename T, typename... Args>
		inline auto &addComponent(entt::entity entity, Args &&...args)
		{
			return res->template emplace<T>(entity, std::forward_as_tuple<Args>(args)...);
		}

		template <typename T>
		inline auto &addComponent(const std::function<void(T&)> & init = nullptr)
		{
			auto & t = res->ctx().emplace<T>();
			if (init)
				init(t);
			return t;
		}

		template <typename T>
		inline auto removeComponent()
		{
			res->ctx().erase<T>();
		}

		template <typename T>
		inline auto &getOrAddComponent(entt::entity entityHandle)
		{
			return res->template get_or_emplace<T>(entityHandle);
		}

		template <typename T>
		inline auto &getComponent(entt::entity entityHandle)
		{
			return *res->template try_get<T>(entityHandle);
		}

		template <typename T>
		inline auto hasComponent() const
		{
			return res->ctx().typename find<T>() != nullptr;
		}

		template <typename T>
		inline auto hasComponent(entt::entity entityHandle) const
		{
			return res->any_of<T>(entityHandle);
		}

		template <typename T>
		inline auto &getComponent()
		{
			return res->ctx().typename at<T>();
		}

		template <typename T>
		inline auto tryGetComponent(entt::entity entityHandle)
		{
			return res->template try_get<T>(entityHandle);
		}

		inline auto isValid(entt::entity entityHandle) const
		{
			return res->valid(entityHandle);
		}

		inline auto &getRegistry()
		{
			return *res;
		};

		template <typename T>
		inline auto removeComponent(entt::entity entityHandle)
		{
			res->template remove<T>(entityHandle);
		}

	private:
		entt::registry* res = nullptr;
	};
};