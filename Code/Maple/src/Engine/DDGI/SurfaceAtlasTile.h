//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <glm/glm.hpp>

namespace maple::sdf
{
	template <typename NodeType, typename SizeType = uint32_t>
	struct RectAtlas
	{
		NodeType* left = nullptr;
		NodeType* right = nullptr;

		SizeType x;
		SizeType y;

		SizeType width;
		SizeType height;

		bool isUsed;

		RectAtlas(SizeType x, SizeType y, SizeType width, SizeType height) :
			x(x), y(y), width(width), height(height), isUsed(false)
		{
		}

		~RectAtlas()
		{
			if (left)
				delete left;
			if (right)
				delete right;
		}

		template <class... Args>
		NodeType* insert(SizeType itemWidth, SizeType itemHeight, SizeType itemPadding, Args &&...args)
		{
			NodeType* result;
			const SizeType paddedWidth = itemWidth + itemPadding;
			const SizeType paddedHeight = itemHeight + itemPadding;

			if (!isUsed && width == paddedWidth && height == paddedHeight)
			{
				isUsed = true;
				result = (NodeType*)this;
				result->onInsert(std::forward<Args>(args)...);
				return result;
			}

			if (left || right)
			{
				if (left)
				{
					result = left->insert(itemWidth, itemHeight, itemPadding, std::forward<Args>(args)...);
					if (result)
						return result;
				}
				if (right)
				{
					result = right->insert(itemWidth, itemHeight, itemPadding, std::forward<Args>(args)...);
					if (result)
						return result;
				}
			}

			// This slot can't fit or has been already occupied
			if (isUsed || paddedWidth > width || paddedHeight > height)
			{
				// Not enough space
				return nullptr;
			}

			// The width and height of the new child node
			const SizeType remainingWidth = std::max(0, width - paddedWidth);
			const SizeType remainingHeight = std::max(0, height - paddedHeight);

			// Split the remaining area around this slot into two children
			if (remainingHeight <= remainingWidth)
			{
				// Split vertically
				left = new NodeType(x, y + paddedHeight, paddedWidth, remainingHeight);
				right = new NodeType(x + paddedWidth, y, remainingWidth, height);
			}
			else
			{
				// Split horizontally
				left = new NodeType(x + paddedWidth, y, remainingWidth, paddedHeight);
				right = new NodeType(x, y + paddedHeight, width, remainingHeight);
			}

			// Shrink the slot to the actual area
			width = paddedWidth;
			height = paddedHeight;

			// Insert into this slot
			isUsed = true;
			result = (NodeType*)this;
			result->onInsert(std::forward<Args>(args)...);
			return result;
		}

		template <class... Args>
		inline auto free(Args &&...args)
		{
			MAPLE_ASSERT(isUsed, "should be in used...");
			isUsed = false;
			((NodeType*)this)->onFree(std::forward<Args>(args)...);
		}
	};

	namespace surface::component
	{
		struct MeshSurfaceAtlas;
	}

	struct GlobalSurfaceAtlasData
	{
		glm::vec3 cameraPos;
		float     chunkSize;
		uint32_t  culledObjectsCapacity;
		uint32_t  resolution;
		uint32_t  objectsCount;
		uint32_t  padding;
	};

	struct SurfaceAtlasTile : public RectAtlas<SurfaceAtlasTile, uint16_t>
	{
		glm::vec3 viewDirection;
		glm::vec3 viewUp;
		glm::vec3 viewPosition;
		glm::vec3 viewBoundsSize;
		glm::mat4 viewMatrix;
		uint32_t  address = 0;
		uint32_t  objectAddressOffset = 0;

		SurfaceAtlasTile(uint16_t x, uint16_t y, uint16_t width, uint16_t height) :
			RectAtlas<SurfaceAtlasTile, uint16_t>(x, y, width, height) {};

		inline auto onFree() {};

		auto onInsert(surface::component::MeshSurfaceAtlas& atlas, int32_t tileIndex) -> void;
	};
}        // namespace maple::sdf