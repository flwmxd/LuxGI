//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "CameraController.h"
#include "Camera.h"
#include "Devices/Input.h"
#include "Math/MathUtils.h"

namespace maple
{
	static constexpr float Multiplier = 200;

	EditorCameraController::EditorCameraController()
	{
		focalPoint            = maple::ZERO;
		velocity              = glm::vec3(0.0f);
		mouseSensitivity      = 0.00001f;
		zoomDampeningFactor   = 0.00001f;
		dampeningFactor       = 0.00001f;
		rotateDampeningFactor = 0.0000001f;
	}

	EditorCameraController::~EditorCameraController()
	{
	}

	auto EditorCameraController::handleMouse(component::Transform &transform, float dt, float xpos, float ypos) -> void
	{
		if (_2DMode)
		{
			if (Input::getInput()->isMouseHeld(KeyCode::ButtonRight))
			{
				mouseSensitivity = 1.f;
				auto position    = transform.getLocalPosition();
				position.x -= (xpos - previousCurserPos.x) * mouseSensitivity * 0.5f;
				position.y += (ypos - previousCurserPos.y) * mouseSensitivity * 0.5f;
				transform.setLocalPosition(position);
			}
		}
		else
		{
			if (Input::getInput()->isMouseHeld(KeyCode::MouseKey::ButtonRight))
			{
				mouseSensitivity = 0.01f;
				rotateVelocity   = rotateVelocity + (glm::vec2{xpos, ypos} - previousCurserPos) * mouseSensitivity;
			}

			glm::vec3 euler = glm::degrees(transform.getLocalOrientation());
			float     pitch = euler.x - rotateVelocity.y;
			float     yaw   = euler.y - rotateVelocity.x;

			pitch = std::min(pitch, 89.0f);
			pitch = std::max(pitch, -89.0f);

			transform.setLocalOrientation(glm::radians(glm::vec3{pitch, yaw, 0.0f}));
		}

		previousCurserPos = glm::vec2(xpos, ypos);
		rotateVelocity    = rotateVelocity * std::pow(rotateDampeningFactor, dt);
		updateScroll(transform, Input::getInput()->getScrollOffset(), dt);
	}

	auto EditorCameraController::handleKeyboard(component::Transform &transform, float dt) -> void
	{
		if (_2DMode)
		{
			glm::vec3 up(0, 1, 0);
			glm::vec3 right(1, 0, 0);

			cameraSpeed = dt * 20.0f;

			if (Input::getInput()->isKeyHeld(KeyCode::Id::A))
			{
				velocity -= right * cameraSpeed;
			}

			if (Input::getInput()->isKeyHeld(KeyCode::Id::D))
			{
				velocity += right * cameraSpeed;
			}

			if (Input::getInput()->isKeyHeld(KeyCode::Id::W))
			{
				velocity += up * cameraSpeed;
			}

			if (Input::getInput()->isKeyHeld(KeyCode::Id::S))
			{
				velocity -= up * cameraSpeed;
			}

			if (!math::equals(velocity, glm::vec3(0)))
			{
				auto position = transform.getLocalPosition();
				position += velocity * dt;
				velocity = velocity * pow(dampeningFactor, dt);
				transform.setLocalPosition(position);
			}
		}
		else
		{
			float multiplier = 100.0f;

			if (Input::getInput()->isKeyHeld(KeyCode::Id::LeftShift))
			{
				multiplier = Multiplier * 10.f;
			}

			cameraSpeed = multiplier * dt;

			if (Input::getInput()->isMouseHeld(KeyCode::MouseKey::ButtonRight))
			{
				if (Input::getInput()->isKeyHeld(KeyCode::Id::W))
				{
					velocity -= transform.getForwardDirection() * cameraSpeed;
				}

				if (Input::getInput()->isKeyHeld(KeyCode::Id::S))
				{
					velocity += transform.getForwardDirection() * cameraSpeed;
				}

				if (Input::getInput()->isKeyHeld(KeyCode::Id::A))
				{
					velocity -= transform.getRightDirection() * cameraSpeed;
				}

				if (Input::getInput()->isKeyHeld(KeyCode::Id::D))
				{
					velocity += transform.getRightDirection() * cameraSpeed;
				}

				if (Input::getInput()->isKeyHeld(KeyCode::Id::Q))
				{
					velocity -= transform.getUpDirection() * cameraSpeed;
				}

				if (Input::getInput()->isKeyHeld(KeyCode::Id::E))
				{
					velocity += transform.getUpDirection() * cameraSpeed;
				}
			}

			if (!math::equals(velocity, maple::ZERO))
			{
				glm::vec3 position = transform.getLocalPosition();
				position += velocity * dt;
				transform.setLocalPosition(position);
				velocity = velocity * std::pow(dampeningFactor, dt);
			}
		}
	}

	auto EditorCameraController::updateScroll(component::Transform &transform, float offset, float dt) -> void
	{
		if (_2DMode)
		{
			float multiplier = 100.0f;
			if (Input::getInput()->isKeyHeld(KeyCode::Id::LeftShift))
			{
				multiplier = Multiplier;
			}

			if (offset != 0.0f)
			{
				zoomVelocity += dt * offset * multiplier;
			}

			if (!math::equals(zoomVelocity, 0.0f))
			{
				float scale = camera->getScale();

				scale -= zoomVelocity;

				if (scale < 0.15f)
				{
					scale        = 0.15f;
					zoomVelocity = 0.0f;
				}
				else
				{
					zoomVelocity = zoomVelocity * pow(zoomDampeningFactor, dt);
				}
				camera->setScale(scale);
			}
		}
		else
		{
			if (offset != 0.0f)
			{
				zoomVelocity -= dt * offset * 10.0f;
			}

			if (!math::equals(zoomVelocity, 0.0f))
			{
				glm::vec3 pos = transform.getLocalPosition();
				pos += transform.getForwardDirection() * zoomVelocity;
				zoomVelocity = zoomVelocity * std::pow(zoomDampeningFactor, dt);

				transform.setLocalPosition(pos);
			}
		}
	}
};        // namespace maple
