#pragma once

#include "../../Core/Math/math.h"

namespace engine {

	class Node;
	struct RenderDescriptor;

	class Graphic {
	public:
		virtual ~Graphic() = default;
		inline RenderDescriptor* getRenderDescriptor() const { return _descriptor; }

	protected:
		RenderDescriptor* _descriptor = nullptr;
	};

	template <typename T>
	class NodeGraphic : public Graphic {
	public:
		NodeGraphic(T* g) : _descriptor(&g->getRenderDescriptor()), _graphics(g) {}

		~NodeGraphic() {
			_descriptor = nullptr;
			delete _graphics;
			_graphics = nullptr;
		}
	private:
		T* _graphic = nullptr;
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

		const Graphic* getGraphic() const { return _graphic; }

	private:
		Node* _node = nullptr;
		Graphic* _graphic = nullptr;
		glm::mat4* _customTransform;
		glm::mat4* _completeTransform;
	};

}