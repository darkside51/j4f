#pragma once

#include "../Core/Common.h"
#include "../Core/EngineModule.h"

#include <unordered_map>
#include <functional>
#include <memory>

namespace engine {

	class IEventObserver {
	public:
		virtual ~IEventObserver() = default;
	};

    template <typename T>
    class EventObserverImpl;

    class IEventSubscriber {
    public:
        virtual ~IEventSubscriber() = default;
    };

    template <typename T>
    class EventSubscriber;

	class Bus final : public IEngineModule {
	public:
		~Bus() override = default;

        template <typename EVENT>
        inline void addObserver(EventObserverImpl<EVENT>* o) {
			const uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<EVENT>();
            std::vector<IEventObserver*>& observers = _eventObservers[observer_type_id];
            if (std::find(observers.begin(), observers.end(), o) == observers.end()) {
                observers.emplace_back(o);
            }
        }

        template <typename EVENT>
        inline void removeObserver(EventObserverImpl<EVENT>* o) {
			const uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<EVENT>();
            auto it = _eventObservers.find(observer_type_id);
            if (it != _eventObservers.end()) {
                std::vector<IEventObserver*>& observers = it->second;
                observers.erase(std::remove(observers.begin(), observers.end(), o), observers.end());
                observers.shrink_to_fit();

                if (observers.empty()) { // ?
                    _eventObservers.erase(it);
                }
            }
        }

        template <typename EVENT>
        inline void addSubscriber(const EventSubscriber<EVENT>& s) {
            const uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<EVENT>();
            std::vector<IEventSubscriber*>& subscribers = _eventSubscribers[observer_type_id];
            auto&& ref = const_cast<EventSubscriber<EVENT>&>(s);
            ref._bus = const_cast<Bus*>(this);
            subscribers.push_back(&(ref));
        }

        template <typename EVENT>
        inline std::unique_ptr<EventSubscriber<EVENT>> addSubscriber(const std::function<bool(const EVENT&)>& callback) {
            const uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<EVENT>();
            std::vector<IEventSubscriber*>& subscribers = _eventSubscribers[observer_type_id];
            auto&& result = static_cast<EventSubscriber<EVENT>*>(subscribers.emplace_back(new EventSubscriber<EVENT>(callback)));
            result->_bus = const_cast<Bus*>(this);
            return std::unique_ptr<EventSubscriber<EVENT>>(result);
        }

        template <typename EVENT>
        inline void removeSubscriber(const EventSubscriber<EVENT>* s) {
            const uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<EVENT>();
            auto it = _eventSubscribers.find(observer_type_id);
            if (it != _eventSubscribers.end()) {
                std::vector<IEventSubscriber*>& subscribers = it->second;
                subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), s), subscribers.end());
                subscribers.shrink_to_fit();

                if (subscribers.empty()) { // ?
                    _eventSubscribers.erase(it);
                }
            }
        }

        template <typename EVENT>
        inline bool sendEvent(EVENT&& evt) { // �������� ������� ����������,
            // �������� ������� ������ ����������������, �.�. ���������� � ��� �����, ��� ��������� �����, �������� ����� ���� �� ��������������� ����������, �������� �� Bus::update, �� ���� �� ���� ����� �������

            using simple_event_type = typename std::remove_const<typename std::remove_reference<EVENT>::type>::type;
            const uint16_t observer_type_id = UniqueTypeId<IEventObserver>::getUniqueId<simple_event_type>();

            bool result = false;
            auto it2 = _eventSubscribers.find(observer_type_id);
            if (it2 != _eventSubscribers.end()) {
                for (IEventSubscriber* o : it2->second) {
                    result |= static_cast<EventSubscriber<EVENT>*>(o)->processEvent(evt);
                }
            }

            auto it = _eventObservers.find(observer_type_id);
            if (it != _eventObservers.end()) {
                for (IEventObserver* o : it->second) {
                    result |= static_cast<EventObserverImpl<EVENT>*>(o)->processEvent(std::forward<EVENT>(evt));
                    // if (result && notifyOne) { break; }
                }
            }

            return result;
        }

        void update(const float /*delta*/) {}

	private:
		std::unordered_map<size_t, std::vector<IEventObserver*>> _eventObservers;
        std::unordered_map<size_t, std::vector<IEventSubscriber*>> _eventSubscribers;
	};

    template <typename T>
    class EventObserverImpl : public IEventObserver {
        using type = std::decay_t<T>;
    public:
        virtual bool processEvent(const type& evt) = 0;
        virtual bool processEvent(type&& evt) = 0;
    };

    template <typename T>
    class EventSubscriber : public IEventSubscriber {
        friend class Bus;
    public:
        using Callback = std::function<bool(const T&)>;

        EventSubscriber() = default;
        ~EventSubscriber() override {
            if (_bus && _callback) {
                _bus->removeSubscriber(this);
                _bus = nullptr;
            }
        }

        explicit EventSubscriber(Callback&& callback) : _callback(std::move(callback)) {}
        explicit EventSubscriber(const Callback& callback) : _callback(callback) {}

        EventSubscriber(EventSubscriber&& e) noexcept : _callback(std::move(e._callback)), _bus(e._bus) { e._callback = nullptr; e._bus = nullptr; }
        EventSubscriber(const EventSubscriber& e) : _callback(e.callback), _bus(e._bus) {}

        EventSubscriber& operator= (EventSubscriber&& e) noexcept {
            _callback = std::move(e._callback);
            e._callback = nullptr;
            return *this;
        }

        EventSubscriber& operator= (const EventSubscriber& e) {
            if (&e != this) {
                _callback = e._callback;
            }
            return *this;
        }

        inline bool processEvent(const T& event) const { return _callback(event); }

    private:
        Callback _callback = nullptr;
        Bus* _bus = nullptr;
    };


}