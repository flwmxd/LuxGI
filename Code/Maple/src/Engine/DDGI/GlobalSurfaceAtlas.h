//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Math/OrientedBoundingBox.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Texture.h"
#include "RHI/StorageBuffer.h"
#include "IoC/SystemBuilder.h"

namespace maple::sdf
{
	struct SurfaceAtlasTile;

	namespace surface::component
	{
		struct MeshSurfaceAtlas
		{
			uint64_t            lastFrameUsed;
			uint64_t            lastFrameUpdated;
			uint64_t            lightingUpdateFrame;
			float               radius;
			OrientedBoundingBox obb;
			SurfaceAtlasTile *  tiles[6];
		};
	}        // namespace surface::component

	namespace surface::global::component
	{
		struct GlobalSurfacePublic
		{
			StorageBuffer::Ptr ssboObjectBuffer;
			StorageBuffer::Ptr ssboTileBuffer;
			StorageBuffer::Ptr chunkBuffer;
			StorageBuffer::Ptr culledObjectsBuffer;

			TextureDepth::Ptr surfaceDepth;
			Texture2D::Ptr    surfaceEmissive;
			Texture2D::Ptr    surfaceGBuffer0;        //Color
			Texture2D::Ptr    surfaceGBuffer1;        //Normal
			Texture2D::Ptr    surfaceGBuffer2;        //PBR
			Texture2D::Ptr    surfaceLightCache;
		};
	}        // namespace surface::global::component

	auto registerGlobalSurfaceAtlas(SystemQueue &begin, SystemQueue &render, SystemBuilder::Ptr point) -> void;

	inline auto getTexture(int32_t idx, const surface::global::component::GlobalSurfacePublic &data) -> Texture::Ptr
	{
		switch (idx)
		{
			case 0:
				return data.surfaceEmissive;
			case 1:
				return data.surfaceGBuffer0;
			case 2:
				return data.surfaceGBuffer1;
			case 3:
				return data.surfaceGBuffer2;
			case 4:
				return data.surfaceLightCache;
		}
		return nullptr;
	}
}        // namespace maple::sdf