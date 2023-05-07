#pragma once

#include "../../Core/Common.h"
#include "../../Core/Math/mathematic.h"
#include <vulkan/vulkan.h>
#include "Node.h"

#include <vector>

namespace vulkan {
	class VulkanGpuProgram;
}

namespace engine {

	struct RenderDescriptor;

	class RenderObject {
	public:
		virtual ~RenderObject() { _descriptor = nullptr; };

		RenderObject() = default;
		explicit RenderObject(RenderDescriptor* d) noexcept : _descriptor(d) {}
		[[nodiscard]] inline RenderDescriptor* getRenderDescriptor() const noexcept { return _descriptor; }
		inline void setNeedUpdate(const bool value) noexcept { _needUpdateData = value; }
        [[nodiscard]] inline bool getNeedUpdate() const noexcept { return _needUpdateData; }

	protected:
		RenderDescriptor* _descriptor = nullptr;
		bool _needUpdateData = false;
	};

	class NodeRenderObject : public RenderObject {
		friend class Node;
	public:
        ~NodeRenderObject() override {
			_node = nullptr;
		}

		inline Node* getNode() { return _node; }
		inline const Node* getNode() const { return _node; }

	protected:
		Node* _node = nullptr;
	};

	template<typename T>
	concept IsGraphicsType = requires(T v) {
		v.getRenderDescriptor();
		v.updateRenderData(mat4f(), bool());
		v.updateModelMatrixChanged(bool());
		v.setProgram([]()->vulkan::VulkanGpuProgram* { return nullptr; }(), VkRenderPass()); // wow!, it work :)
	};

	template <typename T> requires IsGraphicsType<T>
	class NodeRenderer : public NodeRenderObject {
	public:
		using type = T*;

		~NodeRenderer() override {
			if (_graphics && _isGraphicsOwner) {
				delete _graphics;
			}

			_graphics = nullptr;
		}

		NodeRenderer() = default;
		explicit NodeRenderer(type g) : RenderObject(&g->getRenderDescriptor()), _graphics(g) {}

		inline type replaceGraphics(type g, const bool own = true) {
			type oldGraphics = _graphics;
			_isGraphicsOwner = own;
			_descriptor = &g->getRenderDescriptor();
			_graphics = g;
			return oldGraphics;
		}

		inline void setGraphics(type g, const bool own = true) {
			if (_graphics && _isGraphicsOwner) {
				delete _graphics;
			}

			_isGraphicsOwner = own;
			_descriptor = &g->getRenderDescriptor();
			_graphics = g;
		}

		inline void resetGraphics() { _graphics = nullptr; _isGraphicsOwner = false; }

		inline void updateRenderData() {
			if (getNeedUpdate()) {
				if (_graphics && _node) {
					_graphics->updateRenderData(_node->model(), _node->modelChanged());
				}
				setNeedUpdate(false);
			} else {
				if (_graphics && _node) {
					_graphics->updateModelMatrixChanged(_node->modelChanged());
				}
			}
		}

		inline vulkan::VulkanGpuProgram* setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			if (_graphics) {
				return _graphics->setProgram(program, renderPass);
			}
			return nullptr;
		}

		inline type operator->() { return _graphics; }
		inline type operator->() const { return _graphics; }

		inline type graphics() { return _graphics; }
		inline type graphics() const { return _graphics; }

	private:
		bool _isGraphicsOwner = true;
		type _graphics = nullptr;
	};

	template <typename T>
	class GraphicsTypeUpdateSystem {
	public:
		using type = NodeRenderer<T>*;

		GraphicsTypeUpdateSystem() = default;

		~GraphicsTypeUpdateSystem() {
			_objects.clear();
		}

		GraphicsTypeUpdateSystem(GraphicsTypeUpdateSystem&& system) noexcept : _objects(std::move(system._objects)) { system._objects.clear(); }
		GraphicsTypeUpdateSystem& operator= (GraphicsTypeUpdateSystem&& system) noexcept {
			_objects = std::move(system._objects);
			system._objects.clear();
			return *this;
		}

		GraphicsTypeUpdateSystem(const GraphicsTypeUpdateSystem& system) = delete;
		GraphicsTypeUpdateSystem& operator= (const GraphicsTypeUpdateSystem& system) = delete;

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

	private:
		std::vector<type> _objects;
	};
}