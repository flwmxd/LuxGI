//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "AccelerationStructure.h"
#include "Engine/Mesh.h"
#include "Engine/Raytrace/RaytraceConfig.h"
#include "Engine/Renderer/RendererData.h"
#include "RHI/AccelerationStructure.h"
#include "RHI/GraphicsContext.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

namespace maple
{
	namespace
	{
		constexpr uint32_t MAX_SCENE_MESH_INSTANCE_COUNT = 2048;
	}

	namespace raytracing
	{
		namespace update
		{
			inline auto system(global::component::TopLevelAs& topLevel,
				const maple::component::RendererData& renderData,
				maple::global::component::SceneTransformChanged* sceneChanged,
				maple::global::component::Bindless& bindless,
				const maple::global::component::GraphicsContext& context,
				const trace::global::component::RaytraceConfig& config,
				ioc::Registry registry)
			{
				if (!context.context->isRaytracingSupported() || trace::isSoftTrace(config))
					return;

				auto meshGroup = registry.getRegistry().view<component::MeshRenderer, component::Transform>();

				if (topLevel.topLevelAs == nullptr)
				{
					if (meshGroup.begin() != meshGroup.end())
					{
						topLevel.topLevelAs = AccelerationStructure::createTopLevel(MAX_SCENE_MESH_INSTANCE_COUNT);
						auto     tasks = BatchTask::create();
						uint32_t meshCount = 0;

						for (auto [meshEntity , mesh, transform]: meshGroup.each())
						{
							auto blas = mesh.mesh->getAccelerationStructure(tasks);
							bindless.meshIndices[(uint32_t)meshEntity] = meshCount;
							topLevel.topLevelAs->updateTLAS(transform.getWorldMatrix(), meshCount++, blas->getDeviceAddress());
						}
						if (meshCount > 0)
						{
							topLevel.topLevelAs->copyToGPU(renderData.commandBuffer, meshCount, 0);
							topLevel.topLevelAs->build(renderData.commandBuffer, meshCount);
							tasks->execute();
						}
					}
				}
				else if (sceneChanged && sceneChanged->dirty)
				{
					auto     tasks = BatchTask::create();
					uint32_t meshCount = 0;
					for (auto [meshEntity, mesh, transform] : meshGroup.each())
					{
						auto blas = mesh.mesh->getAccelerationStructure(tasks);
						bindless.meshIndices[(uint32_t)meshEntity] = meshCount;
						topLevel.topLevelAs->updateTLAS(transform.getWorldMatrix(), meshCount++, blas->getDeviceAddress());
					}
					if (meshCount > 0)
					{
						topLevel.topLevelAs->copyToGPU(renderData.commandBuffer, meshCount, 0);
						topLevel.topLevelAs->build(renderData.commandBuffer, meshCount);
						tasks->execute();
					}
				}
			}
		}        // namespace update

		auto registerAccelerationStructureModule(SystemQueue& begin, std::shared_ptr<SystemBuilder> builder) -> void
		{
#ifndef MAPLE_OPENGL
			builder->registerGlobalComponent<global::component::TopLevelAs>();
			builder->registerWithinQueue<update::system>(begin);
#endif        // !MAPLE_OPENGL
		}
	}        // namespace raytracing
}        // namespace maple