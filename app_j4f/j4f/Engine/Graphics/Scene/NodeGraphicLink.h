#pragma once

#include "../../Core/Math/math.h"

namespace engine {

	class Node;

	class Graphic {
	public:
		virtual ~Graphic() = default;
	};

	class NodeGraphicLink {
	public:
		~NodeGraphicLink() {
			if (_customTransform) {
				delete _customTransform;
				delete _completeTransform;

				_customTransform = nullptr;
				_completeTransform = nullptr;
			}

			if (_graphic) {
				delete _graphic;
				_graphic = nullptr;
			}
		}

		inline bool hasCustomTransform() const { return _customTransform != nullptr; }
		const glm::mat4& transform() const;

		void updateCustomTransform(const glm::mat4& tr);
		void updateNodeTransform();

	private:
		Node* _node = nullptr;
		Graphic* _graphic = nullptr;
		glm::mat4* _customTransform;
		glm::mat4* _completeTransform;
	};

}