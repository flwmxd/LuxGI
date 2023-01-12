//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Definitions.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class Mesh;
	class Texture;
	class Texture2D;
	class TextureCube;
	class TextureDepth;
	class Texture2DArray;
	class UniformBuffer;
	class GBuffer;
	class DescriptorSet;
	class Shader;
	class Scene;
	class FrameBuffer;

	namespace component
	{
		struct Environment;
		struct AtmosphereData;
	}        // namespace component

	namespace capture_graph
	{
		namespace component
		{
			struct RenderGraph;
		}
	}        // namespace capture_graph
	class MAPLE_EXPORT PrefilterRenderer
	{
	  public:
		static constexpr int32_t SkyboxSize = 512;
		PrefilterRenderer();
		~PrefilterRenderer();

		auto init() -> void;
		auto present() -> void;
		auto renderScene(const CommandBuffer *cmd, capture_graph::component::RenderGraph &graph, component::AtmosphereData *data = nullptr) -> void;
		auto beginScene(component::Environment &env) -> bool;

	  private:
		auto updateIrradianceDescriptor(const CommandBuffer *cmd) -> void;
		auto updatePrefilterDescriptor(const CommandBuffer *cmd) -> void;

		auto generateSkybox(const CommandBuffer *cmd, capture_graph::component::RenderGraph &graph) -> void;
		auto generateAtmosphereSkybox(const CommandBuffer *cmd, component::AtmosphereData *data) -> void;
		auto computeSphericalHarmonics(const CommandBuffer *cmd, component::AtmosphereData *data) -> void;

		auto generateIrradianceMap(const CommandBuffer *cmd, capture_graph::component::RenderGraph &graph) -> void;
		auto generatePrefilterMap(const CommandBuffer *cmd, capture_graph::component::RenderGraph &graph) -> void;

		auto createPipeline() -> void;
		auto updateUniform() -> void;
		auto createFrameBuffer() -> void;

		std::shared_ptr<Shader> irradianceShader;
		std::shared_ptr<Shader> prefilterShader;
		std::shared_ptr<Shader> cubeMapShader;

		std::shared_ptr<DescriptorSet>              irradianceSet;
		std::vector<std::shared_ptr<DescriptorSet>> prefilterSets;

		std::shared_ptr<DescriptorSet> cubeMapSet;

		std::shared_ptr<DescriptorSet> currentSet;

		std::shared_ptr<Texture2D>   skyboxCaptureColor;
		std::shared_ptr<TextureCube> skyboxCube;

		std::shared_ptr<Texture2D> irradianceCaptureColor;
		std::shared_ptr<Texture2D> prefilterCaptureColor;
		std::shared_ptr<Texture2D> equirectangularMap;

		std::shared_ptr<Texture2DArray> shIntermediate;
		std::shared_ptr<Texture2D> shOut;

		std::shared_ptr<TextureDepth> skyboxDepth;

		std::shared_ptr<Mesh>   cube;
		std::shared_ptr<Mesh>   cube2;
		component::Environment *envComponent = nullptr;
		int32_t                 maxMips      = 8;
	};
};        // namespace maple
