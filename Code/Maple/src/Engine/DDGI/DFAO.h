//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Math/OrientedBoundingBox.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Texture.h"
#include "RHI/Shader.h"
#include "RHI/StorageBuffer.h"
#include "IoC/SystemBuilder.h"

namespace maple::sdf::ao
{
	namespace global::component
	{
		struct DFAO
		{
			Shader::Ptr shader;
			DescriptorSet::Ptr sets;
			Texture2D::Ptr outColor;
			int32_t step = 2;
			float dist = 1.8;
		};
	}

	auto registerSDFAO(SystemQueue& begin, SystemQueue& render, std::shared_ptr<SystemBuilder> point) -> void;
}