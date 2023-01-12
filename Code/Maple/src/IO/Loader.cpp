//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Loader.h"
#include "FBXLoader.h"
#include "GLTFLoader.h"
#include "OBJLoader.h"
#include "Others/Console.h"
#include "Others/StringUtils.h"

namespace maple
{
	namespace io
	{
		std::unordered_map<std::string, std::shared_ptr<AssetsArchive>>           loaders;
		std::unordered_map<std::string, std::vector<std::shared_ptr<IResource>>> cache;
		std::unordered_set<std::string>                                          supportExtensions;
		std::vector<std::shared_ptr<IResource>>                                  nullOut;

		auto load(const std::string &obj) -> std::vector<std::shared_ptr<IResource>>&
		{
			auto extension = StringUtils::getExtension(obj);
			auto loader    = loaders.find(extension);
			if (loader == loaders.end())
			{
				LOGE("Unknown file extension {0}, fileName : {1}", extension,obj);
			}
			else
			{
				if (auto iter = cache.find(obj); iter != cache.end())
				{
					return iter->second;
				}
				else
				{
					auto &out = cache[obj];
					loader->second->load(obj, extension, out);
					return out;
				}
			}
			return nullOut;
		}

		auto save(const std::string &obj) -> void
		{
			auto extension = StringUtils::getExtension(obj);
			auto loader    = loaders.find(extension);
			if (loader == loaders.end())
			{
				LOGE("Unknown file extension {0}, fileName : {1}", extension, obj);
			}
			else
			{
				loader->second->save(obj);
			}
		}

		auto init() -> void
		{
			addLoader<GLTFLoader>();
			addLoader<OBJLoader>();
			addLoader<FBXLoader>();
		}

		auto addLoader(const std::string &extension, const AssetsArchive::Ptr &loader) -> void
		{
			loaders.emplace(extension, loader);
			supportExtensions.emplace(extension);
		}

		auto getSupportExtensions() -> const std::unordered_set<std::string> &
		{
			return supportExtensions;
		}

		auto getCache() -> std::unordered_map<std::string, std::vector<std::shared_ptr<IResource>>> &
		{
			return cache;
		}
	}        // namespace loaders
};        // namespace maple
