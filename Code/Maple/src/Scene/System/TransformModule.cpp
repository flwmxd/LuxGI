//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Scene/System/TransformModule.h"
#include "Scene/Component/Transform.h"
#include "IoC/SystemBuilder.h"

namespace maple
{
	namespace transform
	{
		namespace on_game_end
		{
			inline auto system(ioc::Registry registry)
			{
				for (auto [entity, transform] : registry.getRegistry().view<maple::component::Transform>().each())
				{
					transform.resetTransform();
				}
			}
		}        // namespace on_game_end

		namespace on_game_start
		{
			inline auto system(ioc::Registry registry)
			{
				for (auto [entity, transform] : registry.getRegistry().view<maple::component::Transform>().each())
					transform.saveTransform();
			}
		}        // namespace on_game_start

		auto registerTransformModule(std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGameEnded<on_game_end::system>();
			builder->registerGameStart<on_game_start::system>();
		}
	}        // namespace transform
};           // namespace maple
