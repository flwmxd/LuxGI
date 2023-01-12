//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>

namespace maple
{
	class Scene;

	class MAPLE_EXPORT SceneManager final
	{
	  public:
		SceneManager() = default;
		auto switchScene(const std::string &name) -> void;
		auto apply() -> void;

		inline auto getCurrentScene() const
		{
			return currentScene;
		}
		inline auto getSceneCount() const
		{
			return static_cast<uint32_t>(allScenes.size());
		}
		inline auto &getScenes() const
		{
			return allScenes;
		}

		inline auto setSwitchScene(bool switching)
		{
			switchingScenes = switching;
		}
		inline auto isSwitchingScene() const
		{
			return switchingScenes;
		}

		auto getSceneByName(const std::string &sceneName) -> Scene *;

		auto addScene(const std::string &name, Scene *scene,bool addDefaultElement = false) -> void;
		auto addSceneFromFile(const std::string &filePath) -> void;

	  protected:
		Scene *                  currentScene = nullptr;
		std::vector<std::string> sceneFilePaths;
		std::vector<std::string> sceneFilePathsToLoad;

		std::unordered_map<std::string, std::shared_ptr<Scene>> allScenes;

	  private:
		bool        switchingScenes = false;
		std::string currentName;
	};
}        // namespace maple
