#pragma once

#include "../Core/BitMask.h"
#include <cstdint>

namespace engine {

	class System {
	public:
		System() = default;
		System(BitMask64&& r, BitMask64&& w) : _readMask(std::move(r)), _writeMask(std::move(w)) {}
		virtual ~System() = default;

		virtual void process(const float delta) = 0;

		inline uint16_t getOrder() const { return _order; }
		inline const BitMask64& readMask() const { return _readMask; }
		inline const BitMask64& writeMask() const { return _writeMask; }

		inline void setOrder(const uint16_t order) { _order = _order; }

	protected:
		uint16_t _order = 0;
		BitMask64 _readMask;
		BitMask64 _writeMask;
	};

	template <typename T>
	class SystemHolder : public System {
	public:
		template <typename... Args>
		SystemHolder(BitMask64&& r, BitMask64&& w, Args&&...args) : System(r, w) {
			_systemImpl = T(std::forward<Args>(args)...);
		}

		inline void process(const float delta) override {
			_systemImpl.process(delta);
		}

	private:
		T _systemImpl;
	};

}