#include "Node.h"
#include "NodeGraphicsLink.h"

namespace engine {

	Node::~Node() {
		if (_graphics) {
			delete _graphics;
			_graphics = nullptr;
		}

		if (_boundingVolume) {
			delete _boundingVolume;
			_boundingVolume = nullptr;
		}
	}

	void Node::setRenderObject(const NodeRenderObject* r) {
		if (_graphics) {
			delete _graphics;
		}
		_graphics = const_cast<NodeRenderObject*>(r);
		_graphics->_node = const_cast<Node*>(this);
	}

}