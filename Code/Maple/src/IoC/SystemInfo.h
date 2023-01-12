//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include <unordered_map>

namespace maple::ioc
{
	struct SystemInfo
	{
		using SystemCall = void (*)(entt::registry &);

		uint32_t                                    systemId;
		std::string_view                            systemName;
		SystemCall                                  systemCall;
	};
}        // namespace ecs
