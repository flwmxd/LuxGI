//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include "RHI/SwapChain.h"

namespace maple
{
	struct WindowInitData
	{
		uint32_t    width;
		uint32_t    height;
		bool        vsync;
		std::string title;
	};

	class NativeWindow
	{
	  public:
		NativeWindow() = default;
		virtual ~NativeWindow()
		{}

		virtual auto onUpdate() -> void             = 0;
		virtual auto getWidth() const -> uint32_t   = 0;
		virtual auto getHeight() const -> uint32_t  = 0;
		virtual auto setVSync(bool sync) -> void    = 0;
		virtual auto isVSync() const -> bool        = 0;
		virtual auto getNativeInterface() -> void * = 0;
		virtual auto init() -> void                 = 0;
		virtual auto isClose() const -> bool        = 0;

		inline auto  getWindowPos() const
		{
			return pos;
		};

		virtual auto setTitle(const std::string &title) -> void;

		virtual auto swapBuffers() -> void{};

		virtual auto getNativeWindow() -> void *
		{
			return nullptr;
		}
		inline auto getScale() const
		{
			return scale;
		}
		inline auto getExtensions() const
		{
			return extensions;
		};

		static auto create(const WindowInitData &data) -> std::unique_ptr<NativeWindow>;

	  protected:
		glm::vec2                 pos;
		std::vector<const char *> extensions;
		float                     scale = 1.f;
	};
};        // namespace maple
