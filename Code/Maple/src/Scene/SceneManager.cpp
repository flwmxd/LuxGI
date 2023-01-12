
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "SceneManager.h"
#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "Entity/Entity.h"
#include "Others/StringUtils.h"
#include "Scene/Scene.h"

#include "Scene/Component/Environment.h"
#include "Scene/Component/Light.h"
#include "IoC/SystemBuilder.h"

namespace maple
{
	auto SceneManager::switchScene(const std::string &filePath) -> void
	{
		switchingScenes = true;
		auto name       = StringUtils::getFileName(filePath);
		if (auto iter = allScenes.find(name); iter != allScenes.end())
		{
			switchingScenes = true;
			currentName     = name;
		}
		else
		{
			switchingScenes = false;
			LOGW("[{0} : {1}] - Unknown Scene : {2}", __FILE__, __LINE__, name);
		}
	}

	auto SceneManager::apply() -> void
	{
		PROFILE_FUNCTION();
		if (!switchingScenes)
		{
			if (currentScene != nullptr)
				return;
			if (allScenes.empty())
			{
				currentName       = "default";
				auto defaultScene = new Scene(currentName);
				addScene(currentName, defaultScene,true);
			}
		}

		//switching to new scene
		if (currentScene != nullptr)        //clear before
		{
			currentScene->onClean();
		}

		currentScene = allScenes[currentName].get();

		currentScene->loadFrom();

		if (Application::get()->getEditorState() == EditorState::Play)
		{
			currentScene->onInit();
		}

		Application::get()->onSceneCreated(currentScene);

		switchingScenes = false;
	}

	auto SceneManager::addSceneFromFile(const std::string &filePath) -> void
	{
		auto name = StringUtils::getFileName(filePath);
		if (auto iter = allScenes.find(name); iter == allScenes.end())
		{
			auto scene = new Scene(name);
			scene->setPath(filePath);
			addScene(name, scene, false);
		}
	}

	Scene *SceneManager::getSceneByName(const std::string &sceneName)
	{
		if (auto iter = allScenes.find(sceneName); iter != allScenes.end())
		{
			return iter->second.get();
		}
		return nullptr;
	}

	auto SceneManager::addScene(const std::string &name, Scene *scene, bool addDefaultElement) -> void
	{
		allScenes[name] = std::shared_ptr<Scene>(scene);
	}
};        // namespace maple
