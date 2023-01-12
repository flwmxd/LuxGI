//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace maple
{
	Camera::Camera(float fov, float near, float far, float aspect) :
	    aspectRatio(aspect),
	    projectionDirty(true),
	    fov(fov),
	    near_(near),
	    far_(far),
	    orthographic(false)
	{
	}

	Camera::Camera(float aspectRatio, float scale) :
	    aspectRatio(aspectRatio),
	    scale(scale),
	    projectionDirty(true),
	    fov(0),
	    near_(0.1),
	    far_(3200000.0f),
	    orthographic(true)
	{
	}

	Camera::Camera(float pitch, float yaw, const glm::vec3 &position, float fov, float near, float far, float aspect) :
	    aspectRatio(aspect),
	    projectionDirty(true),
	    fov(fov),
	    near_(near),
	    far_(far),
	    orthographic(false)
	{
	}

	auto Camera::getProjectionMatrix() -> const glm::mat4 &
	{
		if (projectionDirty)
		{
			projMatrixOld = projMatrix;
			updateProjectionMatrix();
			projectionDirty = false;
		}
		return projMatrix;
	}

	auto Camera::setProjectionMatrix(const glm::mat4 &projection) -> void
	{
		projMatrix = projection;
		projMatrixOld = projMatrix;
	}

	auto Camera::getFrustum(const glm::mat4 &viewMatrix) -> const Frustum &
	{
		if (projectionDirty)
		{
			updateProjectionMatrix();
			projectionDirty = false;
		}
		frustum.from(projMatrix * viewMatrix);
		return frustum;
	}

	auto Camera::updateProjectionMatrix() -> void
	{
		if (orthographic)
			projMatrix = glm::ortho(aspectRatio * -scale, aspectRatio * scale, -scale, scale, near_, far_);
		else
			projMatrix = glm::perspective(glm::radians(fov), aspectRatio, near_, far_);
	}

};        // namespace maple
