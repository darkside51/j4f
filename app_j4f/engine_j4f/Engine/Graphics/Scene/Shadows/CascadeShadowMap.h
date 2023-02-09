#pragma once

#include "../../../Core/Common.h"
#include "../../Core/Math/mathematic.h"
#include "../../Vulkan/vkCommandBuffer.h"
#include "../../Vulkan/vkFrameBuffer.h"
#include "../../Vulkan/vkDebugMarker.h"
#include "../Camera.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace vulkan {
	class VulkanTexture;
	class VulkanGpuProgram;
}

namespace engine {

	class Camera;
	class Graphics;

	enum class ShadowMapSpecialPipelines : uint8_t {
		SH_PIPELINE_DEBUG = 0,
		SH_PIPEINE_PLAIN = 1,
		SH_PIPELINES_COUNT = 2
	};

	enum class ShadowMapTechnique : uint8_t {
		SMT_DEFAULT = 0,
		SMT_GEOMETRY_SH = 1
	};

	class MeshSkinnedShadow {};
	class MeshStaticShadow {};
	class MeshStaticInstanceShadow {};

	class CascadeShadowMap {
		friend class Graphics;

		struct CascadeFrameBuffer {
			~CascadeFrameBuffer(); // call destroy from render->device

			inline void destroy(VkDevice device) {
				vkDestroyImageView(device, view, nullptr);
				delete frameBuffer;
			}

			vulkan::VulkanFrameBuffer* frameBuffer; // ����� �� ��������� ��� ������ swapchain image?
			VkImageView view;
		};

	public:
		template <typename T>
		inline static vulkan::VulkanGpuProgram* getShadowProgram() {
			const uint16_t id = UniqueTypeId<CascadeShadowMap>::getUniqueId<T>();

			if (auto it = _shadowPrograms.find(id);  it != _shadowPrograms.end()) {
				return it->second;
			}

			return nullptr;
		}

		template <typename T>
		inline static bool registerShadowProgram(vulkan::VulkanGpuProgram* program) {
			const uint16_t id = UniqueTypeId<CascadeShadowMap>::getUniqueId<T>();

			if (auto it = _shadowPrograms.find(id);  it != _shadowPrograms.end()) {
				return false;
			}

			_shadowPrograms[id] = program;

			return true;
		}

		inline static const vulkan::VulkanPipeline* getSpecialPipeline(const ShadowMapSpecialPipelines p) {
			return _specialPipelines[static_cast<uint8_t>(p)];
		}

		CascadeShadowMap(const uint16_t dim, const uint8_t count, const glm::vec2& nearFar, const float minZ, const float maxZ);

		~CascadeShadowMap();

		void initCascadeSplits(const glm::vec2& nearFar, const float minZ, const float maxZ);
		inline void setLamdas(const float s, const float r, const float l) {
			_cascadeSplitLambda = s;
			_frustumRadiusLambda = r;
			_cascadeLigthLambda = l;
		}

		void updateCascades(const Camera* camera);
		void renderShadows(vulkan::VulkanCommandBuffer& commandBuffer);

		inline const VkRenderPass getRenderPass() const { return _depthRenderPass; }
		inline VkRenderPass getRenderPass() { return _depthRenderPass; }
		inline const vulkan::VulkanTexture* getTexture() const { return _shadowMapTexture; }
		inline vulkan::VulkanTexture* getTexture() { return _shadowMapTexture; }

		inline void prepareToRender(vulkan::VulkanCommandBuffer& commandBuffer) const {
			commandBuffer.cmdSetViewport(0.0f, 0.0f, static_cast<float>(_dimension), static_cast<float>(_dimension), 0.0f, 1.0f, false);
			commandBuffer.cmdSetScissor(0, 0, _dimension, _dimension);
			commandBuffer.cmdSetDepthBias(0.0f, 0.0f, 0.0f);
		}

		inline void beginRenderPass(vulkan::VulkanCommandBuffer& commandBuffer, const uint8_t cascadeId) const {
			GPU_DEBUG_MARKER_BEGIN_REGION(commandBuffer.m_commandBuffer, engine::fmt_string("j4f shadow render pass %d", cascadeId), 0.0f, 0.0f, 1.0f, 1.0f);
			commandBuffer.cmdBeginRenderPass(_depthRenderPass, { {0, 0}, {_dimension, _dimension} }, &_shadowClearValues, 1, _cascades[cascadeId].frameBuffer->m_framebuffer, VK_SUBPASS_CONTENTS_INLINE);
		}

		inline void endRenderPass(vulkan::VulkanCommandBuffer& commandBuffer) const {
			commandBuffer.cmdEndRenderPass();
			GPU_DEBUG_MARKER_END_REGION(commandBuffer.m_commandBuffer);
		}

		template <typename T>
		inline void setLightPosition(T&& lightPosition) { _lightDirection = as_normalized(-lightPosition); }

		inline uint8_t getCascadesCount() const { return _cascadesCount; }
		inline uint16_t getDimension() const { return _dimension; }
		inline const std::vector<float>& getSplitDepths() const { return _splitDepths; }
		inline std::vector<float>& getSplitDepths() { return _splitDepths; }
		inline const std::vector<glm::mat4>& getVPMatrixes() const { return _cascadeViewProjects; }
		inline std::vector<glm::mat4>& getVPMatrixes() { return _cascadeViewProjects; }

		inline const glm::mat4& getVPMatrix(const uint8_t i) const { return _cascadeViewProjects[i]; }

		template <typename T>
		inline T* getSplitDepthsPointer() const { return reinterpret_cast<T*>(const_cast<float*>(_splitDepths.data())); }

		void updateShadowUniforms(vulkan::VulkanGpuProgram* program, const glm::mat4& cameraMatrix);

		inline void updateShadowUniformsForRegesteredPrograms(const glm::mat4& cameraMatrix) {
			if (_dirtyCascades > 0) {
				--_dirtyCascades;
				for (auto it = _programsShadowUniforms.begin(); it != _programsShadowUniforms.end(); ++it) {
					const ShadowUniforms& uniforms = it->second;
					it->first->setValueToLayout(uniforms.viewLayout, &const_cast<glm::mat4&>(cameraMatrix), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
					it->first->setValueToLayout(uniforms.cascadeMatrixLayout, _cascadeViewProjects.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
					it->first->setValueToLayout(uniforms.cascadeSplitLayout, getSplitDepthsPointer<glm::vec4>(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
				}
			}
		}

		void registerProgramAsReciever(vulkan::VulkanGpuProgram* program);
		void unregisterProgramAsReciever(vulkan::VulkanGpuProgram* program);

		inline ShadowMapTechnique techique() const { return _technique; }

	private:
		static void initPipelines();
		static void registerCommonShadowPrograms();

		inline void static initCommonData() {
			initPipelines();
			registerCommonShadowPrograms();
		}

		inline static vulkan::VulkanPipeline* _specialPipelines[static_cast<uint8_t>(ShadowMapSpecialPipelines::SH_PIPELINES_COUNT)];
		inline static std::unordered_map<uint16_t, vulkan::VulkanGpuProgram*> _shadowPrograms;

		void initVariables();

		ShadowMapTechnique _technique;
		uint8_t _cascadesCount;
		uint16_t _dimension;
		float _cascadeSplitLambda = 1.0f;
		float _frustumRadiusLambda = 1.0f;
		float _cascadeLigthLambda = 1.0f;

		std::vector<CascadeFrameBuffer> _cascades;		// _cascadesCount
		std::vector<float> _cascadeSplits;				// _cascadesCount
		std::vector<float> _splitDepths;				// _cascadesCount
		std::vector<glm::mat4> _cascadeViewProjects;	// _cascadesCount
		std::vector<Frustum> _cascadeFrustums;			// _cascadesCount

		VkClearValue _shadowClearValues;

		VkRenderPass _depthRenderPass;
		vulkan::VulkanTexture* _shadowMapTexture;

		glm::vec3 _lightDirection;

		struct ShadowUniforms {
			const vulkan::GPUParamLayoutInfo* viewLayout;
			const vulkan::GPUParamLayoutInfo* cascadeMatrixLayout;
			const vulkan::GPUParamLayoutInfo* cascadeSplitLayout;
		};

		uint8_t _dirtyCascades = 0;
		std::unordered_map<vulkan::VulkanGpuProgram*, ShadowUniforms> _programsShadowUniforms;
	};

}