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
			_link = nullptr;
		}

		NodeRenderer() = default;
		NodeRenderer(type g) : NodeGraphics(&g->getRenderDescriptor()), _graphics(g) {}

		inline GraphicsLink* getNodeLink() { return _link; }
		inline const GraphicsLink* getNodeLink() const { return _link; }

		inline void setNodeLink(const GraphicsLink* l) {
			_link = const_cast<GraphicsLink*>(l);
			_link->setGraphics(this);
		}

		inline void setGraphics(type g) {
			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
			}

			_descriptor = &g->getRenderDescriptor();
			_graphics = g;
		}

		inline void updateRenderData() {
			if (_graphics && _link) {
				_graphics->updateRenderData(_link->transform());
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
		GraphicsLink* _link = nullptr;
	};

	class GraphicsLink {
	public:
		GraphicsLink(Node* n, RenderObject* g = nullptr) : _node(n), _graphics(g) {}

		~GraphicsLink() {
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

		inline const RenderObject* getGraphics() const { return _graphics; }

		inline void setGraphics(RenderObject* g) {
			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
			}
			_graphics = g;
		}

		inline Node* getNode() { return _node; }
		inline const Node* getNode() const { return _node; }

	private:
		Node* _node = nullptr;
		RenderObject* _graphics = nullptr;
		glm::mat4* _customTransform = nullptr;
		glm::mat4* _completeTransform = nullptr;
	};

}