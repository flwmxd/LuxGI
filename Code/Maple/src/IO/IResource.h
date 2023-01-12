//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace maple
{
	enum class FileType : int32_t
	{
		Normal,
		Folder,
		Texture,
		Model,
		FBX,
		OBJ,
		Text,
		Script,
		Dll,
		Scene,
		MP3,
		OGG,
		AAC,
		WAV,
		TTF,
		C_SHARP,
		Shader,
		Material,
		Animation,
		Skeleton,
		AnimCtrl,
		Blueprint,
		//for gizmo
		Lighting,
		Camera,
		Length
	};

	inline auto fileTypeToStr(FileType type) -> const char *
	{
		switch (type)
		{
#define STR(r)        \
	case FileType::r: \
		return #r
			STR(Normal);
			STR(Folder);
			STR(Texture);
			STR(Model);
			STR(FBX);
			STR(OBJ);
			STR(Text);
			STR(Script);
			STR(Dll);
			STR(Scene);
			STR(MP3);
			STR(OGG);
			STR(AAC);
			STR(WAV);
			STR(TTF);
			STR(C_SHARP);
			STR(Shader);
			STR(Material);
			STR(Animation);
			STR(Skeleton);
			STR(Lighting);
			STR(Camera);
			STR(AnimCtrl);
			STR(Blueprint);
			STR(Length);
#undef STR
			default:
				return "UNKNOWN_ERROR";
		}
	}

	const std::array<std::string, static_cast<int32_t>(FileType::Length)> EditorInBuildIcon =
	    {
	        "editor-icons/icons8-file-100.png",
	        "editor-icons/icons8-folder-100.png",
	        "editor-icons/icons8-image-file-100.png",
	        "editor-icons/icons8-object-100.png",
	        "editor-icons/icons8-fbx-100.png",
	        "editor-icons/icons8-obj-100.png",
	        "editor-icons/icons8-document-100.png",
	        "editor-icons/icons8-document-100.png",
	        "editor-icons/icons8-dll-80.png",
	        "editor-icons/icons8-maple-leaf-100.png",
	        "editor-icons/icons8-mp3-100.png",
	        "editor-icons/icons8-ogg-100.png",
	        "editor-icons/icons8-aac-100.png",
	        "editor-icons/icons8-wav-100.png",
	        "editor-icons/icons8-ttf-100.png",
	        "editor-icons/icons8-cs-80.png",

	        "editor-icons/shader.png",          //shader
	        "editor-icons/material.png",        //material

	        "editor-icons/icons8-animation-85.png",               //Animation
	        "editor-icons/icons8-skeleton-64.png",                //Skeleton
	        "editor-icons/icon8-animation-controller.png",        //AnimCtrl
	        "editor-icons/icons8-blueprint-85.png",
	        "editor-icons/light.png",
	        "editor-icons/camera.png",
	};

	static std::unordered_map<std::string, FileType> ExtensionsToType = {
	    {"mp3", FileType::MP3},
	    {"ogg", FileType::OGG},
	    {"wav", FileType::WAV},
	    {"aac", FileType::AAC},
	    {"jpg", FileType::Texture},
	    {"png", FileType::Texture},
	    {"tga", FileType::Texture},
	    {"lua", FileType::Script},
	    {"cs", FileType::C_SHARP},
	    {"glsl", FileType::Text},
	    {"shader", FileType::Text},
	    {"vert", FileType::Text},
	    {"frag", FileType::Text},
	    {"text", FileType::Text},
	    {"scene", FileType::Scene},
	    {"obj", FileType::Model},
	    {"fbx", FileType::Model},
	    {"glb", FileType::Model},
	    {"gltf", FileType::Model},
	    {"dll", FileType::Dll},
	    {"controller", FileType::AnimCtrl},
	    {"material", FileType::Material},
	    {"blueprint", FileType::Blueprint},
	    {"so", FileType::Dll}};

	class IResource
	{
	  public:
		virtual auto getResourceType() const -> FileType = 0;
		virtual auto getPath() const -> std::string      = 0;
	};
};        // namespace maple
