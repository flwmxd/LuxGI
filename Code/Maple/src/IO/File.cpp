//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "File.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <memory>
#include <sys/stat.h>
#include "Others/Console.h"
#include "Others/StringUtils.h"
#include <nfd.h>

namespace maple
{
	// 128 MB.
	const size_t CHUNK_SIZE = 134217728;

	File::~File()
	{
		if (filePtr != nullptr)
		{
			fclose(filePtr);
		}
		//stream.close();
	}

	File::File() :
	    fileSize(0)
	{
	}

	File::File(const std::string &file, bool write) :
	    file(file), fileSize(0)
	{
		if (!write)
		{
			filePtr = fopen(file.c_str(), "rb");
			if (filePtr != nullptr)
			{
				fseek(filePtr, 0, SEEK_END);
				fileSize = ftell(filePtr);
				fseek(filePtr, 0, SEEK_SET);
			}
		}
		else
		{
			filePtr = fopen(file.c_str(), "wb+");
		}
	}

	auto File::getBuffer() -> std::unique_ptr<int8_t[]>
	{
		return readBytes(fileSize);
	}

	auto File::write(const std::string &file) -> void
	{
		fwrite(file.c_str(), 1, file.length(), filePtr);
	}

	auto File::getMd5() -> std::string
	{
		return "";
	}

	auto File::exists() -> bool
	{
		return fileExists(file);
	}

	auto File::isDirectory() -> bool
	{
		struct stat fileInfo;
		auto        result = stat(file.c_str(), &fileInfo);
		if (result != 0)
		{
			throw std::logic_error("file not exits");
		}
		return (fileInfo.st_mode & S_IFDIR) == S_IFDIR;
	}

	auto File::readBytes(int32_t size) -> std::unique_ptr<int8_t[]>
	{
		std::unique_ptr<int8_t[]> array(new int8_t[size]);
		memset(array.get(), 0, size);
		fread(array.get(), sizeof(int8_t) * size, 1, filePtr);
		pos += sizeof(int8_t) * size;
		return array;
	}

	auto File::removeExtension(const std::string &file) -> std::string
	{
		if (file == "")
		{
			return "";
		}
		std::string ret = file;
		ret             = ret.erase(ret.find_last_of('.'));
		return ret;
	}

	auto File::read(const std::string &name) -> std::unique_ptr<std::vector<uint8_t>>
	{
		if (fileExists(name))
		{
			std::ifstream fileReader(name, std::ios::ate | std::ios::binary);
			auto          fileSize = fileReader.tellg();
			auto          buffer   = std::make_unique<std::vector<uint8_t>>(fileSize);
			fileReader.seekg(0);
			fileReader.read(reinterpret_cast<char *>(buffer->data()), fileSize);
			return buffer;
		}
		return nullptr;
	}

	auto File::getFileType(const std::string &path) -> FileType
	{
		auto extension = StringUtils::getExtension(path);
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
		if (auto iter = ExtensionsToType.find(extension); iter != ExtensionsToType.end())
		{
			return iter->second;
		}
		return FileType::Normal;
	}

	auto File::create(const std::string &file) -> void
	{
		std::ofstream{file};
	}

	auto File::createWitDialog(const std::string &filter, const std::function<void(int32_t, const std::string &)> &call) -> void
	{
		nfdchar_t * outPath    = NULL;
		auto        workingDir = StringUtils::getCurrentWorkingDirectory();
		nfdresult_t result     = NFD_SaveDialog(filter.c_str(), workingDir.c_str(), &outPath);

		if (result == NFD_OKAY)
		{
			std::string str = outPath;
			str += "." + filter;
			create(str);
			call(result, str);
			free(outPath);
		}
		else if (result == NFD_CANCEL)
		{
			call(result, "");
			LOGI("User pressed cancel.");
		}
		else
		{
			call(result, "");
			LOGW("Error: {0}", NFD_GetError());
		}
	}

	auto File::openFile(const std::function<void(int32_t, const std::string &)> &call) -> void
	{
		nfdchar_t * outPath = NULL;
		nfdresult_t result  = NFD_OpenDialog(NULL, NULL, &outPath);

		if (result == NFD_OKAY)
		{
			std::string str = outPath;
			call(result, str);
			free(outPath);
		}
		else if (result == NFD_CANCEL)
		{
			call(result, "");
			LOGI("User pressed cancel.");
		}
		else
		{
			call(result, "");
			LOGW("Error: {0}", NFD_GetError());
		}
	}

	auto File::cache(const std::vector<uint8_t> &buffer) -> void
	{
		if (fileSize != buffer.size())
		{
			fwrite(buffer.data(), sizeof(uint8_t) * buffer.size(), 1, filePtr);
		}
	}

	auto File::fileExists(const std::string &file) -> bool
	{
		struct stat fileInfo;
		return (!stat(file.c_str(), &fileInfo)) != 0;
	}

	auto File::list(const std::function<bool(const std::string &)> &predict) -> const std::vector<std::string>
	{
		std::vector<std::string> files;
		list(files, predict);
		return files;
	}

	auto File::list(std::vector<std::string> &out, const std::function<bool(const std::string &)> &predict) -> void
	{
		listFolder("./", out, predict);
	}

	auto File::listFolder(const std::string &path, std::vector<std::string> &out, const std::function<bool(const std::string &)> &predict) -> void
	{
		std::filesystem::path str(path);
		if (!std::filesystem::exists(str))
		{
			LOGE("%s does not exists", path.c_str());
			return;
		}

		std::filesystem::directory_entry entry(str);        //�ļ����
		if (entry.status().type() == std::filesystem::file_type::directory)
		{
			std::filesystem::directory_iterator list(str);
			for (auto &it : list)
			{
				try
				{
					auto path = it.path().string();

					if (it.status().type() == std::filesystem::file_type::directory)
					{
						listFolder(path, out, predict);
					}
					else
					{
						if (predict == nullptr || predict(it.path().string()))
							out.emplace_back(path);
					}
				}
				catch (...)
				{
					LOGW("{0} : catch an error", __FUNCTION__);
				}
			}
		}
	}

	auto File::getFileName(const std::string &file) -> const std::string
	{
		auto pos = file.find_last_of('/');
		return file.substr(pos + 1);
	}

};        // namespace maple
