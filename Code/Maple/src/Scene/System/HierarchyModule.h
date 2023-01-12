//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include <entt.hpp>
#include "IoC/Registry.h"

namespace maple
{
	class SystemBuilder;

	namespace component
	{
		struct Hierarchy;
	}

	namespace global::component
	{
		struct SceneTransformChanged;
	}

	namespace hierarchy
	{
		auto MAPLE_EXPORT updateTransform(entt::entity entity, global::component::SceneTransformChanged &transform, ioc::Registry registry) -> void;

		auto MAPLE_EXPORT reset(component::Hierarchy &hy) -> void;
		// Return true if current entity is an ancestor of current entity
		//seems that the entt as a reference could have a bug....
		//TODO
		auto MAPLE_EXPORT compare(component::Hierarchy &left, entt::entity entity, ioc::Registry registry) -> bool;
		//adjust the parent
		auto MAPLE_EXPORT reparent(entt::entity entity, entt::entity parent, component::Hierarchy &hierarchy, ioc::Registry registry) -> void;

		auto MAPLE_EXPORT disconnectOnConstruct(std::shared_ptr<SystemBuilder> builder, bool connect) -> void;

		auto registerHierarchyModule(std::shared_ptr<SystemBuilder> builder) -> void;
	}        // namespace hierarchy
};           // namespace maple
