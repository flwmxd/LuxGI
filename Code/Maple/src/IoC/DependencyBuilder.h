//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include <entt.hpp>

namespace maple::ioc
{
	template <typename T, typename = void>
	struct DependencyBuilder
	{
		DependencyBuilder(entt::registry& r, entt::entity entity = entt::null) :
			r(r), entity(entity)
		{}
		entt::registry& r;
		entt::entity    entity;

		inline auto available()
		{
			return true;
		}

		inline auto build() -> T
		{
			return T{ r };
		}
	};

	template <typename Ts>
	struct DependencyBuilder<Ts&>
	{
		DependencyBuilder(entt::registry& r, entt::entity entity = entt::null) :
			r(r), entity(entity)
		{}
		entt::registry& r;
		entt::entity    entity;

		inline auto available()
		{
			if (entity != entt::null)
			{
				return r.all_of<Ts>(entity);
			}
			return r.ctx().template contains<Ts>();
		}

		inline auto build() -> Ts&
		{
			if (entity != entt::null && r.all_of<Ts>(entity))
			{
				return r.get<Ts>(entity);
			}
			return r.ctx().template at<Ts>();
		}
	};

	template <typename Ts>
	struct DependencyBuilder<const Ts&>
	{
		DependencyBuilder(entt::registry& r, entt::entity entity = entt::null) :
			r(r), entity(entity)
		{}
		entt::registry& r;
		entt::entity    entity;

		inline auto available()
		{
			if (entity != entt::null)
			{
				return r.all_of<Ts>(entity);
			}
			return r.ctx().template contains<Ts>();
		}

		inline auto build() -> const Ts&
		{
			if (entity != entt::null && r.all_of<Ts>(entity))
			{
				return r.get<Ts>(entity);
			}
			return r.ctx().template at<Ts>();
		}
	};

	//############

	template <typename Ts>
	struct DependencyBuilder<Ts*>
	{
		DependencyBuilder(entt::registry& r, entt::entity entity = entt::null) :
			r(r), entity(entity)
		{}
		entt::registry& r;
		entt::entity    entity;

		inline auto available()
		{
			return true;
		}

		inline auto build() -> Ts*
		{
			if (entity != entt::null && r.all_of<Ts>(entity))
			{
				return r.try_get<Ts>(entity);
			}
			return r.ctx().template find<Ts>();
		}
	};

	template <typename Ts>
	struct DependencyBuilder<const Ts*>
	{
		DependencyBuilder(entt::registry& r, entt::entity entity = entt::null) :
			r(r), entity(entity)
		{}
		entt::registry& r;
		entt::entity    entity;

		inline auto available()
		{
			return true;
		}

		inline auto build() -> const Ts*
		{
			if (entity != entt::null && r.all_of<Ts>(entity))
			{
				return r.try_get<Ts>(entity);
			}
			return r.ctx().template find<Ts>();
		}
	};
}
