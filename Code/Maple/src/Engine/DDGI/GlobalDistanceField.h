//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "RHI/DescriptorSet.h"
#include "RHI/Texture.h"
#include "SDFBaker.h"
#include "IoC/SystemBuilder.h"
#include "SurfaceAtlasTile.h"

namespace maple::sdf
{
	enum class Quality : uint8_t
	{
		Test
	};

	struct GlobalSDFData        // push consts
	{
		glm::vec4 cascadePosDistance[4];        //center -> xyz / distance -> w
		glm::vec4 cascadeVoxelSize;
		uint32_t  cascadesCount;
		float     resolution;
		float     nearPlane;
		float     farPlane;
	};

	namespace global::component
	{
		struct GlobalDistanceFieldPublic
		{
			baker::SDFBakerConfig  config;
			float                  minObjectRadius = 10;
			float                  gloalScale      = 0.1;
			DescriptorSet::Ptr     sdfSet;
			GlobalSDFData          sdfCommonData;
			GlobalSurfaceAtlasData globalSurfaceAtlasData{};
			float                  giDistance = 4000;
			Texture3D::Ptr         texture;
			Texture3D::Ptr         mipTexture;
		};
	}        // namespace global::component
	/**
	* 1. Merge. and Generate Chunk.
	* 2. Generate Cascade
	* 3. Update Every Cascade with frame.
	*/
	auto registerGlobalDistanceRenderer(SystemQueue &begin, SystemQueue &render, SystemBuilder::Ptr point) -> void;

	auto registerSDFVisualizer(SystemQueue &begin, SystemQueue &render, SystemBuilder::Ptr point) -> void;

}        // namespace maple::sdf