
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DescriptorSet.h"

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/VulkanDescriptorSet.h"
#endif        // MAPLE_VULKAN

#ifdef MAPLE_OPENGL
#	include "RHI/OpenGL/GLDescriptorSet.h"
#endif

namespace maple
{
	std::unordered_multimap<uint32_t, std::shared_ptr<DescriptorSet>> DescriptorSet::setCache;

	namespace
	{
		inline auto isSubLayout(const LayoutBings &desc, const LayoutBings &parent)
		{
			for (auto &l : desc.layouts)
			{
				if (auto iter = std::find_if(parent.layouts.begin(), parent.layouts.end(), [&](const auto & other) {
					    return l.name == other.name && l.type == other.type;
				}); iter == parent.layouts.end())
				{
					return false;
				}
			}
			return true;
		}
	}        // namespace

	auto DescriptorSet::createWithLayout(const LayoutBings &desc) -> std::shared_ptr<DescriptorSet>
	{
		auto range = setCache.equal_range(desc.binding); 
#ifdef MAPLE_VULKAN
		for (auto iter = range.first;iter != range.second;iter++)
		{
			if (isSubLayout(desc, iter->second->getLayoutBings()))
			{
				return std::make_shared<VulkanDescriptorSet>(desc);
			}
		}
		return setCache.emplace(
		    std::piecewise_construct,
		    std::forward_as_tuple(desc.binding),
			std::forward_as_tuple(new VulkanDescriptorSet(desc)))->second;
#endif

#ifdef MAPLE_OPENGL
		return nullptr;
#endif
	}

}        // namespace maple
