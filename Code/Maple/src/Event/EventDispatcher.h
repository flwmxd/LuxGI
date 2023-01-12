//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Event.h"
#include "EventHandler.h"
#include <cstdint>
#include <functional>
#include <future>
#include <queue>
#include <set>

namespace maple
{
	class MAPLE_EXPORT EventDispatcher final
	{
	  public:
		EventDispatcher();
		~EventDispatcher();

		EventDispatcher(const EventDispatcher &) = delete;
		EventDispatcher &operator=(const EventDispatcher &) = delete;

		EventDispatcher(EventDispatcher &&) = delete;
		EventDispatcher &operator=(EventDispatcher &&) = delete;

		auto addEventHandler(EventHandler *handler) -> void;
		auto removeEventHandler(EventHandler *handler) -> void;
		auto dispatchEvent(std::unique_ptr<Event> &&event) -> bool;

		auto postEvent(std::unique_ptr<Event> &&event) -> std::future<bool>;

		auto dispatchEvents() -> void;

	  private:
		std::vector<EventHandler *> eventHandlers;
		std::set<EventHandler *>    eventHandlerAddSet;
		std::set<EventHandler *>    eventHandlerDeleteSet;

		std::mutex                                                        eventQueueMutex;
		std::queue<std::pair<std::promise<bool>, std::unique_ptr<Event>>> eventQueue;
	};

};        // namespace maple
