#pragma once

#include "../Core/Common.h"
#include "../Core/EngineModule.h"

#include <unordered_map>

namespace engine {

	class IEventObserver {
	public:
		virtual ~IEventObserver() = default;
	};

	template <typename T>
	class EventObserverImpl : public IEventObserver {
	public:
		~EventObserverImpl() = default;
		virtual bool processEvent(const T& evt) = 0;
	};

	class Bus : public IEngineModule {
	public:
		~Bus() = default;

        template <typename EVENT>
        inline void addObserver(EventObserverImpl<EVENT>* o) {
			constexpr uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<EVENT>();
            std::vector<IEventObserver*>& observers = _eventObservers[observer_type_id];
            if (std::find(observers.begin(), observers.end(), o) == observers.end()) {
                observers.emplace_back(o);
            }
        }

        template <typename EVENT>
        inline void removeObserver(EventObserverImpl<EVENT>* o) {
			constexpr uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<EVENT>();
            auto it = _eventObservers.find(observer_type_id);
            if (it != _eventObservers.end()) {
                std::vector<IEventObserver*>& observers = it->second;
                observers.erase(std::remove(observers.begin(), observers.end(), o), observers.end());
                observers.shrink_to_fit();
            }
        }

        template <typename EVENT>
        inline bool sendEvent(EVENT&& evt) { // отправка событий обсерверам,
            // отправка событий сейчас непосредственная, т.е. происходит в том месте, где произошел вызов, возможно лучше было бы централизованно отправлять, например из Bus::update, но пока не могу точно сказать

            using simple_event_type = typename std::remove_const<typename std::remove_reference<EVENT>::type>::type;
            constexpr uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<simple_event_type>();

            bool result = false;
            auto it = _eventObservers.find(observer_type_id);
            if (it != _eventObservers.end()) {
                for (IEventObserver* o : it->second) {
                    result |= static_cast<EventObserverImpl<EVENT>*>(o)->processEvent(evt);
                    // if (result && notifyOne) { break; }
                }
            }
            return result;
        }

        void update(const float delta) {}

	private:
		std::unordered_map<size_t, std::vector<IEventObserver*>> _eventObservers;
	};

}