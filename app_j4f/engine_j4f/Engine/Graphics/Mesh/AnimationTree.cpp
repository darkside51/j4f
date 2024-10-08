﻿#include "AnimationTree.h"
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

			size_t firstNoneZeroWeightChild = 0u;
			for (size_t n = 0u; n < childrenCount; ++n) {
				const auto& v = children[n]->value();
				w = v.getWeight();
				if (w > 0.0f) { // get first none zero weight value
					firstNoneZeroWeightChild = n;
					const std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);

                    for (auto& ch_transform : ch_transforms) {
						if (ch_transform.target_node != 0xff'ffu && ch_transform.target_node < transforms.size()) {
							memcpy(&transforms[ch_transform.target_node], &ch_transform, sizeof(TreeAnimator::Transform));
						}
					}

					break;
				}
			}

			if (w < 1.0f) {
				for (size_t n = firstNoneZeroWeightChild + 1u; n < childrenCount; ++n) { // true animation blend variant?
					auto& v = children[n]->value();
					const float w2 = v.getWeight();

					if (w2 <= 0.0f) continue; // skip calculate for weight <= 0.0f

					std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);
					const float mix = w * 1.0f / (w + w2);

                    for (auto& ch_transform : ch_transforms) {
						if (ch_transform.target_node == 0xff'ffu || ch_transform.target_node >= transforms.size()) continue;
						TreeAnimator::Transform& tr0 = transforms[ch_transform.target_node];
						TreeAnimator::Transform& tr1 = ch_transform;

						switch (tr1.mask) {
							case 0b00'00'00'01u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
							}
								break;
							case 0b00'00'00'10u:
							{
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00'00'01'00u:
							{
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00'00'00'11u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00'00'01'01u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00'00'01'10u:
							{
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00'00'01'11u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							default:
								break;
						}

						tr0.mask |= tr1.mask;
						tr1.mask = 0u;
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

			size_t firstNoneZeroWeightChild = 0u;
			for (size_t n = 0u; n < childrenCount; ++n) {
				const auto& v = children[n]->value();
				w = v.getWeight();
				if (w > 0.0f) { // get first none zero weight value
					firstNoneZeroWeightChild = n;
					const std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);

                    for (auto&& ch_transform : ch_transforms) {
						memcpy(&transforms[ch_transform.target_node], &ch_transform, sizeof(TreeAnimator::Transform));
					}

					break;
				}
			}

			if (w < 1.0f) {
				for (size_t n = firstNoneZeroWeightChild + 1; n < childrenCount; ++n) { // true animation blend variant?
					auto& v = children[n]->value();
					const float w2 = v.getWeight();

					if (w2 <= 0.0f) continue; // skip calculate for weight <= 0.0f

					std::vector<TreeAnimator::Transform>& ch_transforms = v.getTransforms(i);
					const float mix = w * 1.0f / (w + w2);

                    for (auto&& ch_transform : ch_transforms) {
						TreeAnimator::Transform& tr0 = transforms[ch_transform.target_node];
						TreeAnimator::Transform& tr1 = ch_transform;

						switch (tr1.mask) {
							case 0b00'00'00'01u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
							}
								break;
							case 0b00'00'00'10u:
							{
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00'00'01'00u:
							{
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00'00'00'11u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
							}
								break;
							case 0b00'00'01'01u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00'00'01'10u:
							{
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							case 0b00'00'01'11u:
							{
								tr0.translation = (tr0.mask & 0b00'00'00'01u) ? glm::mix(tr1.translation, tr0.translation, mix) : tr1.translation;
								tr0.rotation = (tr0.mask & 0b00'00'00'10u) ? glm::normalize(glm::slerp(tr1.rotation, tr0.rotation, mix)) : tr1.rotation;
								tr0.scale = (tr0.mask & 0b00'00'01'00u) ? glm::mix(tr1.scale, tr0.scale, mix) : tr1.scale;
							}
								break;
							default:
								break;
						}

						tr0.mask |= tr1.mask;
						tr1.mask = 0u;
					}

					w += w2;

					if (w >= 1.0f) break;
				}
			}
		}

		return true;
	}
}