//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <memory>

namespace maple
{
	namespace physics
	{
		class PhysicsWorld
		{
		public:
			using Ptr = std::shared_ptr<PhysicsWorld>;
		};

		namespace global::component
		{
			struct PhysicsWorld
			{
				std::shared_ptr<physics::PhysicsWorld> world;
			};
		};        // namespace global::component
	}             // namespace physics
}        // namespace maple