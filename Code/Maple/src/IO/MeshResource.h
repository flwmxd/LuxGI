//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "IO/IResource.h"
#include <string>
#include <unordered_map>

namespace maple
{
	class Mesh;
	class Skeleton;

	class MAPLE_EXPORT MeshResource : public IResource
	{
	  public:
		MeshResource(const std::string &name);
		auto         addMesh(const std::string &name, Mesh *mesh) -> void;
		auto         addMesh(const std::string &name, std::shared_ptr<Mesh> mesh) -> void;
		auto         find(const std::string &name) -> std::shared_ptr<Mesh>;
		inline auto &getMeshes() const
		{
			return meshes;
		}
		inline auto &getMeshes()
		{
			return meshes;
		}

		auto getResourceType() const -> FileType override
		{
			return FileType::Model;
		};
		auto getPath() const -> std::string override
		{
			return name;
		};

	  private:
		std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
		std::string                                            name;
	};
};        // namespace maple
