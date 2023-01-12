//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

#include "Engine/Core.h"

namespace maple
{
	class BoundingBox;

	class MAPLE_EXPORT OrientedBoundingBox
	{
	  public:

		OrientedBoundingBox() :
		    OrientedBoundingBox({0, 0, 0}, glm::mat4(1.f))
		{}

		OrientedBoundingBox(const glm::vec3 &extends, const glm::mat4 &transform) :
		    extends(extends), transform(transform)
		{}

		OrientedBoundingBox(const BoundingBox &bb);

		auto setTransform(const glm::mat4 & worldMatrix) -> void;

		auto getSize() const -> glm::vec3;

		inline auto getCenter() const
		{
			return glm::vec3{transform[3][0], transform[3][1], transform[3][2]};
		}

		inline const auto& getTransform() const
		{
			return transform;
		}

		inline auto &getExtends() const
		{
			return extends;
		}

	  private:
		glm::vec3 extends;        //half length;
		glm::mat4 transform;
	};
};        // namespace maple