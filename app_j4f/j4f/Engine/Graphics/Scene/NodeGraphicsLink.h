#pragma once

#include "../../Core/Math/math.h"
#include <vulkan/vulkan.h>

#include <vector>

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

	template<typename T>
	concept IsGraphicsType = requires(T v) {
		v.getRenderDescriptor();
		v.updateRenderData(glm::mat4(), bool());
		v.setProgram([]()->vulkan::VulkanGpuProgram* {}(), VkRenderPass()); // wow!, it work :)
	};

	template <typename T> requires IsGraphicsType<T>
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
		NodeRenderer(type g) : RenderObject(&g->getRenderDescriptor()), _graphics(g) {}

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
				_graphics->updateRenderData(_node->model(), _node->modelChanged());
			}
		}

		inline bool updateRenderDataIfVisible(const uint8_t visibleId) {
			if (_graphics && _node && _node->isVisible(visibleId)) {
				_graphics->updateRenderData(_node->model(), _node->modelChanged());
				return true;
			}
			return false;
		}

		inline void setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			if (_graphics) {
				_graphics->setProgram(program, renderPass);
			}
		}

		inline type& operator->() { return _graphics; }
		inline const type& operator->() const { return _graphics; }

		inline type& graphics() { return _graphics; }
		inline const type& graphics() const { return _graphics; }

	private:
		type _graphics = nullptr;
		Node* _node = nullptr;
	};

	template <typename T>
	class GraphicsTypeUpdateSystem {
	public:
		using type = NodeRenderer<T>*;

		GraphicsTypeUpdateSystem() = default;

		~GraphicsTypeUpdateSystem() {
			_objects.clear();
		}

		inline void registerObject(type o) {
			_objects.push_back(o);
		}

		inline void unregisterObject(type o) {
			_objects.erase(std::remove(_objects.begin(), _objects.end(), o), _objects.end());
		}

		inline void updateRenderData() {
			for (auto&& o : _objects) {
				o->updateRenderData();
			}
		}

		inline void updateRenderDataIfVisible(const uint8_t visibleId) {
			for (auto&& o : _objects) {
				o->updateRenderDataIfVisible(visibleId);
			}
		}

	private:
		std::vector<type> _objects;
	};
}