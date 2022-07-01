#include "AnimationTree.h"
#include "MeshData.h"
#include "Mesh.h"

namespace engine {

	bool AnimatorCalculator::_(AnimatorType* animator, const uint8_t i) {
		const auto& children = animator->children();
		if (children.empty()) { // calculate single animation values
			animator->value()(animator->value().getCurrentTime(i), i);
		} else { // mix children animation values;
			std::vector<TreeAnimator::Transform>& transforms = animator->value().getTransforms(i);
			const size_t childrenCount = children.size();
			float w = 0.0f;

			size_t firstNoneZeroWeightChild = 0;
			for (size_t n = 0; n < childrenCount; ++n) {
				const auto& v = children[n]->value();
				w = v.getWeight();
				if (w > 0.0f) { // get first none zero weight value
					firstNoneZeroWeightChild = n;
					const auto anim = v.getAnimation();
					const std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);

					for (size_t idx = 0, trs = ch_transforms.size(); idx < trs; ++idx) {
						memcpy(&transforms[ch_transforms[idx].target_node], &ch_transforms[idx], sizeof(TreeAnimator::Transform));
					}

					break;
				}
			}

			if (w < 1.0f) {
				for (size_t n = firstNoneZeroWeightChild + 1; n < childrenCount; ++n) { // true animation blend variant?
					auto& v = children[n]->value();
					const float w2 = v.getWeight();

					if (w2 <= 0.0f) continue; // skip calculate for weight <= 0.0f

					const auto anim = v.getAnimation();
					std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);
					const float mix = w * 1.0f / (w + w2);

					for (size_t idx = 0, trs = ch_transforms.size(); idx < trs; ++idx) {
						TreeAnimator::Transform& tr0 = transforms[ch_transforms[idx].target_node];
						TreeAnimator::Transform& tr1 = ch_transforms[idx];

						switch (tr1.mask) {
							case 0b00000001:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
							}
								break;
							case 0b00000010:
							{
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00000100:
							{
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00000011:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00000101:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00000110:
							{
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00000111:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							default:
								break;
						}

						tr0.mask |= tr1.mask;
						tr1.mask = 0;
					}

					w += w2;

					if (w >= 1.0f) break;
				}
			}
		}

		return true;
	}

	bool MeshAnimationTree::calculateAnimators(AnimatorType* animator, const uint8_t i) {
		const auto& children = animator->children();
		if (children.empty()) { // calculate single animation values
			animator->value()(animator->value().getCurrentTime(i), i);
		} else { // mix children animation values;
			std::vector<TreeAnimator::Transform>& transforms = animator->value().getTransforms(i);
			const size_t childrenCount = children.size();
			float w = 0.0f;

			size_t firstNoneZeroWeightChild = 0;
			for (size_t n = 0; n < childrenCount; ++n) {
				const auto& v = children[n]->value();
				w = v.getWeight();
				if (w > 0.0f) { // get first none zero weight value
					firstNoneZeroWeightChild = n;
					const auto anim = v.getAnimation();
					const std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);

					for (size_t idx = 0, trs = ch_transforms.size(); idx < trs; ++idx) {
						memcpy(&transforms[ch_transforms[idx].target_node], &ch_transforms[idx], sizeof(TreeAnimator::Transform));
					}

					break;
				}
			}

			if (w < 1.0f) {
				for (size_t n = firstNoneZeroWeightChild + 1; n < childrenCount; ++n) { // true animation blend variant?
					auto& v = children[n]->value();
					const float w2 = v.getWeight();

					if (w2 <= 0.0f) continue; // skip calculate for weight <= 0.0f

					const auto anim = v.getAnimation();
					std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);
					const float mix = w * 1.0f / (w + w2);

					for (size_t idx = 0, trs = ch_transforms.size(); idx < trs; ++idx) {
						TreeAnimator::Transform& tr0 = transforms[ch_transforms[idx].target_node];
						TreeAnimator::Transform& tr1 = ch_transforms[idx];

						switch (tr1.mask) {
							case 0b00000001:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
							}
								break;
							case 0b00000010:
							{
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00000100:
							{
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00000011:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00000101:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00000110:
							{
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00000111:
							{
								tr0.translation = (tr0.mask & 0b00000001) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00000010) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00000100) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							default:
								break;
						}

						tr0.mask |= tr1.mask;
						tr1.mask = 0;
					}

					w += w2;

					if (w >= 1.0f) break;
				}
			}
		}

		return true;
	}
}