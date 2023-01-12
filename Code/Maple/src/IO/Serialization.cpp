//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Serialization.h"
#include "IO/File.h"
#include "Engine/Camera.h"
#include "Engine/DDGI/DDGIRenderer.h"
#include "Engine/Mesh.h"
#include "Engine/Raytrace/RaytracedShadow.h"
#include "Engine/Raytrace/RaytracedReflection.h"
#include "Engine/VXGI/Voxelization.h"
#include "Engine/Physics/RigidBody.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Component.h"
#include "Scene/Component/Environment.h"
#include "Scene/Component/Hierarchy.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Engine/DDGI/MeshDistanceField.h"
#include "Engine/AnimationSystem.h"
#include "Scene/Scene.h"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

#include <filesystem>
#include <sstream>

#include "Application.h"
#include "Loader.h"
#include "IoC/SystemBuilder.h"

namespace maple::component 
{
	struct PathIntegrator
	{
		int32_t                    depth = 8;
		int32_t                    maxBounces = 8;
		float                      shadowRayBias = 0.0000;
		SERIALIZATION(depth, maxBounces, shadowRayBias);
	};
}

#define ALL_COMPONENTS Camera,                                                  \
	                   component::Transform,                                    \
	                   component::ActiveComponent,                              \
	                   component::Hierarchy,                                    \
	                   component::Environment,                                  \
	                   component::NameComponent,                                \
	                   component::Light,                                        \
	                   camera_controller::component::CameraControllerComponent, \
	                   component::VolumetricCloud,                              \
	                   component::BoneComponent,                                \
	                   physics::component::RigidBody,                           \
	                   vxgi::component::VoxelVolume,                            \
	                   component::PathIntegrator,                               \
	                   raytraced_shadow::component::RaytracedShadow,            \
	                   raytraced_reflection::component::RaytracedReflection,    \
	                   ddgi::component::IrradianceVolume,                       \
	                   animation::component::SkeletonComponent,                 \
	                   animation::component::Animator,                          \
	                   component::MeshRenderer,                                 \
	                   component::SkinnedMeshRenderer,                          \
	                   sdf::component::MeshDistanceField,                       \
	                   component::BoundingBoxComponent

namespace cereal
{

	template <typename Archive>
	auto save(Archive& archive, const maple::animation::component::SkeletonComponent& skeleton) -> void
	{
		archive(cereal::make_nvp("SkeletonPath", ""));
	}
	template <typename Archive>
	auto load(Archive& archive, maple::animation::component::SkeletonComponent& skeletonComp) -> void
	{
		std::string path;
		archive(cereal::make_nvp("SkeletonPath", path));
	}

	template <typename Archive>
	auto save(Archive &archive, const maple::component::MeshRenderer &m) -> void
	{
		archive(m.active, m.castShadow, m.type, m.meshName, m.filePath);
	}

	template <typename Archive>
	auto save(Archive &archive, const maple::component::SkinnedMeshRenderer &m) -> void
	{
		archive(m.castShadow, m.meshName, m.filePath);
	}

	template <typename Archive>
	auto load(Archive &archive, maple::component::MeshRenderer &m) -> void
	{
		archive(m.active, m.castShadow, m.type, m.meshName, m.filePath);

		if (m.filePath != "")
		{
			auto &resources = maple::io::load(m.filePath);
			using namespace maple;
			for (auto &res : resources)
			{
				if (res->getResourceType() == FileType::Model)
				{
					auto meshes = std::static_pointer_cast<maple::MeshResource>(res);
					m.mesh      = meshes->getMeshes()[m.meshName];
					break;
				}
			}
		}
		else
		{
			switch (m.type)
			{
				case maple::component::PrimitiveType::Quad:
					m.mesh = maple::Mesh::createQuad();
					break;
				case maple::component::PrimitiveType::Cube:
					m.mesh = maple::Mesh::createCube();
					break;
				case maple::component::PrimitiveType::Pyramid:
					m.mesh = maple::Mesh::createPyramid();
					break;
				case maple::component::PrimitiveType::Sphere:
					m.mesh = maple::Mesh::createSphere();
					break;
				case maple::component::PrimitiveType::Capsule:
					m.mesh = maple::Mesh::createCapsule();
					break;
			}
		}
	}

	template <typename Archive>
	auto load(Archive &archive, maple::component::SkinnedMeshRenderer &m) -> void
	{
		archive(m.castShadow, m.meshName, m.filePath);
	}
}        // namespace cereal

namespace maple
{
	auto io::serialize(Scene *scene) -> void
	{
		std::ofstream storage(scene->getPath(), std::ios::binary);
		// output finishes flushing its contents when it goes out of scope
		cereal::JSONOutputArchive output{storage};
		entt::snapshot{
		    Application::getBuilder()->getRegistry()}
		    .entities(output)
		    .component<ALL_COMPONENTS>(output);
	}

	auto io::loadScene(Scene *scene, const std::string &file) -> void
	{
		std::ifstream            istr(file, std::ios::in);
		cereal::JSONInputArchive input(istr);
		entt::snapshot_loader{
		    Application::getBuilder()->getRegistry()}
		    .entities(input)
		    .component<ALL_COMPONENTS>(input)
		    .orphans();
	}

	auto io::loadMaterial(Material *material, const std::string &file) -> void
	{
		File f(file);
		if (f.getFileSize() != 0)
		{
			auto               buffer = f.getBuffer();
			std::istringstream istr;
			istr.str((const char *) buffer.get());
			cereal::JSONInputArchive input(istr);
			input(*material);
		}
	}

	auto io::serialize(Material *material) -> void
	{
		auto              outPath = material->getPath();
		std::stringstream storage;
		{
			// output finishes flushing its contents when it goes out of scope
			cereal::JSONOutputArchive output{storage};
			output(*material);
		}

		File file(outPath, true);
		file.write(storage.str());
	}
};        // namespace maple
