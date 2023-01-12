//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Loader.h"
#include <string>
#include <unordered_map>

namespace maple
{
	class Mesh;
	namespace io
	{
		class OBJLoader : public AssetsArchive
		{
		  public:
			static constexpr char *EXTENSIONS[] = {"obj"};
			auto                   load(const std::string &fileName, const std::string &extension, std::vector<std::shared_ptr<IResource>> &out) const -> void override;
		};
	}
};        // namespace maple
