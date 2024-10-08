#pragma once

#include "../../Core/Common.h"
#include "../../Core/Math/mathematic.h"
#include "../../Core/ref_ptr.h"
#include "../../Utils/Debug/Assert.h"
#include "Node.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace vulkan {
	class VulkanGpuProgram;
}

namespace engine {

	struct RenderDescriptor;
    class RenderedEntity;

	class RenderObject {
	public:
		virtual ~RenderObject() { _renderEntity = nullptr; };

		RenderObject() = default;
		[[nodiscard]] RenderDescriptor& getRenderDescriptor() noexcept;
        [[nodiscard]] inline RenderedEntity* getRenderEntity() const noexcept { return _renderEntity; }
		inline void setNeedUpdate(const bool value) noexcept { _needUpdateData = value; }
        [[nodiscard]] inline bool getNeedUpdate() const noexcept { return _needUpdateData; }

	protected:
		bool _needUpdateData = false;
        RenderedEntity* _renderEntity = nullptr;
	};

	class NodeRenderer : public RenderObject {
		friend class Node;
	public:
        ~NodeRenderer() override {
			_node = nullptr;
		}

		inline Node* getNode() { return _node; }
		inline const Node* getNode() const { return _node; }

	protected:
		Node* _node = nullptr;
	};

	template<typename T, typename = std::enable_if_t<is_pointer_v<T> || is_smart_pointer_v<T>>>
	concept IsRenderAvailableTypePointer = requires(T v) {
			v->getRenderDescriptor();
			v->updateRenderData(*static_cast<engine::RenderDescriptor*>(nullptr), mat4f(), bool());
			v->updateModelMatrixChanged(bool());
			v->setProgram(static_cast<vulkan::VulkanGpuProgram *>(nullptr),
							VkRenderPass()); // wow!, it work :)
	};

	template<typename T, typename = std::enable_if_t<!is_pointer_v<T> && !is_smart_pointer_v<T>>>
	concept IsRenderAvailableTypeObject = requires(T v) {
			v.getRenderDescriptor();
			v.updateRenderData(*static_cast<engine::RenderDescriptor*>(nullptr), mat4f(), bool());
			v.updateModelMatrixChanged(bool());
			v.setProgram(static_cast<vulkan::VulkanGpuProgram *>(nullptr),
							VkRenderPass()); // wow!, it work :)
	};

	template<typename T>
	concept IsRenderAvailableType = IsRenderAvailableTypePointer<T> || IsRenderAvailableTypeObject<T>;

	template <typename T> requires IsRenderAvailableType<T>
	class NodeRendererImpl : public NodeRenderer {
	public:
		using type = T;

		~NodeRendererImpl() override {
            if constexpr (std::is_pointer_v<type>) {
                if (_graphics && _isGraphicsOwner) {
                    delete _graphics;
                }
            }

			_graphics = nullptr;
            _renderEntity = nullptr;
		}

		NodeRendererImpl() = default;
		explicit NodeRendererImpl(type&& g) : RenderObject(&g->getRenderDescriptor()), _graphics(g) {
            _renderEntity = _graphics->getRenderEntity();
        }

        template <typename Type = type>
		inline void setGraphics(Type&& g, const bool own = true) {
            if constexpr (std::is_pointer_v<type>) {
                if (_graphics && _isGraphicsOwner) {
                    delete _graphics;
                }
            }

			_isGraphicsOwner = own;
			_graphics = std::forward<Type>(g);
            _renderEntity = _graphics->getRenderEntity();
		}

		inline void resetGraphics() { _graphics = nullptr; _isGraphicsOwner = false; _renderEntity = nullptr; }

		inline void updateRenderData() {
			if (getNeedUpdate()) {
				if (_graphics && _node) {
					_graphics->updateRenderData(getRenderDescriptor(), _node->model(), _node->modelChanged());
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

		inline type& operator->() { return _graphics; }
		inline const type& operator->() const { return _graphics; }

		inline type& graphics() { return _graphics; }
		inline const type& graphics() const { return _graphics; }

	private:
		bool _isGraphicsOwner = true;
		type _graphics = nullptr;
	};

    class IGraphicsDataUpdateSystem {
    public:
        virtual ~IGraphicsDataUpdateSystem() = default;
        virtual void updateRenderData() = 0;
    };

	template <typename T>
	class GraphicsDataUpdateSystem final : public IGraphicsDataUpdateSystem {
	public:
		using type = NodeRendererImpl<T>*;

        GraphicsDataUpdateSystem() = default;

		~GraphicsDataUpdateSystem() override {
			_objects.clear();
		}

        GraphicsDataUpdateSystem(GraphicsDataUpdateSystem&& system) noexcept : _objects(std::move(system._objects)) { system._objects.clear(); }
        GraphicsDataUpdateSystem& operator= (GraphicsDataUpdateSystem&& system) noexcept {
			_objects = std::move(system._objects);
			system._objects.clear();
			return *this;
		}

        GraphicsDataUpdateSystem(const GraphicsDataUpdateSystem& system) = delete;
        GraphicsDataUpdateSystem& operator= (const GraphicsDataUpdateSystem& system) = delete;

		inline void registerObject(type o) {
			_objects.push_back(o);
		}

		inline void unregisterObject(type o) {
			_objects.erase(std::remove(_objects.begin(), _objects.end(), o), _objects.end());
		}

		inline void updateRenderData() override {
			for (auto&& o : _objects) {
				o->updateRenderData();
			}
		}

	private:
		std::vector<type> _objects;
	};

    class GraphicsDataUpdater final {
    public:
        template <typename T>
        inline void registerSystem(T&& s) {
			static const auto id =
                    UniqueTypeId<IGraphicsDataUpdateSystem>::getUniqueId<std::remove_pointer_t<std::decay_t<T>>>();
			if (_systems.size() <= id) {
				_systems.resize(id + 1u);
			} else if (_systems[id] != nullptr) {
				ENGINE_BREAK_CONDITION(false);
				return;
			}

			if constexpr (std::is_pointer_v<T>) {
				_systems[id] = s;
			} else {
				_systems[id] = &s;
			}
        }

        template <typename T>
        inline void unregisterSystem(T&& s) noexcept {
            static const auto id = UniqueTypeId<IGraphicsDataUpdateSystem>::getUniqueId<T>();
            if (_systems.size() > id) {
                _systems[id] = nullptr;
            }
        }

        void updateData() {
            for (auto && s: _systems) {
                if (s) {
                    s->updateRenderData();
                }
            }
        }

        template<typename T, typename... Args>
        inline void updateData() {
            update_data_strict<T>();
            updateData<Args...>();
        }

    private:
        template<typename... Args>
        inline typename std::enable_if<sizeof...(Args) == 0>::type updateData() noexcept {}

        template<typename T>
        inline void update_data_strict() {
            static const auto id = UniqueTypeId<IGraphicsDataUpdateSystem>::getUniqueId<std::decay_t<T>>();
            if (_systems.size() > id) {
                static_cast<T*>(_systems[id].get())->updateRenderData();
            }
        }

        std::vector<ref_ptr<IGraphicsDataUpdateSystem>> _systems;
    };
}