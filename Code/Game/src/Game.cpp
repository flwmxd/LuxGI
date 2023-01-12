//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Main.h"
#include "Application.h"
#include "IO/File.h"
#include "Scene/Component/Transform.h"
#include "Engine/CameraController.h"
#include "Engine/Camera.h"
#include "Scene/Scene.h"
#include "Devices/Input.h"
#include <memory>

namespace maple
{
	class Game : public Application
	{
	public:
		Game() : Application(new DefaultDelegate()) {}
		auto init() -> void override
		{
			Application::init();
			if (File::fileExists("dark-room-emissive.scene")) {
				sceneManager->addSceneFromFile("dark-room-emissive.scene");
				sceneManager->switchScene("dark-room-emissive.scene");
			}

			auto winSize = window->getWidth() / (float)window->getHeight();

			camera = std::make_unique<Camera>(
				60, 0.1, 3000, winSize);
			editorCameraController.setCamera(camera.get());
		};

		auto onUpdate(const Timestep& delta) -> void override
		{
			Application::onUpdate(delta);
			editorCameraTransform.setWorldMatrix(glm::mat4(1.f));
			const auto mousePos = Input::getInput()->getMousePosition();
			editorCameraController.handleMouse(editorCameraTransform, delta, mousePos.x, mousePos.y);
			editorCameraController.handleKeyboard(editorCameraTransform, delta);
			if (auto currentScene = getSceneManager()->getCurrentScene())
			{
				currentScene->setOverrideCamera(camera.get());
				currentScene->setOverrideTransform(&editorCameraTransform);
			}
		}

	private:
		std::unique_ptr<Camera> camera;
		component::Transform    editorCameraTransform;
		EditorCameraController  editorCameraController;
	};
};

maple::Application* createApplication()
{
	return new maple::Game();
}