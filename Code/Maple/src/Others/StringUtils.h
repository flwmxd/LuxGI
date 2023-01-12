//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <functional>
#include <memory>
#include <stdarg.h>
#include <string>
#include <vector>

namespace maple
{
	class MAPLE_EXPORT StringUtils
	{
	  public:
		template <typename... Args>
		inline static auto format(const std::string &format, Args... args) -> std::string
		{
			size_t                  size = snprintf(nullptr, 0, format.c_str(), args...) + 1;        // Extra space for '\0'
			std::unique_ptr<char[]> buf(new char[size]);
			snprintf(buf.get(), size, format.c_str(), args...);
			return std::string(buf.get(), buf.get() + size - 1);
		}

		static auto replace(std::u16string &src, const std::u16string &origin, const std::u16string &des) -> void;
		static auto split(std::string input, const std::string &delimiter) -> std::vector<std::string>;
		static auto split(std::u16string input, const std::u16string &delimiter, std::vector<std::u16string> &outs) -> void;
		static auto split(std::string input, const std::string &delimiter, std::vector<std::string> &outs) -> void;
		static auto startWith(const std::string &str, const std::string &start, bool ignoreCase = false) -> bool;
		static auto contains(const std::string &str, const std::string &start) -> bool;
		static auto endWith(const std::string &str, const std::string &start, bool ignoreCase = false) -> bool;
		static auto trim(std::string &str, const std::string &trimStr = " ") -> void;
		static auto trim(std::u16string &str) -> void;
		static auto replace(std::string &str, const std::string &old, const std::string &newStr) -> void;
		static auto toLower(std::string &data) -> void;
		static auto toLower2(std::string data) -> std::string;

		static auto              replaceExtension(const std::string &path, const std::string &extension) -> std::string;
		static auto              isEmptyOrWhitespace(const std::string &str) -> bool;
		static auto              getExtension(const std::string &fileName) -> std::string;
		static auto              removeExtension(const std::string &fileName) -> std::string;
		static auto              getFileName(const std::string &filePath) -> std::string;
		static auto              getFileNameWithoutExtension(const std::string &filePath) -> std::string;
		static auto              getFilePath(const std::string &filePath) -> std::string;
		static auto              getCurrentWorkingDirectory() -> std::string;
		static auto              isHiddenFile(const std::string &filePath) -> bool;
		static auto              isTextFile(const std::string &filePath) -> bool;
		static auto              isLuaFile(const std::string &filePath) -> bool;
		static auto              isAudioFile(const std::string &filePath) -> bool;
		static auto              isSceneFile(const std::string &filePath) -> bool;
		static auto              isControllerFile(const std::string &filePath) -> bool;
		static auto              isModelFile(const std::string &filePath) -> bool;
		static auto              isTextureFile(const std::string &filePath) -> bool;
		static auto              isCSharpFile(const std::string &filePath) -> bool;
		static auto              isFBXFile(const std::string &filePath) -> bool;
		static const std::string delimiter;

	};        // namespace StringUtils
};            // namespace maple
