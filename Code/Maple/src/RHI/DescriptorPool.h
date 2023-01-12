//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "DescriptorSet.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace maple
{
	struct DescriptorPoolCreateInfo
	{
		uint32_t maxSets;
		std::vector<DescriptorPoolInfo> poolInfos;
	};

	class DescriptorPool
	{
	  public:
		using Ptr = std::shared_ptr<DescriptorPool>;

		virtual ~DescriptorPool() = default;
		static auto create(const DescriptorPoolCreateInfo &desc) -> std::shared_ptr<DescriptorPool>;
	};
}        // namespace maple
