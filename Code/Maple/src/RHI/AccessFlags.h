//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace maple
{
	class Texture;
	class StorageBuffer;
	enum class ShaderType : uint32_t;

#ifndef BIT
#	define BIT(x) (1 << x)
#endif        // !BIT

	enum AccessFlags : uint32_t
	{
		Read      = BIT(0),
		Write     = BIT(1),
		ReadWrite = Read | Write
	};

	struct ImageMemoryBarrier
	{
		std::vector<std::shared_ptr<Texture>> textures;
		ShaderType                            fromStage;
		ShaderType                            toStage;
		AccessFlags                           from;
		AccessFlags                           to;
	};

	struct BufferBarrier
	{
		AccessFlags    from;
		AccessFlags    to;
		StorageBuffer *ssbo;
		uint32_t size = std::numeric_limits<uint32_t>::max();
	};
};        // namespace maple
