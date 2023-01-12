//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Scene/System/MeshModule.h"
#include "Scene/Component/Transform.h"
#include "IoC/SystemBuilder.h"
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include "IO/Loader.h"
#include "Scene/Component/MeshRenderer.h"

namespace maple
{
	namespace mesh
	{
		namespace on_game_end
		{
			inline auto system(global::component::SceneAABB &aabb)
			{
				aabb.aabb.clear();
			}
		}        // namespace on_game_end

		namespace on_game_start
		{
			inline auto system(ioc::Registry registry, global::component::SceneAABB &aabb)
			{
				aabb.aabb.clear();
				registry.getRegistry().view<component::MeshRenderer>().each([&](component::MeshRenderer &meshRender) {
					if (meshRender.mesh)
					{
						aabb.aabb.merge(meshRender.mesh->getBoundingBox());
					}
				});
			}
		}        // namespace on_game_start

		auto registerMeshModule(std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<global::component::SceneAABB>();
			builder->registerGameEnded<on_game_end::system>();
			builder->registerGameStart<on_game_start::system>();
		}
	}        // namespace mesh
};           // namespace maple
