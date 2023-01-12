//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "VulkanHelper.h"

namespace maple
{
	class VulkanFence
	{
	  public:
		VulkanFence(bool createSignaled = false);
		~VulkanFence();
		NO_COPYABLE(VulkanFence);
		auto checkState() -> bool;
		auto isSignaled() -> bool;
		auto wait() -> bool;
		auto reset() -> void;
		auto waitAndReset() -> void;

		inline auto setSignaled(bool signaled)
		{
			this->signaled = signaled;
		}

		inline auto getHandle()
		{
			return handle;
		}

		autoUnpack(handle);

	  private:
		VkFence handle;
		bool    signaled;
	};
}        // namespace maple
