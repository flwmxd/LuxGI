//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "ImGuiSystem.h"
#include "Application.h"
#include "Window/WindowWin.h"

#include "Engine/Profiler.h"
#include "ImGuiHelpers.h"
#include "RHI/ImGuiRenderer.h"

#include <IconsMaterialDesignIcons.h>
#include <MaterialDesign.inl>
#include <RobotoRegular.inl>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "IoC/SystemBuilder.h"
#include "Engine/Renderer/RendererData.h"

namespace maple
{
	ImGuiSystem::ImGuiSystem(bool clearScreen) :
	    clearScreen(clearScreen)
	{
	}
	ImGuiSystem::~ImGuiSystem()
	{
		ImGui::DestroyContext();
	}

	auto ImGuiSystem::newFrame(const Timestep &step) -> void
	{
		imguiRender->newFrame(step);
	}

	auto ImGuiSystem::onInit() -> void
	{
		PROFILE_FUNCTION();
		Application::get()->getEventDispatcher().addEventHandler(&handler);

		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO &io = ImGui::GetIO();

		io.DisplaySize = ImVec2(
		    static_cast<float>(Application::getWindow()->getWidth()),
		    static_cast<float>(Application::getWindow()->getHeight()));

		addIcon();

		imguiRender = ImGuiRenderer::create(
		    Application::getWindow()->getWidth(),
		    Application::getWindow()->getHeight(),
		    clearScreen);

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		//io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;

		imguiRender->init();

		/*io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.ConfigWindowsMoveFromTitleBarOnly = true;*/
		setTheme();
	}

	auto ImGuiSystem::onRender(Scene *scene) -> void
	{
		PROFILE_FUNCTION();
		imguiRender->render(Application::getBuilder()->getGlobalComponent<component::RendererData>().commandBuffer);
	}

	auto ImGuiSystem::addIcon() -> void
	{
		PROFILE_FUNCTION();
		ImGuiIO &            io             = ImGui::GetIO();
		static const ImWchar icons_ranges[] = {ICON_MIN_MDI, ICON_MAX_MDI, 0};
		//io.Fonts->AddFontFromFileTTF("fonts/simsun.ttf", 16.f, NULL, io.Fonts->GetGlyphRangesChineseFull());
		//	io.Fonts->AddFontDefault();

		ImFontConfig iconsConfig;
		iconsConfig.MergeMode   = false;
		iconsConfig.PixelSnapH  = true;
		iconsConfig.OversampleH = iconsConfig.OversampleV = 1;
		iconsConfig.GlyphMinAdvanceX                      = 4.0f;
		iconsConfig.SizePixels                            = 16.f;

		static const ImWchar ranges[] = {
		    0x0020,
		    0x00FF,
		    0x0400,
		    0x044F,
		    0,
		};

		io.Fonts->AddFontFromMemoryCompressedTTF(
		    RobotoRegular_compressed_data,
		    RobotoRegular_compressed_size, 16.f,
		    &iconsConfig,
		    ranges);

		iconsConfig.MergeMode     = true;
		iconsConfig.PixelSnapH    = true;
		iconsConfig.GlyphOffset.y = 1.0f;
		iconsConfig.OversampleH = iconsConfig.OversampleV = 1;
		iconsConfig.PixelSnapH                            = true;
		iconsConfig.SizePixels                            = 16.f;
		io.Fonts->AddFontFromMemoryCompressedTTF(
		    MaterialDesign_compressed_data,
		    MaterialDesign_compressed_size, 16.f,
		    &iconsConfig, icons_ranges);

		io.Fonts->AddFontDefault();
	}

	auto ImGuiSystem::onResize(uint32_t w, uint32_t h) -> void
	{
		PROFILE_FUNCTION();
		imguiRender->onResize(w, h);
	}

	auto ImGuiSystem::setTheme() -> void
	{
		ImGuiStyle *style  = &ImGui::GetStyle();
		ImVec4 *    colors = style->Colors;

		colors[ImGuiCol_Text]                  = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_TextDisabled]          = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
		colors[ImGuiCol_WindowBg]              = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_ChildBg]               = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
		colors[ImGuiCol_PopupBg]               = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_Border]                = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
		colors[ImGuiCol_BorderShadow]          = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
		colors[ImGuiCol_FrameBg]               = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
		colors[ImGuiCol_FrameBgActive]         = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
		colors[ImGuiCol_TitleBg]               = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgActive]         = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_MenuBarBg]             = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_CheckMark]             = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_SliderGrab]            = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Button]                = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
		colors[ImGuiCol_ButtonHovered]         = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_ButtonActive]          = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
		colors[ImGuiCol_Header]                = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_HeaderHovered]         = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_HeaderActive]          = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_Separator]             = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SeparatorActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_ResizeGrip]            = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
		colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
		colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Tab]                   = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabHovered]            = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
		colors[ImGuiCol_TabActive]             = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_TabUnfocused]          = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_DockingPreview]        = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
		colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_PlotLines]             = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_PlotHistogram]         = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
		colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_TextSelectedBg]        = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_DragDropTarget]        = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavHighlight]          = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
		colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

		style->ChildRounding     = 4.0f;
		style->FrameBorderSize   = 1.0f;
		style->FrameRounding     = 2.0f;
		style->GrabMinSize       = 7.0f;
		style->PopupRounding     = 2.0f;
		style->ScrollbarRounding = 12.0f;
		style->ScrollbarSize     = 13.0f;
		style->TabBorderSize     = 1.0f;
		style->TabRounding       = 4.0f;
		style->WindowRounding    = 4.0f;
	}

	auto ImGuiSystem::update() -> void
	{
		ImGui::Render();
	}

};        // namespace maple
