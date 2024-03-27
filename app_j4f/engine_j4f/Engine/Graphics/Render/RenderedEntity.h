#pragma once

#include "../Vulkan/vkRenderData.h"
#include "RenderDescriptor.h"

#include <cstdint>
#include <string>
#include <utility>
#include <functional>
#include <string_view>
#include <unordered_map>

namespace engine {

	struct FixedLayoutDescriptor {
		std::string name;
		ViewParams::Ids viewParamIdx = ViewParams::Ids::UNKNOWN;
	};

	class RenderedEntity {
	public:
		virtual ~RenderedEntity() {
			_renderDescriptor.destroy();
		}

		template <typename T>
		inline void setParamByName(std::string_view name, T&& value, bool copyData, const uint32_t count = 1u) {
			for (auto && r_data : _renderDescriptor.renderData) {
                r_data->setParamByName(name, std::forward<T>(value), copyData, count);
			}
		}

		template <typename T>
		inline void setParamForLayout(const vulkan::GPUParamLayoutInfo* info, T&& value, const bool copyData, const uint32_t count = 1u) {
            for (auto && r_data : _renderDescriptor.renderData) {
				r_data->setParamForLayout(info, std::forward<T>(value), copyData, count);
			}
		}

		template <typename T>
		inline void setParamByName(const size_t offset, std::string_view name, T&& value, const uint32_t count = 1u) {
            for (auto && r_data : _renderDescriptor.renderData) {
                r_data->setParamByName(offset, name, std::forward<T>(value), count);
			}
		}

		template <typename T>
		inline void setParamForLayout(const size_t offset, const vulkan::GPUParamLayoutInfo* info, T&& value, const uint32_t count = 1u) {
            for (auto && r_data : _renderDescriptor.renderData) {
                r_data->setParamForLayout(offset, info, std::forward<T>(value), count);
			}
		}

		const vulkan::GPUParamLayoutInfo* getParamLayout(std::string name) const {
			return _renderDescriptor.renderData[0]->getLayout(name);
		}

        inline vulkan::RenderData* getRenderDataAt(const uint16_t n) {
            if (_renderDescriptor.renderData.size() <= n) return nullptr;
            return _renderDescriptor.renderData[n].get();
        }

		inline const vulkan::RenderData* getRenderDataAt(const uint16_t n) const {
			if (_renderDescriptor.renderData.size() <= n) return nullptr;
			return _renderDescriptor.renderData[n].get();
		}

		inline vulkan::VulkanRenderState& renderState() { return _renderState; }
		inline const vulkan::VulkanRenderState& renderState() const { return _renderState; }

		inline void changeRenderState(const std::function<void(vulkan::VulkanRenderState&)>& changer, VkRenderPass renderPass = nullptr) {
			changer(_renderState);
			pipelineAttributesChanged(renderPass);
		}

        inline void changeRenderState(std::function<void(vulkan::VulkanRenderState&)>&& changer, VkRenderPass renderPass = nullptr) {
            changer(_renderState);
            pipelineAttributesChanged(renderPass);
        }

		inline const RenderDescriptor& getRenderDescriptor() const { return _renderDescriptor; }
		inline RenderDescriptor& getRenderDescriptor() { return _renderDescriptor; }

        const std::vector<std::pair<const vulkan::GPUParamLayoutInfo*, FixedLayoutDescriptor>> & getFixedGpuLayouts() const {
            return _fixedGpuLayouts;
        }

		void setPipeline(vulkan::VulkanPipeline* p) {
			if (_renderDescriptor.renderData[0]->pipeline == nullptr || _renderDescriptor.renderData[0]->pipeline->program != p->program) {
				if (auto it = _pipelinesLayoutsMap.find(p); it != _pipelinesLayoutsMap.end()) {
					uint16_t layoutIdx = 0;
					for (auto&& l : _fixedGpuLayouts) {
						l.first = it->second[layoutIdx++];
					}
				} else {
					std::vector<const vulkan::GPUParamLayoutInfo*> layouts;
					for (auto&& l : _fixedGpuLayouts) {
						l.first = p->program->getGPUParamLayoutByName(l.second.name);
						layouts.push_back(l.first);
					}
					_pipelinesLayoutsMap[p] = std::move(layouts);
				}				
			}

            if (_renderDescriptor.mode != RenderDescriptorMode::CUSTOM_DRAW) {
				for (auto&& l : _fixedGpuLayouts) {
					if (l.second.viewParamIdx != ViewParams::Ids::UNKNOWN) {
						_renderDescriptor.setViewParamLayout(l.first, l.second.viewParamIdx);
					}
				}
            }

            for (auto && r_data : _renderDescriptor.renderData) {
                r_data->setPipeline(p);
			}
		}

		inline vulkan::VulkanGpuProgram* setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			if (_renderDescriptor.renderData.empty()) return nullptr;
            //if (std::holds_alternative<std::monostate>(_renderState.vertexDescription.attributes)) return nullptr;

			auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
			vulkan::VulkanPipeline* pipeline = renderer->getGraphicsPipeline(_renderState, program, renderPass);
			vulkan::VulkanPipeline* currentPipeline = _renderDescriptor.renderData[0]->pipeline;

			const vulkan::VulkanGpuProgram* currentProgram = currentPipeline ? currentPipeline->program : nullptr;

			if (currentPipeline == pipeline) return const_cast<vulkan::VulkanGpuProgram*>(currentProgram);

			setPipeline(pipeline);
			return const_cast<vulkan::VulkanGpuProgram*>(currentProgram);
		}

		inline void pipelineAttributesChanged(VkRenderPass renderPass = nullptr) {
            if (!_renderDescriptor.renderData.empty()) {
                setPipeline(
                        Engine::getInstance().getModule<Graphics>().getRenderer()->getGraphicsPipeline(
                                _renderState, _renderDescriptor.renderData[0]->pipeline->program, renderPass)
                );
            }
		}

		inline void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams, const uint16_t drawCount) {
			_renderDescriptor.render(commandBuffer, currentFrame, viewParams, drawCount);
		}

	protected:
		RenderDescriptor _renderDescriptor;
		vulkan::VulkanRenderState _renderState;
		std::vector<std::pair<const vulkan::GPUParamLayoutInfo*, FixedLayoutDescriptor>> _fixedGpuLayouts;
		std::unordered_map<vulkan::VulkanPipeline*, std::vector<const vulkan::GPUParamLayoutInfo*>> _pipelinesLayoutsMap;
	};

    template <typename T>
    class ReferenceEntity : public RenderedEntity {
		using EntityCreator = std::function<bool(ReferenceEntity *)>;
    public:
        explicit ReferenceEntity(T* entity) : _entity(entity) {
			init();
        }

		explicit ReferenceEntity(EntityCreator&& creator) : _creator(creator) {}

        void updateRenderData(RenderDescriptor & renderDescriptor, const mat4f& worldMatrix, const bool worldMatrixChanged) {
			if (_creator && _creator(this)) {
				_creator = nullptr;
			}

			if (!_entity) return;

            _entity->updateRenderData(renderDescriptor, worldMatrix, worldMatrixChanged);
        }

        inline void updateModelMatrixChanged(const bool /*worldMatrixChanged*/) noexcept { }
    
		void init(T* entity) {
			_entity = entity;
			_renderState = _entity->renderState();
			_fixedGpuLayouts = _entity->getFixedGpuLayouts();
			auto const& renderDescriptor = _entity->getRenderDescriptor();
			_renderDescriptor.mode = renderDescriptor.mode;
			_renderDescriptor.customRenderer = renderDescriptor.customRenderer;

			auto const size = renderDescriptor.renderData.size();
			_renderDescriptor.renderData.resize(size);

			for (size_t i = 0u; i < size; ++i) {
				auto const& src = renderDescriptor.renderData[i];
				auto& dst = _renderDescriptor.renderData[i];

				dst = std::make_unique<vulkan::RenderData>();
				dst->vertexes = src->vertexes;
				dst->indexes = src->indexes;

				dst->vertexSize = src->vertexSize;
				dst->batchingParams = src->batchingParams;
				dst->indexType = src->indexType;

				dst->renderPartsCount = src->renderPartsCount;
				dst->renderParts = new vulkan::RenderData::RenderPart[dst->renderPartsCount];
				memcpy(dst->renderParts, src->renderParts, dst->renderPartsCount * sizeof(vulkan::RenderData::RenderPart));
			}
		}

		RenderedEntity* getRenderEntity() noexcept { return this; }

	private:
		EntityCreator _creator = nullptr;
        T* _entity = nullptr;
    };
}