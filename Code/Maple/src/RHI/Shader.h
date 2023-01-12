//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Definitions.h"
#include "DescriptorSet.h"
#include "Engine/Core.h"
#include "IO/IResource.h"
#include <unordered_set>

namespace spv
{
	enum ImageFormat;
};
namespace spirv_cross
{
	struct SPIRType;
}

namespace maple
{
	//Keep Order....
	constexpr int32_t ShaderTypeLength = 12;

	inline auto shaderTypeToName(ShaderType type) -> std::string
	{
		switch (type)
		{
#define STR(r)                 \
	case maple::ShaderType::r: \
		return #r
			STR(Vertex);
			STR(Fragment);
			STR(Geometry);
			STR(TessellationControl);
			STR(TessellationEvaluation);
			STR(RayMiss);
			STR(RayCloseHit);
			STR(RayAnyHit);
			STR(RayGen);
			STR(RayIntersect);
#undef STR
		}
		return "Unknown";
	}

	struct PushConstant
	{
		uint32_t                       size;
		std::unordered_set<ShaderType> shaderStages;
		std::vector<uint8_t>           data;
		uint32_t                       offset = 0;
		std::string                    name;

		std::vector<BufferMemberInfo> members;

		inline auto setValue(const std::string &name, const void *value)
		{
			for (auto &member : members)
			{
				if (member.name == name)
				{
					memcpy(&data[member.offset], value, member.size);
					break;
				}
			}
		}

		inline auto setData(const void *value)
		{
			memcpy(data.data(), value, size);
		}
	};

	struct ShaderEnumClassHash
	{
		template <typename T>
		constexpr auto operator()(T t) const -> std::size_t
		{
			return static_cast<std::size_t>(t);
		}
	};

	using VariableArraySize = std::unordered_map<std::string, size_t>;

	class CommandBuffer;
	class Pipeline;
	class DescriptorSet;

	class MAPLE_EXPORT Shader : public IResource
	{
	  public:
		using Ptr                                                                                      = std::shared_ptr<Shader>;
		virtual ~Shader()                                                                              = default;
		virtual auto bind() const -> void                                                              = 0;
		virtual auto unbind() const -> void                                                            = 0;
		virtual auto getName() const -> const std::string &                                            = 0;
		virtual auto getFilePath() const -> const std::string &                                        = 0;
		virtual auto getHandle() const -> void *                                                       = 0;
		virtual auto getPushConstants() -> std::vector<PushConstant> &                                 = 0;
		virtual auto bindPushConstants(const CommandBuffer *commandBuffer, Pipeline *pipeline) -> void = 0;
		virtual auto getPushConstant(uint32_t index) -> PushConstant *
		{
			return nullptr;
		}

		virtual auto getDescriptorInfo(uint32_t index) -> const std::vector<Descriptor>
		{
			return std::vector<Descriptor>{};
		}

		virtual auto getResourceType() const -> FileType
		{
			return FileType::Shader;
		}

		auto        spirvTypeToDataType(const spirv_cross::SPIRType &type, uint32_t size) -> ShaderDataType;
		static auto spirvTypeToTextureType(const spv::ImageFormat &format) -> TextureFormat;

		virtual auto reload() -> void{};

		inline auto getLocalSizeX() const
		{
			return localSizeX;
		};

		inline auto getLocalSizeY() const
		{
			return localSizeY;
		};
		inline auto getLocalSizeZ() const
		{
			return localSizeZ;
		};

		inline auto isComputeShader() const
		{
			return computeShader;
		}

		inline auto isRaytracingShader() const
		{
			return raytracingShader;
		}

	  protected:
		auto parseSource(const std::vector<std::string> &lines, std::unordered_multimap<ShaderType, std::string> &shaders) -> void;

	  public:
		static auto create(const std::string &filepath, const VariableArraySize &size = {}) -> std::shared_ptr<Shader>;
		static auto create(const std::vector<uint32_t> &vertData, const std::vector<uint32_t> &fragData) -> std::shared_ptr<Shader>;

	  protected:
		bool     computeShader    = false;
		bool     raytracingShader = false;
		uint32_t localSizeX       = 1;
		uint32_t localSizeY       = 1;
		uint32_t localSizeZ       = 1;
	};
}        // namespace maple
