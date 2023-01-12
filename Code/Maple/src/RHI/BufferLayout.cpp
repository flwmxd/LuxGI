//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "BufferLayout.h"
#include "GraphicsContext.h"

namespace maple
{
	BufferLayout::BufferLayout()
	{
	}

	auto BufferLayout::reset() -> void
	{
		size = 0;
		layouts.clear();
	}

	auto BufferLayout::push(const std::string &name, Format format, uint32_t s, uint32_t location, bool normalized) -> void
	{
		layouts[location] = {name, format, s, normalized};
	}

	auto BufferLayout::computeStride() -> void
	{
		uint32_t stride = 0;
		for (auto &layout : layouts)
		{
			auto size     = layout.offset;
			layout.offset = stride;
			stride += size;
		}
		size = stride;
	}

	template <>
	auto BufferLayout::push<uint32_t>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32_UINT, sizeof(uint32_t), location, normalized);
	}

	template <>
	auto BufferLayout::push<uint8_t>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R8_UINT, sizeof(uint8_t), location, normalized);
	}

	template <>
	auto BufferLayout::push<float>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32_FLOAT, sizeof(float), location, normalized);
	}

	template <>
	auto BufferLayout::push<glm::vec2>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32G32_FLOAT, sizeof(glm::vec2), location, normalized);
	}

	template <>
	auto BufferLayout::push<glm::vec3>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32G32B32_FLOAT, sizeof(glm::vec3), location, normalized);
	}

	template <>
	auto BufferLayout::push<glm::vec4>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32G32B32A32_FLOAT, sizeof(glm::vec4), location, normalized);
	}

	template <>
	auto BufferLayout::push<int32_t>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32_INT, sizeof(glm::ivec2), location, normalized);
	}
	template <>
	auto BufferLayout::push<glm::ivec2>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32G32_INT, sizeof(glm::ivec2), location, normalized);
	}

	template <>
	auto BufferLayout::push<glm::ivec3>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32G32B32_INT, sizeof(glm::ivec3), location, normalized);
	}
	template <>
	auto BufferLayout::push<glm::ivec4>(const std::string &name, uint32_t location, bool normalized) -> void
	{
		push(name, Format::R32G32B32A32_INT, sizeof(glm::ivec4), location, normalized);
	}

}        // namespace maple
