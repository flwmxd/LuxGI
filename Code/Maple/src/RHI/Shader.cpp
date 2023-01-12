//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Shader.h"
#include "Others/Console.h"
#include "Others/StringUtils.h"
#include "RHI/Definitions.h"
#include <spirv_cross.hpp>

namespace maple
{
	namespace
	{
		inline auto getShaderTypeByName(const std::string &name) -> ShaderType
		{
			const std::unordered_map<std::string, ShaderType> types =
			    {
			        {"Vertex", ShaderType::Vertex},
			        {"Fragment", ShaderType::Fragment},
			        {"Geometry", ShaderType::Geometry},
			        {"Compute", ShaderType::Compute},
			        {"RayMiss", ShaderType::RayMiss},
			        {"RayCloseHit", ShaderType::RayCloseHit},
			        {"RayAnyHit", ShaderType::RayAnyHit},
			        {"RayGen", ShaderType::RayGen},
			        {"RayIntersect", ShaderType::RayIntersect}};

			if (auto iter = types.find(name); iter != types.end())
				return iter->second;

			MAPLE_ASSERT(false, "Unknow shader type");
			return ShaderType::Unknown;
		}
	}        // namespace

	auto Shader::spirvTypeToDataType(const spirv_cross::SPIRType &type, uint32_t size) -> ShaderDataType
	{
		switch (type.basetype)
		{
			case spirv_cross::SPIRType::Boolean:
				return ShaderDataType::Bool;
			case spirv_cross::SPIRType::Int:
				if (type.vecsize == 1)
					return ShaderDataType::Int;
				if (type.vecsize == 2)
					return ShaderDataType::IVec2;
				if (type.vecsize == 3)
					return ShaderDataType::IVec3;
				if (type.vecsize == 4)
					return ShaderDataType::IVec4;

			case spirv_cross::SPIRType::UInt:
				return ShaderDataType::UInt;
			case spirv_cross::SPIRType::Float:
				if (type.columns == 3)
					return ShaderDataType::Mat3;
				if (type.columns == 4)
				{
					if (size > sizeof(glm::mat4))
					{
						return ShaderDataType::Mat4Array;
					}
					return ShaderDataType::Mat4;
				}

				if (type.vecsize == 1)
					return ShaderDataType::Float32;
				if (type.vecsize == 2)
					return ShaderDataType::Vec2;
				if (type.vecsize == 3)
					return ShaderDataType::Vec3;
				if (type.vecsize == 4)
					return ShaderDataType::Vec4;
				break;
			case spirv_cross::SPIRType::Struct:
				return ShaderDataType::Struct;
		}
		return ShaderDataType::None;
	}

	auto Shader::spirvTypeToTextureType(const spv::ImageFormat &format) -> TextureFormat
	{
		switch (format)
		{
			case spv::ImageFormatRgba32f:
				return TextureFormat::RGBA32;
			case spv::ImageFormatRgba16f:
				return TextureFormat::RGBA16;
			case spv::ImageFormatRgba8:
				return TextureFormat::RGBA8;
			case spv::ImageFormatR32i:
				return TextureFormat::R32I;
			case spv::ImageFormatR32ui:
				return TextureFormat::R32UI;
			case spv::ImageFormatR8:
				return TextureFormat::R8;
			case spv::ImageFormatRg16f:
				return TextureFormat::RG16F;
			case spv::ImageFormatR16f:
				return TextureFormat::R16F;
		}

		MAPLE_ASSERT(false, "unsupported spv::ImageFormat");
	}

	auto Shader::parseSource(const std::vector<std::string> &lines, std::unordered_multimap<ShaderType, std::string> &shaders) -> void
	{
		for (uint32_t i = 0; i < lines.size(); i++)
		{
			std::string str = lines[i];
			if (StringUtils::startWith(str, "#"))
			{
				auto path = StringUtils::split(str, " ");
				auto type = getShaderTypeByName(path[0].substr(1, path[0].length()));
				if (type != ShaderType::Unknown)
				{
					StringUtils::trim(path[1], "\r");
					shaders.emplace(type, path[1]);
					LOGV("{0} : {1}", path[0], path[1]);
				}
			}
		}
	}

}        // namespace maple
