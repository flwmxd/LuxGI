//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>

namespace maple
{
	class BatchTask
	{
	  public:
		using Ptr = std::shared_ptr<BatchTask>;
		virtual auto execute() -> void = 0;
		static auto  create() -> Ptr;
	};
};        // namespace maple