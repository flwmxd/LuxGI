//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

namespace maple
{
	namespace hash
	{
		template <typename T>
		inline auto hashCode(std::size_t &seed) -> void
		{
		}

		template <typename T, typename... Rest>
		inline auto hashCode(std::size_t &seed, const T &v, Rest... rest) -> void
		{
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			(hashCode(seed, rest), ...);
		}
	};        // namespace HashCode
};            // namespace maple

namespace std
{
	template <>
	struct hash<std::vector<uint32_t>>
	{
		std::size_t operator()(const std::vector<uint32_t> &vec) const
		{
			std::size_t seed = vec.size();
			for (const auto &i : vec)
			{
				seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}
			return seed;
		}
	};
};        // namespace std
