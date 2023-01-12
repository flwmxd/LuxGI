//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Engine/Core.h"
#include "Engine/Mesh.h"

namespace maple
{
	class Skeleton;
	class Animation;

	namespace io
	{
		class MAPLE_EXPORT AssetsArchive
		{
		  public:
			using Ptr = std::shared_ptr<AssetsArchive>;
			virtual auto load(const std::string &fileName, const std::string &extension, std::vector<std::shared_ptr<IResource>> &out) const -> void{};
			virtual auto save(const std::string &fileName) const -> void{};
		};

		auto MAPLE_EXPORT load(const std::string &obj) -> std::vector<std::shared_ptr<IResource>> &;
		auto MAPLE_EXPORT save(const std::string &obj) -> void;

		auto MAPLE_EXPORT init() -> void;

		auto MAPLE_EXPORT addLoader(const std::string &extension, const AssetsArchive::Ptr &loader) -> void;

		auto MAPLE_EXPORT getSupportExtensions() -> const std::unordered_set<std::string> &;

		auto MAPLE_EXPORT getCache() -> std::unordered_map<std::string, std::vector<std::shared_ptr<IResource>>> &;

		template <typename T, typename... Args>
		inline auto emplace(const std::string &obj, Args &&...args) -> std::shared_ptr<T> 
		{
			auto &cache = getCache();
			auto iter = cache.find(obj);
			if (iter == cache.end())
			{
				auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
				cache[obj].emplace_back(ptr);
				return std::static_pointer_cast<T>(ptr);
			}
			return std::static_pointer_cast<T>(iter->second[0]);
		}

		template <typename T>
		inline typename std::enable_if<std::is_base_of<AssetsArchive, T>::value, void>::type addLoader()
		{
			auto loader = std::make_shared<T>();
			for (auto ext : T::EXTENSIONS)
			{
				addLoader(ext, loader);
			}
		}
	}        // namespace loaders
};        // namespace maple
