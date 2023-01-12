//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Plane.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
namespace maple
{
	class BoundingBox;

	class MAPLE_EXPORT Frustum
	{
	  public:
		enum FrustumPlane
		{
			PlaneNear = 0,
			PlaneLeft,
			PlaneRight,
			PlaneUp,
			PlaneDown,
			PlaneFar
		};

		static constexpr uint32_t FRUSTUM_VERTICES = 8;

		Frustum() noexcept = default;

		auto from(const glm::mat4 &projection) -> void;

		auto isInside(const glm::vec3 &pos) const -> bool;
		auto isInside(const BoundingBox &box) const -> bool;
		auto isInside(const std::shared_ptr<BoundingBox> &box) const -> bool;

		inline auto &getPlane(FrustumPlane id) const
		{
			return planes[id];
		}

		inline auto &getPlane(int32_t i) const
		{
			return planes[static_cast<FrustumPlane>(i)];
		}

		inline auto getVertices() const
		{
			return vertices;
		}

		inline auto getVertices()
		{
			return vertices;
		}

	  private:
		Plane     planes[6];
		glm::vec3 vertices[FRUSTUM_VERTICES];
	};

};        // namespace maple
