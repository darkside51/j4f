#pragma once

#include "../../Core/Math/math.h"
#include "../../Core/Hierarchy.h"
#include "MeshData.h"
#include "Mesh.h"

namespace engine {

	class MeshAnimator {
	public:
		struct Transform {
			uint8_t mask = 0;
			uint16_t target_node;
			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
		};

		MeshAnimator(float weight, const size_t transformsCount, const uint8_t latency) :
			_weight(weight),
			_time(0.0f),
			_animation(nullptr),
			_frameTimes(latency),
			_transforms(latency)
		{
			for (size_t i = 0; i < latency; ++i) {
				_transforms[i].resize(transformsCount);
				_frameTimes[i] = 0.0f;
			}
		}

		MeshAnimator(const Mesh_Animation* animation, float weight, const uint8_t latency, float speed = 1.0f) :
			_weight(weight),
			_time(0.0f),
			_speed(speed),
			_animation(animation),
			_frameTimes(latency),
			_transforms(latency)
		{
			for (uint16_t i = 0; i < latency; ++i) {
				_transforms[i].resize(_animation->maxTargetNodeId - _animation->minTargetNodeId + 1);
				_frameTimes[i] = 0.0f;
			}
		}

		inline void update(const float dt, const uint8_t i) {
			if (_animation == nullptr) return;

			_time += _speed * dt;

			if (_time > _animation->duration) {
				_time -= _animation->duration;
			}

			_frameTimes[i] = _animation->start + _time;
		}

		inline float getCurrentTime(const uint8_t i) const {
			if (_animation == nullptr) return 0.0f;
			return _frameTimes[i];
		}

		inline void operator()(const float time, const uint8_t i) {
			if (_animation == nullptr) return;

			constexpr float epsilon = 1e-4f;

			size_t channelNum = 0;
			for (const auto& channel : _animation->channels) {
				if (channel.sampler == 0xffff || channel.target_node == 0xffff) {
					continue;
				}

				Transform& transform = _transforms[i][channel.target_node - _animation->minTargetNodeId];
				transform.target_node = channel.target_node;
				const Mesh_Animation::AnimationSampler& sampler = _animation->samplers[channel.sampler];

				for (size_t i = 0, sz = sampler.inputs.size() - 1; i < sz; ++i) {

					const float t0 = sampler.inputs[i];
					const float t1 = sampler.inputs[i + 1];

					if ((time >= t0) && (time < t1)) {

						const glm::vec4& v0 = sampler.outputs[i];

						switch (sampler.interpolation) {
							case Mesh_Animation::Interpolation::LINEAR:
							{
								const glm::vec4& v1 = sampler.outputs[i + 1];
								const float mix_c = (time - t0) / (t1 - t0);
								switch (channel.path) {
									case Mesh_Animation::AimationChannelPath::TRANSLATION:
									{
										transform.mask |= 0b00000001;
										if (!compare(v0, v1, epsilon)) {
											transform.translation = v0;
										} else {
											transform.translation = glm::mix(v0, v1, mix_c);
										}
									}
										break;
									case Mesh_Animation::AimationChannelPath::ROTATION:
									{
										transform.mask |= 0b00000010;
										if (!compare(v0, v1, epsilon)) {
											transform.rotation = glm::quat(v0.w, v0.x, v0.y, v0.z);
										} else {
											const glm::quat q1(v0.w, v0.x, v0.y, v0.z);
											const glm::quat q2(v1.w, v1.x, v1.y, v1.z);
											transform.rotation = glm::normalize(glm::slerp(q1, q2, mix_c));
										}
									}
										break;
									case Mesh_Animation::AimationChannelPath::SCALE:
									{
										transform.mask |= 0b0000100;
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
										transform.mask |= 0b00000001;
										transform.translation = v0;
										break;
									case Mesh_Animation::AimationChannelPath::ROTATION:
										transform.mask |= 0b00000010;
										transform.rotation = glm::quat(v0.w, v0.x, v0.y, v0.z);
										break;
									case Mesh_Animation::AimationChannelPath::SCALE:
										transform.mask |= 0b00000100;
										transform.scale = v0;
										break;
									default:
										break;
								}
							}
								break;
							case Mesh_Animation::Interpolation::CUBICSPLINE: // todo!
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
		inline const std::vector<Transform>& getTransforms(const uint8_t i) const { return _transforms[i]; }

		inline float getWeight() const { return _weight; }
		inline void setWeight(const float w) { _weight = w; }

		inline float getSpeed() const { return _speed; }
		inline void setSpeed(const float s) { _speed = s; }

		inline const Mesh_Animation* getAnimation() const { return _animation; }

		inline void applyToSkeleton(Skeleton* skeleton, const uint8_t updateFrame) {
			auto& transforms = _transforms[updateFrame];

			for (size_t i = 0, trs = transforms.size(); i < trs; ++i) {
				Transform& transform = transforms[i];
				Mesh_Node& target = skeleton->getNode(updateFrame, transform.target_node);

				switch (transform.mask) {
					case 0b00000001:
					{
						target.setTranslation(transform.translation);
					}
						break;
					case 0b00000010:
					{
						target.setRotation(transform.rotation);
					}
						break;
					case 0b00000100:
					{
						target.setScale(transform.scale);
					}
						break;
					case 0b00000011:
					{
						target.setTranslation(transform.translation);
						target.setRotation(transform.rotation);
					}
						break;
					case 0b00000101:
					{
						target.setTranslation(transform.translation);
						target.setScale(transform.scale);
					}
						break;
					case 0b00000110:
					{
						target.setRotation(transform.rotation);
						target.setScale(transform.scale);
					}
						break;
					case 0b00000111:
					{
						target.setTranslation(transform.translation);
						target.setRotation(transform.rotation);
						target.setScale(transform.scale);
					}
						break;
					default:
						break;
				}

				transform.mask = 0;
			}
		}

	private:
		float _weight;
		float _time;
		float _speed;
		const Mesh_Animation* _animation;
		std::vector<float> _frameTimes;
		std::vector<std::vector<Transform>> _transforms;
	};

	class MeshAnimationTree {
		using TreeAnimator = MeshAnimator;
	public:
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
		inline void update(const float delta, const uint8_t i) {
			if (_animator->value().getWeight() >= 1.0f) {
				_animator->value().update(delta, i);
			} else {
				_animator->execute(updateAnimators, delta, i);
			}
		}

		inline void calculate(const uint8_t i) {
			if (_animator->value().getWeight() >= 1.0f) {
				_animator->value()(_animator->value().getCurrentTime(i), i);
			} else {
				_animator->r_execute(calculateAnimators, skipAnimator, i);
			}
		}

		inline void applyToSkeleton(Skeleton* skeleton, const uint8_t updateFrame) const {
			_animator->value().applyToSkeleton(skeleton, updateFrame);
		}

		MeshAnimationTree(float weight, const size_t transformsCount, const uint8_t latency) : _animator(new AnimatorType(weight, transformsCount, latency)) { }
		MeshAnimationTree(const Mesh_Animation* animation, float weight, const uint8_t latency) : _animator(new AnimatorType(animation, weight, latency)) { }

		~MeshAnimationTree() {
			delete _animator;
		}

		inline AnimatorType* getAnimator() { return _animator; }
		inline const AnimatorType const* getAnimator() const { return _animator; }

	private:
		AnimatorType* _animator;
	};
}