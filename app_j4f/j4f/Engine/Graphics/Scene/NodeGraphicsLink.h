#pragma once

#include "../../Core/Math/math.h"

namespace engine {

	class Node;
	struct RenderDescriptor;
	class NodeGraphicsLink;

	class SceneGraphicsObject {
	public:
		virtual ~SceneGraphicsObject() { _nodeGraphicsLink = nullptr; }

		inline const NodeGraphicsLink* getNodeLink() const { return _nodeGraphicsLink; }
		inline void setNodeLink(const NodeGraphicsLink* l) { _nodeGraphicsLink = const_cast<NodeGraphicsLink*>(l); }
	protected:
		NodeGraphicsLink* _nodeGraphicsLink = nullptr;
	};

	class NodeGraphics {
	public:
		virtual ~NodeGraphics() = default;
		NodeGraphics(RenderDescriptor* d) : _descriptor(d) {}
		inline RenderDescriptor* getRenderDescriptor() const { return _descriptor; }

	protected:
		RenderDescriptor* _descriptor = nullptr;
	};

	template <typename T>
	class NodeGraphicsType : public NodeGraphics {
	public:
		NodeGraphicsType(T* g) : NodeGraphics(&g->getRenderDescriptor()), _graphics(g) {}

		~NodeGraphicsType() {
			_descriptor = nullptr;
			delete _graphics;
			_graphics = nullptr;
		}
	private:
		T* _graphics = nullptr;
	};

	class NodeGraphicsLink {
	public:

		NodeGraphicsLink(Node* n, NodeGraphics* g) : _node(n), _graphics(g) {}

		~NodeGraphicsLink() {
			if (_customTransform) {
				delete _customTransform;
				delete _completeTransform;

				_customTransform = nullptr;
				_completeTransform = nullptr;
			}

			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
			}
		}

		inline bool hasCustomTransform() const { return _customTransform != nullptr; }
		const glm::mat4& transform() const;

		void updateCustomTransform(const glm::mat4& tr);
		void updateNodeTransform();

		inline const NodeGraphics* getGraphics() const { return _graphics; }

		inline void setGraphics(NodeGraphics* g) {
			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
			}
			_graphics = g;
		}

	private:
		Node* _node = nullptr;
		NodeGraphics* _graphics = nullptr;
		glm::mat4* _customTransform = nullptr;
		glm::mat4* _completeTransform = nullptr;
	};

}