//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <glm/vec3.hpp>
#include <string>

#include <entt.hpp>

namespace maple
{
	class TextureCube;
	class Texture;
	class Texture2D;

	namespace component
	{
		struct MAPLE_EXPORT Environment
		{
			static constexpr int32_t IrradianceMapSize = 128;
			static constexpr int32_t PrefilterMapSize  = 256;

			//TOOD should be refactored in the future..
			std::shared_ptr<Texture2D>   equirectangularMap;
			std::shared_ptr<TextureCube> environment;
			std::shared_ptr<TextureCube> prefilteredEnvironment;
			std::shared_ptr<Texture2D>   irradianceSH;

			uint32_t numMips = 0;
			uint32_t width   = 0;
			uint32_t height  = 0;

			bool      envLighting = true;
			bool      pseudoSky   = false;
			float     mieG        = 0.75f;
			glm::vec3 betaR       = glm::vec3(0.0058f, 0.0135f, 0.0331f);
			std::string filePath;

			Environment();

			SERIALIZATION(envLighting, pseudoSky, mieG, betaR, filePath);
		};
	}        // namespace component
}        // namespace maple
