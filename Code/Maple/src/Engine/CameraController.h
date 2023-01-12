//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/Component/Transform.h"
#include <glm/glm.hpp>

namespace maple
{
	class Camera;

	class MAPLE_EXPORT CameraController
	{
	  public:
		CameraController()          = default;
		virtual ~CameraController() = default;

		virtual auto handleMouse(component::Transform &transform, float dt, float xpos, float ypos) -> void{};
		virtual auto handleKeyboard(component::Transform &transform, float dt) -> void{};
		virtual auto updateScroll(component::Transform &transform, float offset, float dt) -> void{};
		virtual auto onImGui() -> void{};
		inline auto  setMouseSensitivity(float value)
		{
			mouseSensitivity = value;
		}
		inline auto &getVelocity() const
		{
			return velocity;
		}

	  protected:
		glm::vec3 velocity       = {0, 0, 0};
		glm::vec2 rotateVelocity = {0, 0};
		glm::vec3 focalPoint     = {0, 0, 0};

		float zoomVelocity = 0.0f;
		float cameraSpeed  = 0.0f;
		float distance     = 0.0f;
		float zoom         = 1.0f;

		glm::vec2 projectionOffset  = glm::vec2(0.0f, 0.0f);
		glm::vec2 previousCurserPos = glm::vec2(0.0f, 0.0f);

		float mouseSensitivity      = 0.1f;
		float zoomDampeningFactor   = 0.00001f;
		float dampeningFactor       = 0.00001f;
		float rotateDampeningFactor = 0.001f;
	};

	class MAPLE_EXPORT EditorCameraController : public CameraController
	{
	  public:
		EditorCameraController();
		~EditorCameraController();

		auto        handleMouse(component::Transform &transform, float dt, float xpos, float ypos) -> void override;
		auto        handleKeyboard(component::Transform &transform, float dt) -> void override;
		auto        updateScroll(component::Transform &transform, float offset, float dt) -> void override;
		inline auto setTwoDMode(bool _2d)
		{
			_2DMode = _2d;
		}
		inline auto setCamera(maple::Camera *val)
		{
			camera = val;
		}

	  private:
		bool    _2DMode = false;
		Camera *camera  = nullptr;
	};
}        // namespace maple
