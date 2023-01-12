//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "MeshResource.h"
#include "IO/Loader.h"

namespace maple
{
	MeshResource::MeshResource(const std::string &name) :
	    name(name)
	{
	}

	auto MeshResource::addMesh(const std::string &name, Mesh *mesh) -> void
	{
		meshes.emplace(name, std::shared_ptr<Mesh>(mesh));
	}

	auto MeshResource::addMesh(const std::string &name, std::shared_ptr<Mesh> mesh) -> void
	{
		meshes.emplace(name, mesh);
	}
	auto MeshResource::find(const std::string &name) -> std::shared_ptr<Mesh>
	{
		if (auto iter = meshes.find(name); iter != meshes.end())
		{
			return iter->second;
		}
		return nullptr;
	}
};        // namespace maple
