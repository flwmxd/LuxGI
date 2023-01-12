//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "IO/Image.h"
#include "Loader.h"
#include <memory>
#include <string>

namespace maple
{
	class ImageLoader final
	{
	  public:
		static auto loadAsset(const std::string &name, bool mipmaps = true, bool flipY = true) -> std::unique_ptr<Image>;
		static auto loadAsset(const std::string &name, Image *image) -> void;
	};

}        // namespace maple
