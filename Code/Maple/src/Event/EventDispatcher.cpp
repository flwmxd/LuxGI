//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "EventDispatcher.h"
#include "Application.h"
#include "Engine/Profiler.h"
#include <algorithm>
#include <stdlib.h>

namespace maple
{
	EventDispatcher::EventDispatcher()
	{
	}

	EventDispatcher::~EventDispatcher()
	{
		for (EventHandler *eventHandler : eventHandlerAddSet)
			eventHandler->eventDispatcher = nullptr;

		for (EventHandler *eventHandler : eventHandlers)
		{
			auto i = std::find(eventHandlerDeleteSet.begin(), eventHandlerDeleteSet.end(), eventHandler);
			if (i == eventHandlerDeleteSet.end())
			{
				eventHandler->eventDispatcher = nullptr;
			}
		}
	}

	auto EventDispatcher::addEventHandler(EventHandler *eventHandler) -> void
	{
		PROFILE_FUNCTION();
		if (eventHandler->eventDispatcher)
			eventHandler->eventDispatcher->removeEventHandler(eventHandler);

		eventHandler->eventDispatcher = this;

		eventHandlerAddSet.insert(eventHandler);

		auto setIterator = eventHandlerDeleteSet.find(eventHandler);

		if (setIterator != eventHandlerDeleteSet.end())
			eventHandlerDeleteSet.erase(setIterator);
	}

	auto EventDispatcher::removeEventHandler(EventHandler *eventHandler) -> void
	{
		if (eventHandler->eventDispatcher == this)
			eventHandler->eventDispatcher = nullptr;

		eventHandlerDeleteSet.insert(eventHandler);

		auto setIterator = eventHandlerAddSet.find(eventHandler);

		if (setIterator != eventHandlerAddSet.end())
			eventHandlerAddSet.erase(setIterator);
	}

	auto EventDispatcher::dispatchEvent(std::unique_ptr<Event> &&event) -> bool
	{
		PROFILE_FUNCTION();
		if (!event)
			return false;

		bool handled = false;

		for (const EventHandler *eventHandler : eventHandlers)
		{
			auto i = std::find(eventHandlerDeleteSet.begin(), eventHandlerDeleteSet.end(), eventHandler);
			//not find in delete set
			if (i == eventHandlerDeleteSet.end())
			{
				switch (event->getType())
				{
					case EventType::WindowClose:
						break;
					case EventType::WindowResize:
						break;
					case EventType::WindowFocus:
						break;
					case EventType::WindowLostFocus:
						break;

					case EventType::KeyPressed:
						if (eventHandler->keyPressedHandler)
						{
							handled = eventHandler->keyPressedHandler(static_cast<KeyPressedEvent *>(event.get()));
						}
						break;
					case EventType::KeyReleased:
						if (eventHandler->keyReleasedHandler)
						{
							handled = eventHandler->keyReleasedHandler(static_cast<KeyReleasedEvent *>(event.get()));
						}
						break;
					case EventType::MouseClicked:
						if (eventHandler->mouseClickHandler)
							handled = eventHandler->mouseClickHandler(static_cast<MouseClickEvent *>(event.get()));
						break;
					case EventType::MouseReleased:
						if (eventHandler->mouseRelaseHandler)
							handled = eventHandler->mouseRelaseHandler(static_cast<MouseReleaseEvent *>(event.get()));
						break;
					case EventType::MouseMove:
						if (eventHandler->mouseMoveHandler)
							handled = eventHandler->mouseMoveHandler(static_cast<MouseMoveEvent *>(event.get()));
						break;
					case EventType::MouseScrolled:
						if (eventHandler->mouseScrollHandler)
							handled = eventHandler->mouseScrollHandler(static_cast<MouseScrolledEvent *>(event.get()));
						break;
					case EventType::DeferredType:
						if (eventHandler->deferredTypeHandler)
							handled = eventHandler->deferredTypeHandler(static_cast<DeferredTypeEvent *>(event.get()));
						break;
					case EventType::CharInput:
						if (eventHandler->charInputHandler)
							handled = eventHandler->charInputHandler(static_cast<CharInputEvent *>(event.get()));
						break;
				}
			}
			if (handled)        //if this event handled,this even will not dispatch in the low priority handler.
				break;
		}
		return handled;
	}

	auto EventDispatcher::postEvent(std::unique_ptr<Event> &&event) -> std::future<bool>
	{
		PROFILE_FUNCTION();
		std::promise<bool>           promise;
		std::future<bool>            future = promise.get_future();
		std::unique_lock<std::mutex> lock(eventQueueMutex);
		eventQueue.push(std::pair<std::promise<bool>, std::unique_ptr<Event>>(std::move(promise), std::move(event)));
		return future;
	}

	auto EventDispatcher::dispatchEvents() -> void
	{
		PROFILE_FUNCTION();
		//# clear handler what wait for delete
		for (EventHandler *eventHandler : eventHandlerDeleteSet)
		{
			auto i = std::find(eventHandlers.begin(), eventHandlers.end(), eventHandler);

			if (i != eventHandlers.end())
				eventHandlers.erase(i);
		}

		eventHandlerDeleteSet.clear();
		//# sort with priority
		for (EventHandler *eventHandler : eventHandlerAddSet)
		{
			auto i = std::find(eventHandlers.begin(), eventHandlers.end(), eventHandler);

			if (i == eventHandlers.end())
			{
				auto upperBound = std::upper_bound(eventHandlers.begin(), eventHandlers.end(), eventHandler,
				                                   [](const EventHandler *a, const EventHandler *b) {
					                                   return a->priority > b->priority;
				                                   });
				eventHandlers.insert(upperBound, eventHandler);
			}
		}

		eventHandlerAddSet.clear();

		std::pair<std::promise<bool>, std::unique_ptr<Event>> event;

		for (;;)
		{
			std::unique_lock<std::mutex> lock(eventQueueMutex);
			if (eventQueue.empty())
				break;

			event = std::move(eventQueue.front());
			eventQueue.pop();
			lock.unlock();

			event.first.set_value(dispatchEvent(std::move(event.second)));
		}
	}
};        // namespace maple