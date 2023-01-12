//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Entity.h"
#include "Others/StringUtils.h"
#include "Scene/System/HierarchyModule.h"
#include "IoC/Registry.h"

namespace maple
{
	auto Entity::isActive() -> bool
	{
		if (hasComponent<component::ActiveComponent>())
			return registry->get<component::ActiveComponent>(entityHandle).active;

		return true;
	}

	auto Entity::setActive(bool isActive) -> void
	{
		getOrAddComponent<component::ActiveComponent>().active = isActive;
	}

	auto Entity::setParent(const Entity &entity) -> void
	{
		bool acceptable         = false;
		auto hierarchyComponent = tryGetComponent<component::Hierarchy>();
		if (hierarchyComponent != nullptr)
		{
			acceptable = entity.entityHandle != entityHandle && (!entity.isParent(*this)) && (hierarchyComponent->parent != entityHandle);
		}
		else
			acceptable = entity.entityHandle != entityHandle;

		if (!acceptable)
			return;

		if (hierarchyComponent)
			hierarchy::reparent(entityHandle, entity.entityHandle, *hierarchyComponent, ioc::Registry{*registry});
		else
		{
			registry->emplace<component::Hierarchy>(entityHandle, entity.entityHandle);
		}
	}

	Entity Entity::findByPath(const std::string &path)
	{
		if (path == "")
		{
			return {};
		}
		Entity ent      = {};
		auto   layers   = StringUtils::split(path, "/");
		auto   children = getChildren();

		for (int32_t i = 0; i < layers.size(); ++i)
		{
			bool findChild = false;

			if (layers[i] == "..")
			{
				ent = getParent();
			}
			else
			{
				for (auto entt : children)
				{
					auto &nameComp = entt.getComponent<component::NameComponent>();
					if (layers[i] == nameComp.name)
					{
						ent      = entt;
						children = ent.getChildren();
						break;
					}
				}
			}
		}
		return ent;
	}

	Entity Entity::getChildInChildren(const std::string &name)
	{
		auto children = getChildren();
		for (auto entt : children)
		{
			auto &nameComp = entt.getComponent<component::NameComponent>();
			if (name == nameComp.name)
			{
				return entt;
			}
			auto ret = entt.getChildInChildren(name);
			if (ret.valid())
			{
				return ret;
			}
		}
		return {};
	}

	Entity Entity::getParent()
	{
		auto hierarchyComp = tryGetComponent<component::Hierarchy>();
		if (hierarchyComp)
			return Entity(hierarchyComp->parent, *registry);
		else
			return {};
	}

	std::vector<Entity> Entity::getChildren()
	{
		std::vector<Entity> children;
		auto                hierarchyComponent = tryGetComponent<component::Hierarchy>();
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->first;
			while (child != entt::null && registry->valid(child))
			{
				children.emplace_back(child, *registry);
				hierarchyComponent = registry->try_get<component::Hierarchy>(child);
				if (hierarchyComponent)
					child = hierarchyComponent->next;
			}
		}
		return children;
	}

	auto Entity::removeAllChildren() -> void
	{
		auto hierarchyComponent = registry->try_get<component::Hierarchy>(entityHandle);
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->first;
			while (child != entt::null)
			{
				auto   hierarchyComponent = registry->try_get<component::Hierarchy>(child);
				auto   next               = hierarchyComponent ? hierarchyComponent->next : entt::null;
				Entity ent                = {child, *registry};
				ent.removeAllChildren();
				ent.destroy();
				child = next;
			}
		}
	}

	bool Entity::isParent(const Entity &potentialParent) const
	{
		auto nodeHierarchyComponent = registry->try_get<component::Hierarchy>(entityHandle);
		if (nodeHierarchyComponent)
		{
			auto parent = nodeHierarchyComponent->parent;
			while (parent != entt::null)
			{
				if (parent == potentialParent.entityHandle)
				{
					return true;
				}
				else
				{
					nodeHierarchyComponent = registry->try_get<component::Hierarchy>(parent);
					parent                 = nodeHierarchyComponent ? nodeHierarchyComponent->parent : entt::null;
				}
			}
		}
		return false;
	}

	auto Entity::destroy() -> void
	{
		registry->destroy(entityHandle);
	}

	auto Entity::valid() -> bool
	{
		return registry && registry->valid(entityHandle);
	}

};        // namespace maple
