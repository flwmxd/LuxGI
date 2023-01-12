//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace maple
{
	namespace component
	{
		auto Transform::getForwardDirection() const -> glm::vec3
		{
			return getWorldOrientation() * FORWARD;
		}

		auto Transform::getRightDirection() const -> glm::vec3
		{
			return getWorldOrientation() * RIGHT;
		}

		auto Transform::getUpDirection() const -> glm::vec3
		{
			return getWorldOrientation() * UP;
		}

		Transform::Transform()
		{
			localPosition    = {0.0f, 0.0f, 0.0f};
			localOrientation = {};
			localScale       = {1.0f, 1.0f, 1.0f};
			localMatrix      = glm::mat4(1.f);
			worldMatrix      = glm::mat4(1.f);

			initLocalPosition    = localPosition;
			initLocalScale       = localScale;
			initLocalOrientation = localOrientation;
		}

		Transform::Transform(const glm::mat4 &matrix)
		{
			glm::vec3 skew;
			glm::quat rotation;
			glm::vec4 perspective;
			glm::decompose(matrix, localScale, rotation, localPosition, skew, perspective);
			localOrientation   = glm::eulerAngles(rotation);
			localMatrix        = matrix;
			worldMatrix        = matrix;
			worldMatrixInverse = glm::inverse(worldMatrix);

			initLocalPosition    = localPosition;
			initLocalScale       = localScale;
			initLocalOrientation = localOrientation;
		}

		Transform::Transform(const glm::vec3 &position)
		{
			localPosition    = position;
			localOrientation = {};
			localScale       = {1.0f, 1.0f, 1.0f};
			localMatrix      = glm::mat4(1.f);
			worldMatrix      = glm::mat4(1.f);
			setLocalPosition(position);

			initLocalPosition    = localPosition;
			initLocalScale       = localScale;
			initLocalOrientation = localOrientation;
		}

		Transform::~Transform() = default;

		auto Transform::setWorldMatrix(const glm::mat4 &mat) -> void
		{
			if (dirty)
				updateLocalMatrix();
			worldMatrix        = mat * localMatrix;
			worldMatrixInverse = glm::inverse(worldMatrix);
		}

		auto Transform::setLocalTransform(const glm::mat4 &localMat) -> void
		{
			localMatrix = localMat;
			dirty       = true;
			applyTransform();        //decompose
		}

		auto Transform::setLocalPosition(const glm::vec3 &localPos) -> void
		{
			dirty         = true;
			localPosition = localPos;
		}

		auto Transform::setLocalScale(const glm::vec3 &scale) -> void
		{
			dirty      = true;
			localScale = scale;
		}

		//void Transform::SetLocalOrientation(const glm::quat& quat)
		auto Transform::setLocalOrientation(const glm::vec3 &rotation) -> void
		{
			dirty            = true;
			localOrientation = rotation;
		}

		auto Transform::setLocalOrientation(const glm::quat &rotation) -> void
		{
			dirty            = true;
			localOrientation = glm::eulerAngles(rotation);
		}

		auto Transform::lookAt(const glm::vec3 &target) -> void
		{
			dirty       = true;
			localMatrix = glm::lookAt(localPosition, target, getUpDirection());
			applyTransform();
		}

		auto Transform::updateLocalMatrix() -> void
		{
			localMatrix = glm::translate(glm::mat4(1), localPosition);
			localMatrix *= glm::toMat4(glm::quat(localOrientation));
			localMatrix = glm::scale(localMatrix, localScale);
			dirty       = false;
			hasUpdate   = true;
		}

		auto Transform::applyTransform() -> void
		{
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::quat rotation;
			glm::decompose(localMatrix, localScale, rotation, localPosition, skew, perspective);
			localOrientation = glm::eulerAngles(rotation);
		}

		auto Transform::getScaleFromMatrix(const glm::mat4 &mat) -> glm::vec3
		{
			glm::vec3 skew;
			glm::vec3 localScale;
			glm::quat localOrientation;
			glm::vec3 localPosition;
			glm::vec4 perspective;
			glm::decompose(mat, localScale, localOrientation, localPosition, skew, perspective);
			return localScale;
		}

		auto Transform::createMatrix(const glm::vec3 &pos, const glm::quat &rotation) -> glm::mat4
		{
			auto localMatrix = glm::translate(glm::mat4(1), pos);
			localMatrix *= glm::toMat4(rotation);
			return localMatrix;
		}

		auto Transform::localToWorldVector(const glm::mat4 &matrix, const glm::vec3 &vec) -> glm::vec3
		{
			auto world = matrix;
			world[3]   = {0, 0, 0, 1};
			return world * glm::vec4(vec, 1);
		}

		auto Transform::setOffsetTransform(const glm::mat4 &localMat) -> void
		{
			offsetMatrix = localMat;
		}

		auto Transform::resetTransform() -> void
		{
			dirty            = true;
			localPosition    = initLocalPosition;
			localScale       = initLocalScale;
			localOrientation = initLocalOrientation;
		}

		auto Transform::saveTransform() -> void
		{
			initLocalPosition    = localPosition;
			initLocalScale       = localScale;
			initLocalOrientation = localOrientation;
		}
	};        // namespace component
};            // namespace maple
