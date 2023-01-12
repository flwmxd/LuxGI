//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/GraphicsContext.h"

#if defined(RENDER_API_VULKAN)
#	include "RHI/Vulkan/VKCommandBuffer.h"
#	include "RHI/Vulkan/VKDevice.h"
#endif

#if defined(PROFILE_GPU) && defined(RENDER_API_VULKAN) && defined(MAPLE_PROFILE) && defined(TRACY_ENABLE)
#	define GPUProfile(name)
#else
#	define GPUProfile(name)
#endif
