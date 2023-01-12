//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Scene.h"
#include "Entity/Entity.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "IoC/SystemBuilder.h"
#include "Scene/SystemBuilder.inl"

#include "IO/Loader.h"

#include "Devices/Input.h"
#include "Engine/Camera.h"
#include "Engine/CameraController.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"

#include "IO/Serialization.h"
#include "Others/StringUtils.h"

#include "Others/Console.h"
#include <filesystem>
#include <fstream>

#include "IoC/Registry.h"

#include "Application.h"

namespace maple
{
	Scene::Scene(const std::string &initName) :
	    name(initName)
	{
	}

	auto Scene::setSize(uint32_t w, uint32_t h) -> void
	{
		width  = w;
		height = h;
	}

	auto Scene::saveTo(const std::string &path, bool binary) -> void
	{
		PROFILE_FUNCTION();
		LOGV("save to disk");

		if (path != "" && path != filePath)
		{
			MAPLE_ASSERT(StringUtils::endWith(path, ".scene"), "should end with .scene");
			filePath = path;
		}

		if (filePath == "")
		{
			filePath = name + ".scene";
		}
		io::serialize(this);
	}

	auto Scene::loadFrom() -> void
	{
		PROFILE_FUNCTION();
		if (filePath != "")
		{
			//Application::getBuilder()->clear();
			hierarchy::disconnectOnConstruct(Application::getBuilder(), false);
			io::loadScene(this, filePath);
			hierarchy::disconnectOnConstruct(Application::getBuilder(), true);
		}
	}

	auto Scene::createEntity() -> Entity
	{
		PROFILE_FUNCTION();
		dirty       = true;
		auto entity = Application::getBuilder()->create();
		if (onEntityAdd)
			onEntityAdd(entity);
		return entity;
	}

	auto Scene::createEntity(const std::string &name) -> Entity
	{
		PROFILE_FUNCTION();
		dirty          = true;
		int32_t i      = 0;
		auto    entity = Application::getBuilder()->getEntityByName(name);
		while (entity.valid())
		{
			entity = Application::getBuilder()->getEntityByName(name + "(" + std::to_string(i + 1) + ")");
			i++;
		}
		auto newEntity = Application::getBuilder()->create(i == 0 ? name : name + "(" + std::to_string(i) + ")");
		if (onEntityAdd)
			onEntityAdd(newEntity);
		return newEntity;
	}

	auto Scene::duplicateEntity(const Entity &entity, const Entity &parent) -> void
	{
		PROFILE_FUNCTION();
		dirty = true;

		Entity newEntity = Application::getBuilder()->create();

		if (parent)
			newEntity.setParent(parent);

		copyComponents(entity, newEntity);
	}

	auto Scene::duplicateEntity(const Entity &entity) -> void
	{
		PROFILE_FUNCTION();

		dirty            = true;
		Entity newEntity = Application::getBuilder()->create();
		//COPY
		copyComponents(entity, newEntity);
	}

	auto Scene::getCamera() -> std::pair<Camera *, component::Transform *>
	{
		PROFILE_FUNCTION();

		auto query = Application::getBuilder()->getRegistry().view<Camera, component::Transform>();

		if (useSceneCamera)
		{
			if (Application::get()->getEditorState() == EditorState::Play)
			{
				for (auto entity : query.each())
				{
					auto [_, sceneCam, sceneCamTr] = entity;
					return {&sceneCam, &sceneCamTr};
				}
			}
		}
		return {overrideCamera, overrideTransform};
	}

	auto Scene::removeAllChildren(entt::entity entity) -> void
	{
		PROFILE_FUNCTION();
		Application::getBuilder()->removeAllChildren(entity);
	}

	auto Scene::calculateBoundingBox() -> void
	{
		PROFILE_FUNCTION();
	}

	auto Scene::onMeshRenderCreated() -> void
	{
		boxDirty = true;
	}

	auto Scene::addMesh(const std::string &file) -> Entity
	{
		PROFILE_FUNCTION();
		auto name        = StringUtils::getFileNameWithoutExtension(file);
		auto modelEntity = createEntity(name);

		auto &resources = io::load(file);

		auto skeletonIter = std::find_if(resources.begin(), resources.end(), [](auto &res) {
			return res->getResourceType() == FileType::Skeleton;
		});

		for (auto &res : resources)
		{
			if (res->getResourceType() == FileType::Model)
			{
				for (auto mesh : std::static_pointer_cast<MeshResource>(res)->getMeshes())
				{
					auto child = createEntity(mesh.first);
					auto& meshRenderer = child.addComponent<component::MeshRenderer>();
					meshRenderer.type = component::PrimitiveType::File;
					meshRenderer.mesh = mesh.second;
					meshRenderer.meshName = mesh.first;
					meshRenderer.filePath = file;
					child.setParent(modelEntity);
				}
			}
		}
		return modelEntity;
	}

	auto Scene::copyComponents(const Entity &from, const Entity &to) -> void
	{
		LOGW("Not implementation {0}", __FUNCTION__);
	}

	auto Scene::onInit() -> void
	{
		PROFILE_FUNCTION();
		if (initCallback != nullptr)
		{
			initCallback(this);
		}
	}

	auto Scene::onClean() -> void
	{
		Application::getBuilder()->clear();
	}

	auto Scene::updateCameraController(float dt) -> void
	{
		PROFILE_FUNCTION();
	}

	auto Scene::onUpdate(float dt) -> void
	{
		PROFILE_FUNCTION();
		auto &deltaTime = Application::getBuilder()->getGlobalComponent<global::component::DeltaTime>();
		deltaTime.dt    = dt;
		updateCameraController(dt);
		getBoundingBox();
	}
};           // namespace maple
