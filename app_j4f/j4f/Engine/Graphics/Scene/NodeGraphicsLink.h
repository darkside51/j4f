#pragma once

#include "../../Core/Math/math.h"
#include <vulkan/vulkan.h>

namespace vulkan {
	class VulkanGpuProgram;
}

namespace engine {

	class Node;
	struct RenderDescriptor;
	class GraphicsLink;

	class RenderObject {
	public:
		virtual ~RenderObject() {
			_descriptor = nullptr;
		};

		RenderObject() = default;
		RenderObject(RenderDescriptor* d) : _descriptor(d) {}
		inline RenderDescriptor* getRenderDescriptor() const { return _descriptor; }

	protected:
		RenderDescriptor* _descriptor = nullptr;
	};

	template <typename T>
	class NodeRenderer : public RenderObject {
	public:
		using type = T*;

		~NodeRenderer() {
			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
			}
			_node = nullptr;
		}

		NodeRenderer() = default;
		NodeRenderer(type g) : NodeGraphics(&g->getRenderDescriptor()), _graphics(g) {}

		inline void setNode(const Node* n) {
			_node = const_cast<Node*>(n);
			_node->setRenderObject(this);
		}

		inline void setNode(const Node& n) {
			_node = &const_cast<Node&>(n);
			_node->setRenderObject(this);
		}

		inline Node* getNode() { return _node; }
		inline const Node* getNode() const { return _node; }

		inline void setGraphics(type g) {
			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
			}

			_descriptor = &g->getRenderDescriptor();
			_graphics = g;
		}

		inline void updateRenderData() {
			if (_graphics && _node) {
				_graphics->updateRenderData(_node->model());
			}
		}

		inline void setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			if (_graphics) {
				_graphics->setProgram(program, renderPass);
			}
		}

		inline type operator->() { return _graphics; }
		inline const type operator->() const { return _graphics; }

		inline type graphics() { return _graphics; }
		inline const type graphics() const { return _graphics; }

	private:
		type _graphics = nullptr;
		Node* _node = nullptr;
	};
}