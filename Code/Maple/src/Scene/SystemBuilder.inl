//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/HierarchyModule.h"
#include "Engine/Renderer/BindlessModule.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Noise/BlueNoise.h"
#include "System/TransformModule.h"
#include "System/MeshModule.h"

namespace maple
{
	inline auto registerSystem(std::shared_ptr<SystemBuilder> builder)
	{
		transform::registerTransformModule(builder);
		blue_noise::registerBlueNoiseModule(builder);
		mesh::registerMeshModule(builder);
		bindless::registerBindless(builder);
		hierarchy::registerHierarchyModule(builder);
	}
}        // namespace maple
