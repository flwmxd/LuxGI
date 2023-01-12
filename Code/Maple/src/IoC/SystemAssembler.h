//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "DependencyBuilder.h"
#include "SystemInfo.h"
#include "TypeList.h"

namespace maple::ioc
{
	template <auto F, typename T = decltype(F)>
	struct SystemFunction
	{
		static constexpr T value = F;

		constexpr operator T() const
		{
			return value;
		};
	};

	struct SystemAssembler
	{
		static constexpr auto leftSize = sizeof("struct ecs::SystemFunction<&__cdecl");

		template <auto System, typename... TArgs>
		static auto assembleSystem(SystemFunction<System, void (*)(TArgs...)>)
		{
			return [](SystemFunction<System, void (*)(TArgs...)> system,
			          std::tuple<DependencyBuilder<TArgs>...> &        dependency) {
				bool available = (std::get<DependencyBuilder<TArgs>>(dependency).available() && ... && true);
				if (available)
				{
					system.value(std::get<DependencyBuilder<TArgs>>(dependency).build()...);
				}
			};
		}

		template <typename... TArgs>
		static auto reflectVariables(entt::registry &r, const TypeList<TArgs...>)
		{
			return std::tuple{ DependencyBuilder<TArgs>{r}...};
		}

		template <auto System, typename... TArgs>
		static auto reflectVariables(entt::registry &r, SystemFunction<System, void (*)(TArgs...)>)
		{
			return reflectVariables(r, ioc::TypeList<TArgs...>{});
		}

		template <typename... TArgs>
		static auto reflectVariables(entt::registry &r, entt::entity entity, const TypeList<TArgs...>)
		{
			return std::tuple{DependencyBuilder<TArgs>{r,entity}...};
		}

		template <auto System, typename... TArgs>
		static auto reflectVariables(entt::registry &r, entt::entity entity, SystemFunction<System, void (*)(TArgs...)>)
		{
			return reflectVariables(r, entity, ioc::TypeList<TArgs...>{});
		}

		template <auto System, typename... TArgs>
		static constexpr auto getSystemFullName(SystemFunction<System, void (*)(TArgs...)> system)
		{
			constexpr auto sing = function_info::details::functionName<decltype(System)>();
			constexpr auto name = function_info::details::functionName<decltype(system)>();
			return std::string_view{name.data() + leftSize, name.size() - sing.size() - 2 - leftSize};
		}

		template <auto System, typename... TArgs>
		static constexpr auto getSystemShortName(SystemFunction<System, void (*)(TArgs...)> system)
		{
			constexpr auto name = function_info::details::functionName<decltype(system)>();
			return std::string_view{name.data() + leftSize, name.find_first_of("(", leftSize) - leftSize};
		}
	};
}        // namespace ecs
