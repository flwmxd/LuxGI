//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Math/Frustum.h"
#include <glm/glm.hpp>
namespace maple
{
	class GBuffer;
	class CommandBuffer;
	class Mesh;
	class Texture;
	class RenderDevice;
	class TextureCube;
	class Texture2D;
	class Texture3D;

	namespace component
	{
		class Transform;
		struct CameraView
		{
			glm::mat4             proj;
			glm::mat4             view;
			glm::mat4             projView;
			glm::mat4             projViewOld;
			float                 nearPlane;
			float                 farPlane;
			float                 fov;
			float                 aspect;
			Frustum               frustum;
			component::Transform *cameraTransform;
			glm::vec3             prevPosition{};
			glm::vec3             cameraDelta{};
		};

		struct EnvironmentData
		{
			std::shared_ptr<Texture> skybox;
			std::shared_ptr<Texture> environmentMap;
			std::shared_ptr<Texture> irradianceMap;
			bool                     cloud = false;
		};

		struct RendererData
		{
			CommandBuffer *              commandBuffer = nullptr;
			GBuffer *                    gbuffer       = nullptr;
			std::shared_ptr<Mesh>        screenQuad;
			std::shared_ptr<TextureCube> unitCube;        //1
			RenderDevice *               renderDevice = nullptr;
			uint32_t                     numFrames    = 0;
			std::shared_ptr<Texture2D>   unitTexture;
			std::shared_ptr<Texture3D>   unitTexture3D;
		};

		struct WindowSize
		{
			uint32_t width;
			uint32_t height;
		};
	}        // namespace component

#ifdef MAPLE_OPENGL
	constexpr glm::mat4 BIAS_MATRIX = {
	    0.5, 0.0, 0.0, 0.0,
	    0.0, 0.5, 0.0, 0.0,
	    0.0, 0.0, 0.5, 0.0,
	    0.5, 0.5, 0.5, 1.0};
#endif        // MAPLE_OPENGL

#ifdef MAPLE_VULKAN
	constexpr glm::mat4 BIAS_MATRIX = {
	    0.5, 0.0, 0.0, 0.0,
	    0.0, 0.5, 0.0, 0.0,
	    0.0, 0.0, 1.0, 0.0,
	    0.5, 0.5, 0.0, 1.0};

	//Vulkan's NDC are [-1, 1] on x- and y-axes and [0, 1] on the z-axis

#endif
}        // namespace maple
