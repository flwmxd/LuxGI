//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "IO/IResource.h"
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace maple
{
	class Texture2D;
	class Shader;
	class Pipeline;
	class UniformBuffer;
	class CommandBuffer;
	class DescriptorSet;

	static constexpr float   PBR_WORKFLOW_SEPARATE_TEXTURES  = 0.0f;
	static constexpr float   PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
	static constexpr float   PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;
	static constexpr int32_t MATERAL_LAYOUT_INDEX            = 1;

	struct MaterialProperties
	{
		glm::vec4 albedoColor       = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 roughnessColor    = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 metallicColor     = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 emissiveColor     = glm::vec4(0.0f, 0.0f, 0.0f, 10.0f);
		float     usingAlbedoMap    = 1.0f;
		float     usingMetallicMap  = 1.0f;
		float     usingRoughnessMap = 1.0f;
		float     usingNormalMap    = 1.0f;
		float     usingAOMap        = 1.0f;
		float     usingEmissiveMap  = 1.0f;
		float     workflow          = PBR_WORKFLOW_SEPARATE_TEXTURES;

		//padding in vulkan
		float padding = 0.0f;
	};

	struct MaterialAlbedoProperties
	{
		glm::vec4 albedoColor    = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		float     usingAlbedoMap = 1.0f;
		float     padding        = 0.0f;
		float     padding1       = 0.0f;
		float     padding2       = 0.0f;
	};

	struct PBRMataterialTextures
	{
		std::shared_ptr<Texture2D> albedo;
		std::shared_ptr<Texture2D> normal;
		std::shared_ptr<Texture2D> metallic;
		std::shared_ptr<Texture2D> roughness;
		std::shared_ptr<Texture2D> ao;
		std::shared_ptr<Texture2D> emissive;
	};

	class MAPLE_EXPORT Material : public IResource
	{
	  public:
		enum class RenderFlags : uint32_t
		{
			NONE                 = 0,
			DepthTest            = BIT(0),
			Wireframe            = BIT(1),
			ForwardRender        = BIT(2),
			DeferredRender       = BIT(3),
			ForwardPreviewRender = BIT(4),
			NoShadow             = BIT(5),
			TwoSided             = BIT(6),
			AlphaBlend           = BIT(7)
		};

	  protected:
		int32_t renderFlags = 0;

	  public:
		using Ptr = std::shared_ptr<Material>;

		Material(const std::shared_ptr<Shader> &shader, const MaterialProperties &properties = MaterialProperties(), const PBRMataterialTextures &textures = PBRMataterialTextures());
		Material();

		~Material();

		inline auto setRenderFlags(int32_t flags)
		{
			renderFlags = flags;
		}

		inline auto setRenderFlag(Material::RenderFlags flag)
		{
			renderFlags |= static_cast<int32_t>(flag);
		}

		inline auto removeRenderFlag(Material::RenderFlags flag)
		{
			renderFlags &= ~static_cast<int32_t>(flag);
		}

		auto loadPBRMaterial(const std::string &name, const std::string &path, const std::string &extension = ".png") -> void;
		auto loadMaterial(const std::string &name, const std::string &path) -> void;
		/**
		 * layoutID : id in shader layout(set = ?)
		 */
		auto createDescriptorSet(int32_t layoutID = MATERAL_LAYOUT_INDEX, bool pbr = true) -> void;

		auto updateDescriptorSet() -> void;
		auto updateUniformBuffer() -> void;

		auto setMaterialProperites(const MaterialProperties &properties) -> void;
		auto setTextures(const PBRMataterialTextures &textures) -> void;
		auto setAlbedoTexture(const std::string &path) -> void;
		auto setAlbedo(const std::shared_ptr<Texture2D> &texture) -> void;
		auto setNormalTexture(const std::string &path) -> void;
		auto setRoughnessTexture(const std::string &path) -> void;
		auto setMetallicTexture(const std::string &path) -> void;
		auto setAOTexture(const std::string &path) -> void;
		auto setEmissiveTexture(const std::string &path) -> void;

		auto bind(const CommandBuffer *cmdBuffer) -> void;

		inline auto &isTexturesUpdated() const
		{
			return texturesUpdated;
		}

		inline auto setTexturesUpdated(bool updated)
		{
			texturesUpdated = updated;
		}

		inline auto &getTextures()
		{
			return pbrMaterialTextures;
		}
		inline const auto &getTextures() const
		{
			return pbrMaterialTextures;
		}

		inline auto getShader() const
		{
			return shader;
		}

		inline auto getRenderFlags() const
		{
			return renderFlags;
		}

		inline auto isFlagOf(RenderFlags flag) const -> bool
		{
			return (uint32_t) flag & renderFlags;
		}

		inline auto &getName() const
		{
			return name;
		}

		inline const auto &getProperties() const
		{
			return materialProperties;
		}

		inline auto &getProperties()
		{
			return materialProperties;
		}

		inline auto getDescriptorSet()
		{
			return descriptorSet;
		}

		auto getDescriptorSet(const std::string &name) -> std::shared_ptr<DescriptorSet>;

		inline auto &getMaterialId() const
		{
			return materialId;
		}

		auto setShader(const std::string &path) -> void;
		auto setShader(const std::shared_ptr<Shader> &shader, bool albedoColor = false) -> void;

		template <typename Archive>
		inline auto save(Archive &archive) const -> void
		{
			
		}

		template <typename Archive>
		inline auto load(Archive &archive) -> void
		{
		}

		auto getShaderPath() const -> std::string;

		inline auto getResourceType() const -> FileType
		{
			return FileType::Material;
		}

		inline auto getPath() const -> std::string
		{
			return materialId;
		}

		inline auto getId() const
		{
			return id;
		}

		inline auto isDirty() const
		{
			return dirty;
		}

		inline auto setDirty(bool d)
		{
			dirty = d;
		}

		inline auto getMaterialTextures() const
		{
			return pbrMaterialTextures;
		}

		Material(const std::string &materialId);

	  private:
		PBRMataterialTextures          pbrMaterialTextures;
		MaterialProperties             materialProperties;
		MaterialAlbedoProperties       materialAlbedoProperties;
		std::shared_ptr<Shader>        shader;
		std::shared_ptr<UniformBuffer> materialPropertiesBuffer;
		std::string                    name;
		std::string                    materialId;
		std::shared_ptr<DescriptorSet> descriptorSet;

		std::unordered_map<std::string, std::shared_ptr<DescriptorSet>> cachedDescriptorSet;

		bool    texturesUpdated = false;
		bool    onlyAlbedoColor = false;
		bool    dirty           = false;
		int32_t id              = 0;
	};
}        // namespace maple
