//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <type_traits>
#include <string_view>

namespace maple::ioc
{
	namespace function_info
	{
		using string_view = std::string_view;

		namespace details
		{
			template <typename F>
			constexpr auto functionName() noexcept
			{
				constexpr std::size_t prefix = sizeof("auto __cdecl ecs::function_info::details::functionName<") - 1;
				constexpr string_view name{__FUNCSIG__ + prefix, sizeof(__FUNCSIG__) - prefix};
				return name;
			}

			template <typename F>
			constexpr auto className() noexcept
			{
				constexpr std::size_t prefix = sizeof("auto __cdecl ecs::function_info::details::className<") - 1;
				constexpr std::size_t suffix = sizeof(" >(void) noexcept") - 1;
				constexpr string_view name{__FUNCSIG__ + prefix, sizeof(__FUNCSIG__) - prefix - suffix};
				return name;
			}
		}        // namespace details
	}            // namespace function_info
}        // namespace ecs
