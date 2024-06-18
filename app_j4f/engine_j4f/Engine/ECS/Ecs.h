#pragma once

#include "System.h"

#include <algorithm>
#include <vector>

namespace engine {

	// ECS main manager class

	class Ecs {
	public:

		inline void registerSystem(System* s) {
			_systems.push_back(s);
		}

		inline void unregisterSystem(System* s) {
			_systems.erase(std::remove(_systems.begin(), _systems.end(), s), _systems.end());
		}

		void update(const float delta);

	private:
		std::vector<System*> _systems;
	};

}