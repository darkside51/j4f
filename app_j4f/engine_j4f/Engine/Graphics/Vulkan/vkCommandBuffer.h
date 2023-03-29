#pragma once

#include "vkBuffer.h"
#include "vkImage.h"
#include "vkSynchronisation.h"
#include "vkPipeline.h"
#include "vkDescriptorSet.h"

#include "../../Core/Threads/Synchronisations.h"
#include "../../Core/Engine.h"
#include "../../Utils/Statistic.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <type_traits>
#include <algorithm>
#include <cassert>
#include <optional>

namespace vulkan {

	struct EmptyState {};

	struct VulkanCommandBufferState {
    private:
        static inline constexpr float invalid_float = 0xffffffff;

    public:

		struct BindingSets {
		private:
			static inline constexpr uint8_t descriptor_sets_max_count = 8;
			static inline constexpr uint8_t dynamic_offsets_max_count = 8;

		public:
			struct Descriptors {
				uint32_t dynamicOffsetCount;
				VkDescriptorSet descriptorSets[descriptor_sets_max_count];
				uint32_t pDynamicOffsets[descriptor_sets_max_count];
			};
			
			std::unordered_map<VkPipelineLayout, Descriptors*> layoutDescriptors;

			~BindingSets() {
                for (auto&& [pipelineLayout, descriptors] : layoutDescriptors) {
					delete descriptors;
				}
			}

			inline void invalidate() {
                for (auto&& [pipelineLayout, descriptors] : layoutDescriptors) {
					for (auto&& descriptorSet : descriptors->descriptorSets) {
                        descriptorSet = VK_NULL_HANDLE;
					}
                    descriptors->dynamicOffsetCount = 0;
				}
			}
		};

		struct NeedBindDescriptors {
			uint8_t firstSet = 0;
			uint8_t setsCount = 0;
			uint8_t dynamicOffsetsCount = 0;
			std::array<uint32_t, 8> dynamicOffsets{};
			std::array<VkDescriptorSet, 8> sets{};

			NeedBindDescriptors() = default;

			NeedBindDescriptors(NeedBindDescriptors&& rvalue) noexcept :
				firstSet(rvalue.firstSet),
				setsCount(rvalue.setsCount),
				dynamicOffsetsCount(rvalue.dynamicOffsetsCount),
				dynamicOffsets(rvalue.dynamicOffsets),
				sets(rvalue.sets) {
			}

			NeedBindDescriptors& operator= (NeedBindDescriptors&& rvalue) noexcept {
				firstSet = rvalue.firstSet;
				setsCount = rvalue.setsCount;
				dynamicOffsetsCount = rvalue.dynamicOffsetsCount;
				dynamicOffsets = rvalue.dynamicOffsets;
				sets = rvalue.sets;
				return *this;
			}

			NeedBindDescriptors(const NeedBindDescriptors& lvalue) = delete;
			NeedBindDescriptors& operator= (const NeedBindDescriptors& lvalue) = delete;
		};

		VkPipelineBindPoint m_pipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkViewport m_viewport = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
		VkRect2D m_scissor = { {0, 0}, {0, 0} };

		VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
		VkDeviceSize m_vertexBufferOffset = 0;

		VkBuffer m_indexBuffer = VK_NULL_HANDLE;
		VkDeviceSize m_indexBufferOffset = 0;
		VkIndexType m_indexBufferType = VK_INDEX_TYPE_MAX_ENUM;

		float m_depthBiasConstantFactor = invalid_float;
		float m_depthBiasClamp = invalid_float;
		float m_depthBiasSlopeFactor = invalid_float;
        float m_lineWidth = invalid_float;

		BindingSets m_bindSets[3];

		bool processed = false;

		inline void clear() {
			m_pipeline = VK_NULL_HANDLE;

			m_vertexBuffer = VK_NULL_HANDLE;
			m_vertexBufferOffset = 0;

			m_indexBuffer = VK_NULL_HANDLE;
			m_indexBufferOffset = 0;
			m_indexBufferType = VK_INDEX_TYPE_MAX_ENUM;

			memset(&m_viewport, 0, sizeof(VkViewport));
			memset(&m_scissor, 0, sizeof(VkRect2D));

			m_depthBiasConstantFactor = invalid_float;
			m_depthBiasClamp = invalid_float;
			m_depthBiasSlopeFactor = invalid_float;
            m_lineWidth = invalid_float;

			m_bindSets[0].invalidate();
			m_bindSets[1].invalidate();
			m_bindSets[2].invalidate();
		}

		inline bool begin() {
			if (processed) return false;

			processed = true;
			clear();
	
			return true;
		}

		inline void end() { processed = false; }

		inline void reset() {
			clear();
			processed = false;
		}

		inline bool setViewPort(const VkViewport& viewport) { 
			if ((m_viewport.width != viewport.width) ||
				(m_viewport.height != viewport.height) ||
				(m_viewport.x != viewport.x) ||
				(m_viewport.y != viewport.y) ||
				(m_viewport.minDepth != viewport.minDepth) ||
				(m_viewport.maxDepth != viewport.maxDepth)) {
				memcpy(&m_viewport, &viewport, sizeof(VkViewport));
				return true;
			}
			return false;
		}

		inline bool setScissor(const VkRect2D& scissor) { 
			if ((m_scissor.extent.width != scissor.extent.width) ||
				(m_scissor.extent.height != scissor.extent.height) ||
				(m_scissor.offset.x != scissor.offset.x) ||
				(m_scissor.offset.y != scissor.offset.y) ) {
				memcpy(&m_scissor, &scissor, sizeof(VkRect2D));
				return true;
			}
			return false;
		}

        inline VkRect2D getCurrentScissor() const { return m_scissor; }

		inline bool setDepthBias(const float depthBiasConstantFactor, const float depthBiasClamp, const float depthBiasSlopeFactor) {
			if (m_depthBiasConstantFactor != depthBiasConstantFactor ||
				m_depthBiasClamp != depthBiasClamp ||
				m_depthBiasSlopeFactor != depthBiasSlopeFactor) {
				m_depthBiasConstantFactor = depthBiasConstantFactor;
				m_depthBiasClamp = depthBiasClamp;
				m_depthBiasSlopeFactor = depthBiasSlopeFactor;
				return true;
			}

			return false;
		}

        inline bool setLineWidth(const float lineWidth) {
            if (m_lineWidth != lineWidth) {
                m_lineWidth = lineWidth;
                return true;
            }

            return false;
        }

		inline bool bindPipeline(const VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) { 
			if (/*m_pipelineBindPoint != pipelineBindPoint ||*/ m_pipeline != pipeline) {
				m_pipelineBindPoint = pipelineBindPoint;
				m_pipeline = pipeline;
				return true;
			}
			return false;
		}

		inline bool bindVertexBuffers(VkBuffer* buffers, const VkDeviceSize* offsets, const uint32_t first, const uint32_t count) {
			// this check so ugly
			return true;
		}

		inline bool bindVertexBuffer(const VkBuffer& buffer, const VkDeviceSize offset) {
			if (m_vertexBuffer != buffer || m_vertexBufferOffset != offset) {
				m_vertexBuffer = buffer;
				m_vertexBufferOffset = offset;
				return true;
			}
			return false;
		}

		inline bool bindIndexBuffer(const VkBuffer& indices, const VkDeviceSize offset, const VkIndexType type) { 
			if (m_indexBuffer != indices || m_indexBufferOffset != offset || m_indexBufferType != type) {
				m_indexBuffer = indices;
				m_indexBufferOffset = offset;
				m_indexBufferType = type;
				return true;
			}
			return false;
		}

		inline bool bindDescriptorSet(
			const VkPipelineBindPoint pipelineBindPoint,
			const VkPipelineLayout layout,
			const uint32_t set,
			const VkDescriptorSet descriptorSet,
			const uint32_t dynamicOffsetCount = 0,
			const uint32_t* pDynamicOffsets = nullptr
		) {
			// todo: разобраться! как - то не так это работает
			//return true;

			uint8_t bindSetNum;
			switch (pipelineBindPoint) {
				case VK_PIPELINE_BIND_POINT_GRAPHICS:
				{
					bindSetNum = 0;
				}
					break;
				case VK_PIPELINE_BIND_POINT_COMPUTE:
				{
					bindSetNum = 1;
				}
					break;
				case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
				{
					bindSetNum = 2;
				}
					break;
				default:
				{
					assert(false);
					return false;
				}
					break;
			}

			BindingSets& bindSet = m_bindSets[bindSetNum];
			BindingSets::Descriptors* descriptors;

			auto it = bindSet.layoutDescriptors.find(layout);
			if (it != bindSet.layoutDescriptors.end()) {
				descriptors = it->second;
			} else {
				descriptors = new BindingSets::Descriptors();
				bindSet.layoutDescriptors[layout] = descriptors;
			}

			VkDescriptorSet& bindedDescriptorSet = descriptors->descriptorSets[set];
			if (bindedDescriptorSet != descriptorSet) {
				bindedDescriptorSet = descriptorSet;
				return true;
			} else {
				
			}

			return false;
		}

		std::array<NeedBindDescriptors, 8> bindDescriptorSets(
			const uint32_t frame,
			const VkPipelineBindPoint pipelineBindPoint,
			const VulkanPipeline* pipeline,
			const uint32_t firstSet,							// номер первого сет привязки
			const uint8_t dynamicOffsetsCount,					// количество оффсетов для динамических буфферов
			const uint32_t* dynamicOffsets,						// оффсеты для динамических буфферов
			const uint8_t externalSetsCount,					// количество дополнительных сетов, которые хочется привязать
			const VkDescriptorSet* externalSets					// дополнительные сеты, которые хочется привязать
		) {
			std::array<NeedBindDescriptors, 8> dirtyBind;
			dirtyBind[0].firstSet = firstSet;

			uint8_t bindSetNum;
			switch (pipelineBindPoint) {
				case VK_PIPELINE_BIND_POINT_GRAPHICS:
				{
					bindSetNum = 0;
				}
					break;
				case VK_PIPELINE_BIND_POINT_COMPUTE:
				{
					bindSetNum = 1;
				}
					break;
				case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
				{
					bindSetNum = 2;
				}
					break;
				default:
				{
					assert(false);
					return dirtyBind;
				}
				break;
			}

			BindingSets& bindSet = m_bindSets[bindSetNum];
			BindingSets::Descriptors* descriptors;

            VkPipelineLayout layout = pipeline->program->getPipeLineLayout();

			auto it = bindSet.layoutDescriptors.find(layout);
			if (it != bindSet.layoutDescriptors.end()) {
				descriptors = it->second;

				if (m_pipelineLayout != layout) {
					bindSet.invalidate(); // если layout для pipeline не совпадает и сет уже есть в bindSet.layoutDescriptors
				}

			} else {
				descriptors = new BindingSets::Descriptors();
				bindSet.layoutDescriptors[layout] = descriptors;
			}

			m_pipelineLayout = layout;

			uint8_t allProgramSetsCount;
			const uint8_t gpuSetsCount = pipeline->program->getGPUSetsCount(&allProgramSetsCount);
			const uint8_t buffersSetsTypes = pipeline->program->getGPUBuffersSetsTypes();
			const uint8_t setsCount = std::min(static_cast<uint8_t>(gpuSetsCount + externalSetsCount), allProgramSetsCount) - firstSet;

			uint8_t dynamicBufferNum = 0;
			uint8_t currentBindDescriptorsCount = 0;
			uint8_t currentBind = 0;

			for (uint8_t i = 0; i < setsCount; ++i) {
				const uint8_t set = firstSet + i;
				
				VkDescriptorSet& bindedDescriptorSet = descriptors->descriptorSets[set];

				const bool isBufferDynamic = (buffersSetsTypes & (1 << set));
				const VulkanDescriptorSet* dset = pipeline->program->getDescriptorSet(i);
				const VkDescriptorSet& currentSet = ((dset != nullptr) ? dset->operator[](frame) : *externalSets++);

				const uint32_t bufferOffset = isBufferDynamic ? dynamicOffsets[dynamicBufferNum++] : 0;

				if (bindedDescriptorSet != currentSet) {
					bindedDescriptorSet = currentSet;
					descriptors->pDynamicOffsets[set] = bufferOffset;

					if (isBufferDynamic) {
						dirtyBind[currentBind].dynamicOffsets[dirtyBind[currentBind].dynamicOffsetsCount++] = bufferOffset;
					}
					dirtyBind[currentBind].sets[dirtyBind[currentBind].setsCount++] = currentSet;
					++currentBindDescriptorsCount;
				} else if (descriptors->pDynamicOffsets[set] != bufferOffset) {
					descriptors->pDynamicOffsets[set] = bufferOffset;

					if (isBufferDynamic) {
						dirtyBind[currentBind].dynamicOffsets[dirtyBind[currentBind].dynamicOffsetsCount++] = bufferOffset;
					}

					dirtyBind[currentBind].sets[dirtyBind[currentBind].setsCount++] = currentSet;
					++currentBindDescriptorsCount;
				} else {
					if (currentBindDescriptorsCount > 0) {
						currentBindDescriptorsCount = 0;
						dirtyBind[++currentBind].firstSet = set + 1;
					} else {
						++dirtyBind[currentBind].firstSet;
					}
				}
			}

			return dirtyBind;
		}

	};

	template <typename STATE>
	struct VulkanCommandBufferEx {
		using state_type = STATE;

	private:
		inline static constexpr bool stated = !std::is_same<state_type, EmptyState>::value;

	public:
		state_type state;
		VulkanCommandBufferEx() = default;

		VulkanCommandBufferEx(VkDevice device, VkCommandPool pool, const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) :
			m_device(device), m_pool(pool) {

			VkCommandBufferAllocateInfo allocateInfo;
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.pNext = nullptr;
			allocateInfo.commandPool = pool;
			allocateInfo.level = level;
			allocateInfo.commandBufferCount = 1;

			engine::AtomicLock lock(_lockAllocation);
			vkAllocateCommandBuffers(m_device, &allocateInfo, &m_commandBuffer);
		}

		VulkanCommandBufferEx(VulkanCommandBufferEx&& cmdBuff) noexcept : 
			m_pool(cmdBuff.m_pool),
			m_commandBuffer(cmdBuff.m_commandBuffer),
			m_device(cmdBuff.m_device)
		{
			if constexpr (stated) { state = cmdBuff.state; }

			cmdBuff.m_commandBuffer = VK_NULL_HANDLE;
			cmdBuff.m_pool = VK_NULL_HANDLE;
			cmdBuff.m_device = VK_NULL_HANDLE;
		}

		VulkanCommandBufferEx& operator= (VulkanCommandBufferEx&& cmdBuff) noexcept {
			m_pool = cmdBuff.m_pool;
			m_commandBuffer = cmdBuff.m_commandBuffer;
			m_device = cmdBuff.m_device;

			if constexpr (stated) { state = cmdBuff.state; }

			cmdBuff.m_commandBuffer = VK_NULL_HANDLE;
			cmdBuff.m_pool = VK_NULL_HANDLE;
			cmdBuff.m_device = VK_NULL_HANDLE;

			return *this;
		}

		VulkanCommandBufferEx(const VulkanCommandBufferEx& cmdBuff) = delete;
		VulkanCommandBufferEx& operator= (const VulkanCommandBufferEx& cmdBuff) = delete;

		~VulkanCommandBufferEx() {
			if (m_commandBuffer) {
				engine::AtomicLock lock(_lockAllocation);
				vkFreeCommandBuffers(m_device, m_pool, 1, &m_commandBuffer);
			}
		}

		VkResult begin(VkCommandBufferUsageFlags flags = 0, const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr) {
			if constexpr (stated) { 
				if (!state.begin()) return VK_NOT_READY;
			}

			VkCommandBufferBeginInfo beginInfo;
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = flags;
			beginInfo.pInheritanceInfo = pInheritanceInfo;
			return vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
		}

		VkResult end() { 
			if constexpr (stated) { state.end();}
			return vkEndCommandBuffer(m_commandBuffer);
		}

		VkResult reset(VkCommandBufferResetFlags flags) { 
			if constexpr (stated) { state.reset(); }
			return vkResetCommandBuffer(m_commandBuffer, flags);
		}

		void flush(VkQueue queue) {
			vkEndCommandBuffer(m_commandBuffer);

			VkSubmitInfo submitInfo;
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_commandBuffer;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.signalSemaphoreCount = 0;

			// create fence to ensure that the command buffer has finished executing
			VkFenceCreateInfo fenceInfo;
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.pNext = nullptr;
			fenceInfo.flags = 0; // unsignaled state

			VkFence fence;
			vkCreateFence(m_device, &fenceInfo, nullptr, &fence);
			// submit to the queue
			vkQueueSubmit(queue, 1, &submitInfo, fence);
			// wait for the fence to signal that command buffer has finished executing
			vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
			vkDestroyFence(m_device, fence, nullptr);
		}

		VkSubmitInfo prepareToSubmit(const VkPipelineStageFlags* waitDstStageMask, const uint32_t waitSemCount, const VkSemaphore* waitSemaphores, const uint32_t sigSemCount,  const VkSemaphore* signalSemaphores) const {
			VkSubmitInfo submitInfo;
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_commandBuffer;
			submitInfo.pWaitDstStageMask = waitDstStageMask;
			submitInfo.waitSemaphoreCount = waitSemCount;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.signalSemaphoreCount = sigSemCount;
			submitInfo.pSignalSemaphores = signalSemaphores;

			return submitInfo;
		}

		//// commands
		template <typename CMD, typename ...ARGS>
		inline void addCommand(CMD&& command, ARGS&&... args) {
			command(std::forward<ARGS>(args)...);
		}

		template <typename CMD, typename ...ARGS>
		inline void addCommand(CMD&& command, ARGS&&... args) const {
			command(std::forward<ARGS>(args)...);
		}

		void cmdBeginRenderPass(VkRenderPass renderPass, VkRect2D renderArea, const VkClearValue* clearValues, const uint8_t clearValuesCount, VkFramebuffer frameBuffer, const VkSubpassContents contentsType) const {
			VkRenderPassBeginInfo renderPassBeginInfo;
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;

			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = frameBuffer;
			renderPassBeginInfo.renderArea = renderArea;
			renderPassBeginInfo.clearValueCount = clearValuesCount;
			renderPassBeginInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(m_commandBuffer, &renderPassBeginInfo, contentsType);
		}

		void cmdBeginRenderPass(const VkRenderPassBeginInfo& beginInfo, const VkSubpassContents contentsType) const {
			vkCmdBeginRenderPass(m_commandBuffer, &beginInfo, contentsType);
			//addCommand(vkCmdBeginRenderPass, m_commandBuffer, &beginInfo, contentsType);
		}

		void cmdEndRenderPass() const { vkCmdEndRenderPass(m_commandBuffer); }

		inline bool cmdSetViewport(const VkViewport& viewport) {
			if constexpr (stated) { 
				if (state.setViewPort(viewport)) {
					vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
					return true;
				}
			} else {
				vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
				return true;
			}
			return false;
		}

		inline void cmdSetViewport(const float x, const float y, const float w, const float h, const float minDepth = 0.0f, const float maxDepth = 1.0f, const bool useNegative = true) {
			// using negative viewport height
			VkViewport viewport;
			if (useNegative) {
				viewport.x = x;
				viewport.y = h - y;
				viewport.width = w;
				viewport.height = -h;
				viewport.minDepth = minDepth;
				viewport.maxDepth = maxDepth;
			} else {
				viewport.x = x;
				viewport.y = y;
				viewport.width = w;
				viewport.height = h;
				viewport.minDepth = minDepth;
				viewport.maxDepth = maxDepth;
			}

			cmdSetViewport(viewport);
		}

		inline bool cmdSetScissor(const VkRect2D& scissor) {
			if constexpr (stated) {
				if (state.setScissor(scissor)) {
					vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
					return true;
				}
			} else {
				vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
				return true;
			}
			return false;
		}

		inline bool cmdSetScissor(const int32_t x, const int32_t y, const uint32_t w, const uint32_t h) {
			VkRect2D scissor = {};
			scissor.offset.x = x;
			scissor.offset.y = y;
			scissor.extent.width = w;
			scissor.extent.height = h;
			return cmdSetScissor(scissor);
		}

		inline bool cmdSetDepthBias(const float depthBiasConstantFactor, const float depthBiasClamp, const float depthBiasSlopeFactor) {
			if constexpr (stated) {
				if (state.setDepthBias(depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor)) {
					vkCmdSetDepthBias(m_commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
					return true;
				}
			} else {
				vkCmdSetDepthBias(m_commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
				return true;
			}
			return false;
		}

        inline bool cmdSetLineWidth(const float lineWidth) {
            if constexpr (stated) {
                if (state.setLineWidth(lineWidth)) {
                    vkCmdSetLineWidth(m_commandBuffer, lineWidth);
                    return true;
                }
            } else {
                vkCmdSetLineWidth(m_commandBuffer, lineWidth);
                return true;
            }
            return false;
        }

		inline void cmdBindDescriptorSet(const VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
			const uint32_t set,
			VkDescriptorSet descriptorSet,
			const uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffsets = nullptr) {
			if constexpr (stated) {
				if (state.bindDescriptorSet(pipelineBindPoint, layout, set, descriptorSet, dynamicOffsetCount, pDynamicOffsets)) {
					vkCmdBindDescriptorSets(m_commandBuffer, pipelineBindPoint, layout, set, 1, &descriptorSet, dynamicOffsetCount, pDynamicOffsets);
				}
			} else {
				vkCmdBindDescriptorSets(m_commandBuffer, pipelineBindPoint, layout, set, 1, &descriptorSet, dynamicOffsetCount, pDynamicOffsets);
			}
		}

		inline void cmdBindDescriptorSets(const VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
								const uint32_t firstSet, const uint32_t descriptorSetCount,
								const VkDescriptorSet* pDescriptorSets,
								const uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffsets = nullptr) {
			vkCmdBindDescriptorSets(m_commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
		}

		inline void cmdPushConstants(const VkPipelineLayout layout, const VkShaderStageFlags stages, const uint32_t offset, const uint32_t size, const void* values) const {
			vkCmdPushConstants(m_commandBuffer, layout, stages, offset, size, values);
		}

		inline void cmdBindPipeline(const VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
			if constexpr (stated) {
				if (state.bindPipeline(pipelineBindPoint, pipeline)) {
					vkCmdBindPipeline(m_commandBuffer, pipelineBindPoint, pipeline);
				}
			} else {
				vkCmdBindPipeline(m_commandBuffer, pipelineBindPoint, pipeline);
			}
		}

		inline void cmdBindVertexBuffers(const VkBuffer* buffers, const VkDeviceSize* offsets, const uint32_t first, const uint32_t count) {
			if constexpr (stated) {
				if (state.bindVertexBuffers(buffers, offsets, first, count)) {
					vkCmdBindVertexBuffers(m_commandBuffer, first, count, buffers, offsets);
				}
			} else {
				vkCmdBindVertexBuffers(m_commandBuffer, first, count, buffers, offsets);
			}
		}

		inline void cmdBindVertexBuffer(const VulkanBuffer& buffer, const VkDeviceSize offset) {
			if constexpr (stated) {
				if (state.bindVertexBuffer(buffer.m_buffer, offset)) {
					vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &buffer.m_buffer, &offset);
				}
			} else {
				vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &buffer.m_buffer, &offset);
			}
		}

		inline void cmdBindIndexBuffer(const VkBuffer& indices, const VkDeviceSize offset, const VkIndexType type = VK_INDEX_TYPE_UINT16) {
			if constexpr (stated) {
				if (state.bindIndexBuffer(indices, offset, type)) {
					vkCmdBindIndexBuffer(m_commandBuffer, indices, offset, type);
				}
			} else {
				vkCmdBindIndexBuffer(m_commandBuffer, indices, offset, type);
			}
		}

		inline void cmdBindIndexBuffer(const VulkanBuffer& indices, const VkDeviceSize offset, const VkIndexType type = VK_INDEX_TYPE_UINT16) {
			cmdBindIndexBuffer(indices.m_buffer, offset, type);
		}

		inline void cmdDrawIndexed(const uint32_t indexCount, const uint32_t firstIndex = 0, const uint32_t instanceCount = 1, const int32_t vertexOffset = 0, const uint32_t firstInstance = 0) const {
			vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
			STATISTIC_ADD_DRAW_CALL
		}

		inline void cmdDraw(const uint32_t vertexCount, const uint32_t firstVertex = 0, const uint32_t instanceCount = 1, const uint32_t firstInstance = 0) {
			vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
			STATISTIC_ADD_DRAW_CALL
		}

		inline void cmdCopyBuffer(const VulkanBuffer& src, const VulkanBuffer& dst, const VkBufferCopy* copyRegion) const {
			if (copyRegion == nullptr) {
				const VkBufferCopy bufferCopy = { 0, 0, src.m_size };
				vkCmdCopyBuffer(m_commandBuffer, src.m_buffer, dst.m_buffer, 1, &bufferCopy);
			} else {
				vkCmdCopyBuffer(m_commandBuffer, src.m_buffer, dst.m_buffer, 1, copyRegion);
			}
		}

		inline void cmdCopyBuffer(const VulkanBuffer& src, const VulkanBuffer& dst, const size_t srcOffset, const size_t dstOffset, const size_t size) const {
			const VkBufferCopy bufferCopy = { srcOffset, dstOffset, size };
			vkCmdCopyBuffer(m_commandBuffer, src.m_buffer, dst.m_buffer, 1, &bufferCopy);
		}

		inline void cmdCopyBufferToImage(const VulkanBuffer& src, const VulkanImage& img, const VkImageLayout dstLayout, const uint32_t regionCount, const VkBufferImageCopy* regions) const {
			vkCmdCopyBufferToImage(m_commandBuffer, src.m_buffer, img.image, dstLayout, regionCount, regions);
		}

		inline void cmdClearColorImage(VkImage image, const VkClearColorValue& clearColor, VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, const uint32_t rangeCount = 1, const VkImageSubresourceRange* imageSubresourceRange = nullptr) const {
			if (imageSubresourceRange != nullptr) {
				vkCmdClearColorImage(m_commandBuffer, image, layout, &clearColor, rangeCount, imageSubresourceRange);
			} else {
				VkImageSubresourceRange newImageSubresourceRange;
                newImageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                newImageSubresourceRange.baseMipLevel = 0;
                newImageSubresourceRange.levelCount = 1;
                newImageSubresourceRange.baseArrayLayer = 0;
                newImageSubresourceRange.layerCount = 1;
				vkCmdClearColorImage(m_commandBuffer, image, layout, &clearColor, rangeCount, &newImageSubresourceRange);
			}			
		}

		inline void cmdClearDepthStencilImage(
			VkImage image,
			const VkClearDepthStencilValue& pDepthStencil,
			VkImageAspectFlags mask = VK_IMAGE_ASPECT_DEPTH_BIT,
			VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			const uint32_t rangeCount = 1,
			const VkImageSubresourceRange* pRanges = nullptr
		) const {
			if (pRanges != nullptr) {
				vkCmdClearDepthStencilImage(m_commandBuffer, image, layout, &pDepthStencil, rangeCount, pRanges);
			} else {
				VkImageSubresourceRange imageSubresourceRange;
				imageSubresourceRange.aspectMask = mask;
				imageSubresourceRange.baseMipLevel = 0;
				imageSubresourceRange.levelCount = 1;
				imageSubresourceRange.baseArrayLayer = 0;
				imageSubresourceRange.layerCount = 1;
				vkCmdClearDepthStencilImage(m_commandBuffer, image, layout, &pDepthStencil, rangeCount, &imageSubresourceRange);
			}
		}

		inline void cmdExecuteCommands(const uint32_t commandBuffersCount, const VkCommandBuffer* cmdBuffers) const {
			vkCmdExecuteCommands(m_commandBuffer, commandBuffersCount, cmdBuffers);
		}

		inline void cmdExecuteCommands(const uint32_t commandBuffersCount, const VulkanCommandBufferEx* cmdBuffers) const {
			std::vector<VkCommandBuffer> buffers(commandBuffersCount);
			for (size_t i = 0; i < commandBuffersCount; ++i) {
				buffers[i] = cmdBuffers[i].m_commandBuffer;
			}

			cmdExecuteCommands(commandBuffersCount, buffers.data());
		}

		inline void cmdPipelineBarrier(
			const VkPipelineStageFlags srcStage,
			const VkPipelineStageFlags dstStage,
			const VkDependencyFlags dependencyFlags,
			const uint32_t memoryBarriersCount,
			const VkMemoryBarrier* mBarriers,
			const uint32_t bufferMemoryBarriersCount,
			const VkBufferMemoryBarrier* bBarriers,
			const uint32_t imageMemoryBarriersCount,
			const VkImageMemoryBarrier* iBarriers
		) const {
			vkCmdPipelineBarrier(m_commandBuffer,
				srcStage,
				dstStage,
				dependencyFlags,
				memoryBarriersCount, mBarriers,
				bufferMemoryBarriersCount, bBarriers,
				imageMemoryBarriersCount, iBarriers);
		}

		inline void cmdBlitImage(
			const VulkanImage& srcImage,
			const VkImageLayout srcImageLayout,
			const VulkanImage& dstImage,
			const VkImageLayout dstImageLayout,
			const uint32_t regionCount,
			const VkFilter filter,
			const VkOffset3D srcOffset,
			const VkOffset3D srcExtent,
			const VkOffset3D dstOffset,
			const VkOffset3D dstExtent,
			const VkImageSubresourceLayers srcSubresourceLayers,
			const VkImageSubresourceLayers dstSubresourceLayers
		) const {
			VkImageBlit blit;
			blit.srcOffsets[0] = srcOffset;
			blit.srcOffsets[1] = srcExtent;
			blit.srcSubresource = srcSubresourceLayers;
			blit.dstOffsets[0] = dstOffset;
			blit.dstOffsets[1] = dstExtent;
			blit.dstSubresource = dstSubresourceLayers;

			vkCmdBlitImage(m_commandBuffer, srcImage.image, srcImageLayout, dstImage.image, dstImageLayout, regionCount, &blit, filter);
		}

		////

		//// advanced commands
		inline void changeImageLayout(
			const VulkanImage &image,
			const VkImageAspectFlags aspectMask,
			const VkImageLayout oldLayout,
			const VkImageLayout newLayout,
			const VkAccessFlags srcAccessMask,
			const VkAccessFlags dstAccessMask,
			const VkPipelineStageFlags srcStage,
			const VkPipelineStageFlags dstStage,
			const uint32_t baseMipLevel = 0,
			const uint32_t mipLevelsCount = 0xffffffff,
			const uint32_t arrayLevels = 0xffffffff
		) const {
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = baseMipLevel;
			subresourceRange.levelCount = mipLevelsCount == 0xffffffff ? image.mipLevels : mipLevelsCount;
			subresourceRange.layerCount = arrayLevels == 0xffffffff ? image.arrayLayers : arrayLevels;

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.pNext = nullptr;
			imageMemoryBarrier.image = image.image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldLayout;
			imageMemoryBarrier.newLayout = newLayout;

			cmdPipelineBarrier(srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		inline void cmdUploadDeviceBuffer(const VulkanDeviceBuffer& buffer, VkDeviceSize gpuBufferOffset = 0) const {
			const VkBufferCopy bufferCopy = { 0, gpuBufferOffset,  buffer.m_stageBuffer.m_size };
			vkCmdCopyBuffer(m_commandBuffer, buffer.m_stageBuffer.m_buffer, buffer.m_gpuBuffer.m_buffer, 1, &bufferCopy);
		}

		void renderVertexes(
			const VulkanPipeline* pipeline,
			const VulkanBuffer& vertexes,
			const uint8_t constantCount,
			const VulkanPushConstant** pushConstants,
			const uint32_t firstSet,
			const uint32_t descriptorSetCount,
			const VkDescriptorSet* descriptorSets,
			const uint8_t dynamicOffsetsCount,
			const uint32_t* dynamicOffsets,
			const uint32_t firstVertex,
			const uint32_t vertexCount,
			const uint32_t instanceCount = 1,
			const uint32_t firstInstance = 0,
			const VkDeviceSize vbOffset = 0
		) {
			cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			cmdBindVertexBuffer(vertexes, vbOffset);

			for (uint8_t i = 0; i < constantCount; ++i) { // set push constants
				if (const VulkanPushConstant* pk = pushConstants[i]) {
					cmdPushConstants(pipeline->program->getPipeLineLayout(), pk->range->stageFlags, pk->range->offset, pk->range->size, pk->values);
				}
			}

			if (descriptorSetCount > 0) { // bind descriptor sets describing shader binding points
				cmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->program->getPipeLineLayout(), firstSet, descriptorSetCount, descriptorSets, dynamicOffsetsCount, dynamicOffsets);
			}

			// draw
			cmdDraw(vertexCount, firstVertex, instanceCount, firstInstance);
		}

		void renderVertexesByIndex(
			const VulkanPipeline* pipeline,
			const VulkanBuffer& vertexes,
			const VulkanBuffer& indexes,
			const uint8_t constantCount,
			const VulkanPushConstant** pushConstants,
			const uint32_t firstSet, 
			const uint32_t descriptorSetCount,
			const VkDescriptorSet* descriptorSets,
			const uint8_t dynamicOffsetsCount,
			const uint32_t* dynamicOffsets,
			const uint32_t firstIndex,
			const uint32_t indexCount,
			const uint32_t firstVertex = 0,
			const uint32_t instanceCount = 1,
			const uint32_t firstInstance = 0,
			const VkDeviceSize vbOffset = 0,
			const VkDeviceSize ibOffset = 0,
			const VkIndexType indexType = VK_INDEX_TYPE_UINT16
		) {
			cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			cmdBindVertexBuffer(vertexes, vbOffset);
			cmdBindIndexBuffer(indexes, ibOffset, indexType);

			for (uint8_t i = 0; i < constantCount; ++i) { // set push constants
				if (const VulkanPushConstant* pk = pushConstants[i]) {
					cmdPushConstants(pipeline->program->getPipeLineLayout(), pk->range->stageFlags, pk->range->offset, pk->range->size, pk->values);
				}
			}

			if (descriptorSetCount > 0) { // bind descriptor sets describing shader binding points
				cmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->program->getPipeLineLayout(), firstSet, descriptorSetCount, descriptorSets, dynamicOffsetsCount, dynamicOffsets);
			}

			// draw indexed
			cmdDrawIndexed(indexCount, firstIndex, instanceCount, firstVertex, firstInstance);
		}

		////////
		inline void prepareRender(
			const VulkanPipeline* pipeline,						// пайплайн
			const uint32_t frame,								// номер кадра отрисовки
			const VulkanPushConstant* pushConstants,			// пуш константы
			const uint32_t firstSet,							// номер первого сет привязки
			const uint8_t dynamicOffsetsCount,					// количество оффсетов для динамических буфферов
			const uint32_t* dynamicOffsets,						// оффсеты для динамических буфферов
			const uint8_t externalSetsCount,					// количество дополнительных сетов, которые хочется привязать
			const VkDescriptorSet* externalSets				// дополнительные сеты, которые хочется привязать
		) {
			if constexpr (stated) { // use this?
				const std::array<typename state_type::NeedBindDescriptors, 8> needBind = state.bindDescriptorSets(
					frame,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline,
					firstSet,
					dynamicOffsetsCount,
					dynamicOffsets,
					externalSetsCount,
					externalSets
				);

				uint8_t i = 0;
				while (needBind[i].setsCount) {
					vkCmdBindDescriptorSets(
						m_commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipeline->program->getPipeLineLayout(),
						needBind[i].firstSet,
						needBind[i].setsCount,
						needBind[i].sets.data(),
						needBind[i].dynamicOffsetsCount,
						needBind[i].dynamicOffsets.data()
					);
					++i;
				}
			} else {
				uint8_t allProgramSetsCount;
				const uint8_t gpuSetsCount = pipeline->program->getGPUSetsCount(&allProgramSetsCount);
				const uint8_t setsCount = std::min(static_cast<uint8_t>(gpuSetsCount + externalSetsCount), allProgramSetsCount) - firstSet;
				VkDescriptorSet* sets = setsCount > 0 ? static_cast<VkDescriptorSet*>(alloca(sizeof(VkDescriptorSet) * setsCount)) : nullptr;

				if (sets) {
					pipeline->fillDescriptorSets(frame, sets, externalSets, setsCount);
					cmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->program->getPipeLineLayout(), firstSet, setsCount, sets, dynamicOffsetsCount, dynamicOffsets);
				} else if (setsCount > 0) {
					assert(false);
					return;
				}
			}

			const uint16_t pushConstantsCount = pipeline->program->getPushConstantsCount();
			switch (pushConstantsCount) {
				case 0: {
				}
					break;
				case 1: {
					if (pushConstants->range) {
						cmdPushConstants(pipeline->program->getPipeLineLayout(), pushConstants->range->stageFlags, pushConstants->range->offset, pushConstants->range->size, pushConstants->values);
					}
				}
					break;
				default: {
					for (uint8_t i = 0; i < pushConstantsCount; ++i) { // set push constants
						const VulkanPushConstant& pk = pushConstants[i];
						cmdPushConstants(pipeline->program->getPipeLineLayout(), pk.range->stageFlags, pk.range->offset, pk.range->size, pk.values);
					}

				}
					break;
			}
		}

		inline void render(
			const VulkanPipeline* pipeline,						// пайплайн
			const uint32_t frame,								// номер кадра отрисовки
			const VulkanPushConstant* pushConstants,			// пуш константы
			const uint32_t firstSet,							// номер первого сет привязки
			const uint8_t dynamicOffsetsCount,					// количество оффсетов для динамических буфферов
			const uint32_t* dynamicOffsets,						// оффсеты для динамических буфферов
			const uint8_t externalSetsCount,					// количество дополнительных сетов, которые хочется привязать
			const VkDescriptorSet* externalSets,				// дополнительные сеты, которые хочется привязать
			const VulkanBuffer& vertexes,						// вершины
			const uint32_t firstVertex,							// номер первой вершины
			const uint32_t vertexCount,							// количество вершин
			const uint32_t instanceCount = 1,					// количество инстансов
			const uint32_t firstInstance = 0,					// номер первого инстанса
			const VkDeviceSize vbOffset = 0						// оффсет в вершинном буфере
		) {
			prepareRender(pipeline, frame, pushConstants, firstSet, dynamicOffsetsCount, dynamicOffsets, externalSetsCount, externalSets);

			cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			cmdBindVertexBuffer(vertexes, vbOffset);

			// draw
			cmdDraw(vertexCount, firstVertex, instanceCount, firstInstance);
		}

		inline void renderIndexed(
			const VulkanPipeline* pipeline,						// пайплайн
			const uint32_t frame,								// номер кадра отрисовки
			const VulkanPushConstant* pushConstants,			// пуш константы
			const uint32_t firstSet,							// номер первого сет привязки
			const uint8_t dynamicOffsetsCount,					// количество оффсетов для динамических буфферов
			const uint32_t* dynamicOffsets,						// оффсеты для динамических буфферов
			const uint8_t externalSetsCount,					// количество дополнительных сетов, которые хочется привязать
			const VkDescriptorSet* externalSets,				// дополнительные сеты, которые хочется привязать
			const VulkanBuffer& vertexes,						// вершины
			const VulkanBuffer& indexes,						// индексы
			const uint32_t firstIndex,							// номер первого индекса
			const uint32_t indexCount,							// количество индексов
			const uint32_t firstVertex = 0,						// номер первой вершины
			const uint32_t instanceCount = 1,					// количество инстансов
			const uint32_t firstInstance = 0,					// номер первого инстанса
			const VkDeviceSize vbOffset = 0,					// оффсет в вершинном буфере
			const VkDeviceSize ibOffset = 0,					// оффсет в индексном буффере
			const VkIndexType indexType = VK_INDEX_TYPE_UINT32	// тип индексов
			) {
				prepareRender(pipeline, frame, pushConstants, firstSet, dynamicOffsetsCount, dynamicOffsets, externalSetsCount, externalSets);

				cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
				cmdBindVertexBuffer(vertexes, vbOffset);
				cmdBindIndexBuffer(indexes, ibOffset, indexType);

				// draw indexed
				cmdDrawIndexed(indexCount, firstIndex, instanceCount, firstVertex, firstInstance);
		};

        // getters
        std::optional<VkRect2D> getCurrentScissor() const {
            if constexpr (stated) {
                return state.getCurrentScissor();
            } else {
                return std::nullopt;
            }
        }
		//// advanced commands

		VkCommandPool m_pool = VK_NULL_HANDLE;
		VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		inline static std::atomic_bool _lockAllocation = {};
	};

	template <typename STATE>
	struct VulkanCommandBuffersArrayEx {
		std::vector<std::vector<VkSemaphore>> m_waitSemaphores;
		std::vector<std::vector<VkSemaphore>> m_signalSemaphores;
		std::vector<VulkanSemaphore> m_completeSemaphores;
		std::vector<VulkanCommandBufferEx<STATE>> m_commands;
		VkPipelineStageFlags m_waitStageMask = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

		VulkanCommandBuffersArrayEx() = default;

		VulkanCommandBuffersArrayEx(
			VkDevice device,
			VkCommandPool pool,
			VkPipelineStageFlags waitStageMask,
			uint32_t cmdBuffersCount,
			const VkSemaphoreCreateFlags signalSemFlags = 0,
			const VkAllocationCallbacks* signalSempAllocator = nullptr,
			const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
		) : m_waitStageMask(waitStageMask)
		{
			m_commands.reserve(cmdBuffersCount);
			m_completeSemaphores.reserve(cmdBuffersCount);
			m_waitSemaphores.resize(cmdBuffersCount);
			m_signalSemaphores.resize(cmdBuffersCount);
			for (uint32_t i = 0; i < cmdBuffersCount; ++i) {
				m_commands.emplace_back(device, pool, level);
				m_completeSemaphores.emplace_back(device, signalSemFlags, signalSempAllocator);
				addSignalSemaphore(m_completeSemaphores[i].semaphore, i);
			}
		}

		~VulkanCommandBuffersArrayEx() {
			destroy();
		}

		VulkanCommandBuffersArrayEx(VulkanCommandBuffersArrayEx&& cmdBuff) noexcept {
			m_waitSemaphores = std::move(cmdBuff.m_waitSemaphores);
			m_signalSemaphores = std::move(cmdBuff.m_signalSemaphores);
			m_completeSemaphores = std::move(cmdBuff.m_completeSemaphores);
			m_commands = std::move(cmdBuff.m_commands);
			m_waitStageMask = cmdBuff.m_waitStageMask;
		}

		VulkanCommandBuffersArrayEx& operator= (VulkanCommandBuffersArrayEx&& cmdBuff) noexcept {
			m_waitSemaphores = std::move(cmdBuff.m_waitSemaphores);
			m_signalSemaphores = std::move(cmdBuff.m_signalSemaphores);
			m_completeSemaphores = std::move(cmdBuff.m_completeSemaphores);
			m_commands = std::move(cmdBuff.m_commands);
			m_waitStageMask = cmdBuff.m_waitStageMask;
			return *this;
		}

		VulkanCommandBuffersArrayEx(const VulkanCommandBuffersArrayEx& cmdBuff) = delete;
		VulkanCommandBuffersArrayEx& operator= (const VulkanCommandBuffersArrayEx& cmdBuff) = delete;

		inline VulkanCommandBufferEx<STATE>& operator[] (uint32_t i) { return m_commands[i]; }
		inline const VulkanCommandBufferEx<STATE>& operator[] (uint32_t i) const { return m_commands[i]; }

		inline void addWaitSemaphore(VkSemaphore s, const uint32_t i) {
			if (std::find(m_waitSemaphores[i].begin(), m_waitSemaphores[i].end(), s) == m_waitSemaphores[i].end()) {
				m_waitSemaphores[i].push_back(s);
			}
		}

		inline void removeWaitSemaphore(VkSemaphore s, const uint32_t i) {
			m_waitSemaphores[i].erase(std::remove(m_waitSemaphores[i].begin(), m_waitSemaphores[i].end(), s), m_waitSemaphores[i].end());
		}

		inline void addSignalSemaphore(VkSemaphore s, const uint32_t i) {
			if (std::find(m_signalSemaphores[i].begin(), m_signalSemaphores[i].end(), s) == m_signalSemaphores[i].end()) {
				m_signalSemaphores[i].push_back(s);
			}
		}

		inline void removeSignalSemaphore(VkSemaphore s, const uint32_t i) {
			m_signalSemaphores[i].erase(std::remove(m_signalSemaphores[i].begin(), m_signalSemaphores[i].end(), s), m_signalSemaphores[i].end());
		}

		template <typename S>
		void depend(const VulkanCommandBuffersArrayEx<S>& buffer) {
			uint32_t i = 0;
			for (auto&& semaphore : buffer.m_completeSemaphores) {
				addWaitSemaphore(semaphore.semaphore, i);
				++i;
			}
		}

		template <typename S>
		void independ(const VulkanCommandBuffersArrayEx<S>& buffer) {
			uint32_t i = 0;
			for (auto&& semaphore : buffer.m_completeSemaphores) {
				removeWaitSemaphore(semaphore.semaphore, i);
				++i;
			}
		}

		inline void clearCommands() { 
			m_commands.clear();
		}

		inline void destroy() {
			m_waitSemaphores.clear();
			m_signalSemaphores.clear();
			clearCommands();
			for (auto&& semaphore : m_completeSemaphores) {
				semaphore.destroy();
			}
			m_completeSemaphores.clear();
		}

		[[nodiscard]] VkSubmitInfo prepareToSubmit(const uint32_t commandBufferNum) const {
			const uint32_t frameNum = commandBufferNum % m_commands.size();
			return m_commands[frameNum].prepareToSubmit(&m_waitStageMask, static_cast<uint32_t>(m_waitSemaphores[frameNum].size()), m_waitSemaphores[frameNum].data(), static_cast<uint32_t>(m_signalSemaphores[frameNum].size()), m_signalSemaphores[frameNum].data());
		}
	};

	using VulkanCommandBufferRaw = VulkanCommandBufferEx<EmptyState>;
	using VulkanCommandBuffersArrayRaw = VulkanCommandBuffersArrayEx<EmptyState>;

	using VulkanCommandBuffer = VulkanCommandBufferEx<VulkanCommandBufferState>;
	using VulkanCommandBuffersArray = VulkanCommandBuffersArrayEx<VulkanCommandBufferState>;
}