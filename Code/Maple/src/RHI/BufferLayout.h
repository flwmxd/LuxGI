//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "DescriptorSet.h"
#include "Engine/Core.h"
#include "Others/Console.h"
#include <glm/glm.hpp>
#include <string>

namespace maple
{
	struct MAPLE_EXPORT BufferElement
	{
		std::string name;
		Format      format;
		uint32_t    offset     = 0;
		bool        normalized = false;
	};

	class MAPLE_EXPORT BufferLayout
	{
	  private:
		uint32_t                   size = 0;
		std::vector<BufferElement> layouts;

	  public:
		BufferLayout();
		auto reset() -> void;

		template <typename T>
		auto push(const std::string &name, uint32_t level, bool normalized = false) -> void
		{
			MAPLE_ASSERT(false, "Unknown type!");
		}

		inline auto &getLayout() const
		{
			return layouts;
		}

		inline auto &getLayout()
		{
			return layouts;
		}

		inline auto getStride() const
		{
			return size;
		}

		auto computeStride() -> void;

	  private:
		auto push(const std::string &name, Format format, uint32_t size, uint32_t location, bool normalized) -> void;
	};

	template <>
	auto MAPLE_EXPORT BufferLayout::push<float>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<uint32_t>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<uint8_t>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<glm::vec2>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<glm::vec3>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<glm::vec4>(const std::string &name, uint32_t level, bool normalized) -> void;

	template <>
	auto MAPLE_EXPORT BufferLayout::push<int32_t>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<glm::ivec2>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<glm::ivec3>(const std::string &name, uint32_t level, bool normalized) -> void;
	template <>
	auto MAPLE_EXPORT BufferLayout::push<glm::ivec4>(const std::string &name, uint32_t level, bool normalized) -> void;

}        // namespace maple
