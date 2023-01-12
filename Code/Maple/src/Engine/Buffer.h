//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Others/Console.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>

namespace maple
{
	struct Buffer
	{
		uint8_t *data;
		uint32_t size;

		Buffer() :
		    data(nullptr),
		    size(0)
		{
		}

		Buffer(void *data, uint32_t size) :
		    data((uint8_t *) data),
		    size(size)
		{
		}

		static Buffer copy(const void *data, uint32_t size)
		{
			Buffer buffer;
			buffer.allocate(size);
			memcpy(buffer.data, data, size);
			return buffer;
		}

		inline auto allocate(uint32_t size) -> void
		{
			if (data != nullptr)
			{
				delete[] data;
			}
			data = nullptr;

			if (size == 0)
				return;

			data       = new uint8_t[size];
			this->size = size;
		}

		inline auto release()
		{
			delete[] data;
			data = nullptr;
			size = 0;
		}

		inline auto initializeEmpty()
		{
			if (data)
				memset(data, 0, size);
		}

		template <typename T>
		inline auto &read(uint32_t offset = 0)
		{
			return *(T *) ((uint8_t *) data + offset);
		}

		inline auto readBytes(uint32_t size, uint32_t offset)
		{
			MAPLE_ASSERT(offset + size <= this->size, "Index out of bounds");
			uint8_t *buffer = new uint8_t[size];
			memcpy(buffer, (uint8_t *) data + offset, size);
			return buffer;
		}

		inline auto write(const void *buffer, uint32_t size, uint32_t offset = 0)
		{
			MAPLE_ASSERT(offset + size <= this->size, "Index out of bounds");
			memcpy((uint8_t *) data + offset, buffer, size);
		}

		inline operator bool() const
		{
			return data;
		}

		inline auto operator[](int index) -> uint8_t &
		{
			return ((uint8_t *) data)[index];
		}

		inline auto operator[](int index) const
		{
			return ((uint8_t *) data)[index];
		}

		template <typename T>
		inline auto as()
		{
			return (T *) data;
		}

		inline auto getSize() const
		{
			return size;
		}
	};

	class BinaryReader
	{
	  private:
		const uint8_t *data;
		uint32_t       size;
		uint32_t       offset;

	  public:
		BinaryReader(const uint8_t *data, uint32_t size) :
		    data(data),
		    size(size),
		    offset(0)
		{
		}

		template <typename T>
		inline auto &read()
		{
			MAPLE_ASSERT(offset + sizeof(T) <= this->size, "Index out of bounds");

			T &t = *(T *) ((uint8_t *) data + offset);
			offset += sizeof(T);
			return t;
		}

		inline auto readVec2() -> glm::vec2
		{
			MAPLE_ASSERT(offset + sizeof(glm::vec2) <= this->size, "Index out of bounds");

			glm::vec2 vec = {};
			vec.x         = read<float>();
			vec.y         = read<float>();
			return vec;
		}

		inline auto readVec3() -> glm::vec3
		{
			MAPLE_ASSERT(offset + sizeof(glm::vec3) <= this->size, "Index out of bounds");
			glm::vec3 vec = {};
			vec.x         = read<float>();
			vec.y         = read<float>();
			vec.z         = read<float>();
			return vec;
		}

		inline auto readVec4() -> glm::vec4
		{
			MAPLE_ASSERT(offset + sizeof(glm::vec4) <= this->size, "Index out of bounds");
			glm::vec4 vec = {};
			vec.x         = read<float>();
			vec.y         = read<float>();
			vec.z         = read<float>();
			vec.w         = read<float>();
			return vec;
		}

		inline auto readBytes(uint32_t size) -> std::unique_ptr<uint8_t[]>
		{
			MAPLE_ASSERT(offset + size <= this->size, "Index out of bounds");
			uint8_t *buffer = new uint8_t[size];
			memcpy(buffer, (uint8_t *) data + offset, size);
			offset += size;
			return std::unique_ptr<uint8_t[]>(buffer);
		}

		inline auto skip(uint32_t size)
		{
			MAPLE_ASSERT(offset + size <= this->size, "Index out of bounds");
			offset += size;
		}
	};
}        // namespace maple
