#pragma once

namespace engine {

	class AnimationManager {
	public:

		~AnimationManager() {};

		template <typename T>
		void registerAnimation(T* animation, typename T::TargetType target);

		template <typename T>
		void unregisterAnimation(T* animation);

		void update(const float delta);

	private:
	};

}