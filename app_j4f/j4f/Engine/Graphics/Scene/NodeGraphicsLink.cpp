#include "NodeGraphicsLink.h"
#include "Node.h"

namespace engine {

	const glm::mat4& GraphicsLink::transform() const {
		return _completeTransform ? *_completeTransform : _node->model();
	}

	void GraphicsLink::updateCustomTransform(const glm::mat4& tr) {
		if (_customTransform == nullptr) {
			_customTransform = new glm::mat4(1.0f);
			_completeTransform = new glm::mat4(1.0f);
		}

		if (memcmp(_customTransform, &tr, sizeof(glm::mat4))) {
			memcpy(_customTransform, &tr, sizeof(glm::mat4));
			*_completeTransform = _node->model() * (*_customTransform);
		}
	}

	void GraphicsLink::updateNodeTransform() {
		if (_customTransform == nullptr) return;
		*_completeTransform = _node->model() * (*_customTransform);
	}
}