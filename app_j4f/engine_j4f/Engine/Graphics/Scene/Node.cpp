#include "Node.h"
#include "NodeGraphicsLink.h"

namespace engine {

	Node::Node(NodeRenderer* graphics) : _renderer(graphics) {
		_renderer->_node = const_cast<Node*>(this);
	}

	Node::~Node() {
		if (_renderer) {
			delete _renderer;
			_renderer = nullptr;
		}

		if (_boundingVolume) {
			delete _boundingVolume;
			_boundingVolume = nullptr;
		}
	}

	void Node::setRenderer(const NodeRenderer* r) {
		if (_renderer) {
			delete _renderer;
		}
		_renderer = const_cast<NodeRenderer*>(r);
		_renderer->_node = const_cast<Node*>(this);
	}

}