#pragma once

#include "../../Core/Math/mathematic.h"
#include "../../Core/Hierarchy.h"
#include "../../Core/Ref_ptr.h"
#include "../../Utils/Debug/Assert.h"
#include "MeshData.h"
#include "Mesh.h"

#include <cstdint>
#include <memory>

namespace engine {

	enum class AnimationEvent : uint8_t {
		Unknown = 0u,
		NewLoop = 1u,
		EndLoop = 2u,
		Process = 3u,
		Finish = 4u,
	};

	class MeshAnimator {
	public:
		enum class State : uint8_t {
			Unknown = 0u,
			Process = 1u,
			End = 2u,
		};

		struct Transform {
			uint8_t mask = 0u;
			uint16_t target_node = 0u;
			vec3f scale = vec3f(1.0f);
			quatf rotation = quatf(1.0f, 0.0f, 0.0f, 0.0f);
			vec3f translation = vec3f(0.0f);
		};

		MeshAnimator(float weight, const size_t transformsCount, const uint8_t latency) :
			_weight(weight),
			_time(0.0f),
			_animation(nullptr),
			_frameTimes(latency),
			_transforms(latency),
			_infinity(true)
		{
			for (size_t i = 0u; i < latency; ++i) {
				_transforms[i].resize(transformsCount);
				_frameTimes[i] = 0.0f;
			}
		}

		MeshAnimator(const Mesh_Animation* animation, float weight, const uint8_t latency, float speed = 1.0f, bool infinity = true) :
			_weight(weight),
			_time(0.0f),
			_speed(speed),
			_animation(animation),
			_frameTimes(latency),
			_transforms(latency),
			_infinity(infinity)
		{
			for (uint16_t i = 0u; i < latency; ++i) {
				_transforms[i].resize(_animation->maxTargetNodeId - _animation->minTargetNodeId + 1u);
				_frameTimes[i] = 0.0f;
			}
		}

		inline State update(const float dt, const uint8_t i) {
			if (_animation == nullptr) return State::Unknown;

			auto event = AnimationEvent::Unknown;
			auto state = State::Process;

			_time += _speed * dt;

			if (_time >= _animation->duration) {
				_time -= _animation->duration;
				if (!_infinity) {
					state = State::End;
					event = AnimationEvent::Finish;
				} else {
					event = AnimationEvent::EndLoop;
				}
			} else {
				event = (event == AnimationEvent::EndLoop) ? AnimationEvent::NewLoop : AnimationEvent::Process;
			}

			_frameTimes[i] = _animation->start + _time;

			// todo: translate event to observers
			// observer->onEvent(event);
			return state;
		}

        [[nodiscard]] inline uint8_t getLatency() const { return _frameTimes.size(); }

		[[nodiscard]] inline float getCurrentTime(const uint8_t i) const {
			if (!_animation) return 0.0f;
			return _frameTimes[i];
		}

		inline void operator()(const float time, const uint8_t n) {
			if (_animation == nullptr) return;

			constexpr float epsilon = 1e-4f;

			for (const auto& channel : _animation->channels) {
				if (channel.sampler == 0xffff || channel.target_node == 0xffff) {
					continue;
				}

				Transform& transform = _transforms[n][channel.target_node - _animation->minTargetNodeId];
				transform.target_node = channel.target_node;
				const Mesh_Animation::AnimationSampler& sampler = _animation->samplers[channel.sampler];

				for (size_t i = 0u, sz = sampler.inputs.size() - 1u; i < sz; ++i) {

					const float t0 = sampler.inputs[i];
					const float t1 = sampler.inputs[i + 1];

					if ((time >= t0) && (time < t1)) {

						const vec4f& v0 = sampler.outputs[i];

						switch (sampler.interpolation) {
							case Mesh_Animation::Interpolation::LINEAR:
							{
								const vec4f& v1 = sampler.outputs[i + 1];
								const float mix_c = (time - t0) / (t1 - t0);
								switch (channel.path) {
									case Mesh_Animation::AimationChannelPath::TRANSLATION:
									{
										transform.mask |= 0b00'00'00'01;
										if (!compare(v0, v1, epsilon)) {
											transform.translation = v0;
										} else {
											transform.translation = glm::mix(v0, v1, mix_c);
										}
									}
										break;
									case Mesh_Animation::AimationChannelPath::ROTATION:
									{
										transform.mask |= 0b00'00'00'10;
										if (!compare(v0, v1, epsilon)) {
											transform.rotation = quatf(v0.w, v0.x, v0.y, v0.z);
										} else {
											const quatf q1(v0.w, v0.x, v0.y, v0.z);
											const quatf q2(v1.w, v1.x, v1.y, v1.z);
											transform.rotation = glm::normalize(glm::slerp(q1, q2, mix_c));
										}
									}
										break;
									case Mesh_Animation::AimationChannelPath::SCALE:
									{
										transform.mask |= 0b00'00'01'00;
										if (!compare(v0, v1, epsilon)) {
											transform.scale = v0;
										} else {
											transform.scale = glm::mix(v0, v1, mix_c);
										}
									}
										break;
									default:
										break;
									}
							}
								break;
							case Mesh_Animation::Interpolation::STEP:
							{
								switch (channel.path) {
									case Mesh_Animation::AimationChannelPath::TRANSLATION:
										transform.mask |= 0b00'00'00'01;
										transform.translation = v0;
										break;
									case Mesh_Animation::AimationChannelPath::ROTATION:
										transform.mask |= 0b00'00'00'10;
										transform.rotation = quatf(v0.w, v0.x, v0.y, v0.z);
										break;
									case Mesh_Animation::AimationChannelPath::SCALE:
										transform.mask |= 0b00'00'01'00;
										transform.scale = v0;
										break;
									default:
										break;
								}
							}
								break;
							case Mesh_Animation::Interpolation::CUBICSPLINE: // todo!
                            {
                                ENGINE_BREAK
                            }
								break;
							default:
								break;
							}

						break;
					}
				}
			}
		}

		inline std::vector<Transform>& getTransforms(const uint8_t i) { return _transforms[i]; }
        [[nodiscard]] inline const std::vector<Transform>& getTransforms(const uint8_t i) const { return _transforms[i]; }

        [[nodiscard]] inline float getWeight() const noexcept { return _weight; }
		inline void setWeight(const float w) { _weight = w; }

        [[nodiscard]] inline float getSpeed() const noexcept { return _speed; }
		inline void setSpeed(const float s) { _speed = s; }

		inline void reset() { _time = 0.0f; }

        [[nodiscard]] inline ref_ptr<const Mesh_Animation> getAnimation() const noexcept { return _animation; }

		inline void apply(MeshSkeleton* skeleton, const uint8_t updateFrame) {
			auto& transforms = _transforms[updateFrame];

			for (auto& transform : transforms) {
				Mesh_Node& target = skeleton->getNode(updateFrame, transform.target_node);

				switch (transform.mask) {
					case 0b00'00'00'01:
					{
						target.setTranslation(transform.translation);
					}
						break;
					case 0b00'00'00'10:
					{
						target.setRotation(transform.rotation);
					}
						break;
					case 0b00'00'01'00:
					{
						target.setScale(transform.scale);
					}
						break;
					case 0b00'00'00'11:
					{
						target.setTranslation(transform.translation);
						target.setRotation(transform.rotation);
					}
						break;
					case 0b00'00'01'01:
					{
						target.setTranslation(transform.translation);
						target.setScale(transform.scale);
					}
						break;
					case 0b00'00'01'10:
					{
						target.setRotation(transform.rotation);
						target.setScale(transform.scale);
					}
						break;
					case 0b00'00'01'11:
					{
						target.setTranslation(transform.translation);
						target.setRotation(transform.rotation);
						target.setScale(transform.scale);
					}
						break;
					default:
						break;
				}

				transform.mask = 0u;
			}
		}

	private:
		float _weight = 0.0f;
		float _time = 0.0f;
		float _speed = 1.0f;
		ref_ptr<const Mesh_Animation> _animation = nullptr;
		std::vector<float> _frameTimes;
		std::vector<std::vector<Transform>> _transforms;
		bool _infinity = true;
	};

	struct AnimatorUpdater {
		using TreeAnimator = MeshAnimator;
		using AnimatorType = HierarchyRaw<TreeAnimator>;

		inline static bool _(AnimatorType* animator, const float delta, const uint8_t i, bool& finished) {
			if (animator->value().getWeight() > 0.0f) {
				auto const state = animator->value().update(delta, i);
				finished |= state == MeshAnimator::State::End;
			}
			return true;
		}
	};

	struct AnimatorCalculator {
		using TreeAnimator = MeshAnimator;
		using AnimatorType = HierarchyRaw<TreeAnimator>;

		static bool _(AnimatorType* animator, const uint8_t i);
	};

	struct AnimatorSkipper {
		using TreeAnimator = MeshAnimator;
		using AnimatorType = HierarchyRaw<TreeAnimator>;

		inline static bool _(AnimatorType* animator) { return animator->value().getWeight() > 0.0f; }
	};

	class MeshAnimationTree {
		using TreeAnimator = MeshAnimator;
        
	public:
        using TargetType = MeshSkeleton*;
		using AnimatorType = HierarchyRaw<TreeAnimator>;

	private:
		inline static bool updateAnimators(AnimatorType* animator, const float delta, const uint8_t i) {
			if (animator->value().getWeight() > 0.0f) {
				animator->value().update(delta, i);
			}
			return true;
		}

		static bool calculateAnimators(AnimatorType* animator, const uint8_t i);
		inline static bool skipAnimator(AnimatorType* animator) { return animator->value().getWeight() > 0.0f; }

	public:
		inline void update(const float delta, const uint8_t i) noexcept {
			bool finished = false;
			if (_animator->value().getWeight() >= 1.0f) {
				auto const state = _animator->value().update(delta, i);
				finished |= state == MeshAnimator::State::End;
			} else {
				//_animator->execute(updateAnimators, delta, i);
				_animator->execute_with<AnimatorUpdater>(delta, i, finished);
			}

			if (finished) {
				setActive(false);
			}
		}

		inline void calculate(const uint8_t i) {
			if (_animator->value().getWeight() >= 1.0f) {
				_animator->value()(_animator->value().getCurrentTime(i), i);
			} else {
				//_animator->r_execute(calculateAnimators, skipAnimator, i);
				_animator->r_execute_with<AnimatorCalculator, AnimatorSkipper>(i);
			}
		}

		inline void updateAnimation(const float delta, TargetType target) {
            target->updateAnimation(delta * _speed, this);
		}

		inline void apply(TargetType target, const uint8_t updateFrame) const {
			_animator->value().apply(target, updateFrame);
		}

        inline bool forceUpdate() const noexcept { return false; }

        inline void setActive(const bool n) noexcept { _active = n; }
		inline bool isActive() const noexcept { return _active && _speed != 0.0f; }
		inline void activate() noexcept { setActive(true); }

        inline bool finished() const noexcept { return false; }

        inline void updateAnimation(const float delta) {
            _updateFrameNum = (_updateFrameNum + 1) % _animator->value().getLatency();
            update(delta * _speed, _updateFrameNum); // просто пересчет времени
            calculate(_updateFrameNum); // расчет scale, rotation, translation для нодов анимации
        }

		MeshAnimationTree(float weight, const size_t transformsCount, const uint8_t latency) : _animator(std::make_unique<AnimatorType>(weight, transformsCount, latency)) { }
		MeshAnimationTree(const Mesh_Animation* animation, float weight, const uint8_t latency) : _animator(std::make_unique<AnimatorType>(animation, weight, latency)) { }

		~MeshAnimationTree() = default;

		inline ref_ptr<AnimatorType> getAnimator() noexcept { return ref_ptr<AnimatorType>(_animator.get()); }
		[[nodiscard]] inline const ref_ptr<AnimatorType> getAnimator() const noexcept { return ref_ptr<AnimatorType>(_animator.get()); }

        [[nodiscard]] inline uint8_t frame() const noexcept { return _updateFrameNum; }

        inline void setSpeed(const float s) noexcept { _speed = s; }
        [[nodiscard]] inline float getSpeed() const noexcept { return _speed; }

	private:
		std::unique_ptr<AnimatorType> _animator;
        uint8_t _updateFrameNum = 0;
        bool _active = true;
        float _speed = 1.0f;
	};
}