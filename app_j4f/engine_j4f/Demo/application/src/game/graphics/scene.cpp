#include "scene.h"

#include <Engine/Core/Common.h>
#include <Engine/Graphics/Render/RenderList.h>
#include <Engine/Graphics/Scene/Node.h>
#include <Engine/Graphics/Scene/NodeRenderListHelper.h>
#include <Engine/Graphics/UI/ImGui/Imgui.h>

#include <cstdint>

namespace game {
	constexpr uint16_t kUiOrder = 255u;
	engine::RenderList rootRenderList;
	engine::RenderList uiRenderList;

	Scene::Scene() :
		_statObserver(std::make_unique<engine::ImguiStatObserver>(engine::ImguiStatObserver::Location::top_left)),
		_rootNode(std::make_unique<NodeHR>()),
		_uiNode(std::make_unique<NodeHR>()) {

		auto imgui = std::make_unique<engine::NodeRendererImpl<engine::ImguiGraphics*>>();
		imgui->setGraphics(engine::ImguiGraphics::getInstance());
		imgui->getRenderDescriptor()->order = kUiOrder;
		_imguiGraphics = imgui->graphics();
		 {
			auto* node = new NodeHR(imgui.release());
			_uiNode->addChild(node);
		}
	}

	Scene::~Scene() {
		_rootNode = nullptr;
		_uiNode = nullptr;
	}

	void Scene::update(const float delta) {
	}

	void Scene::render(const float delta) {
		using namespace engine;

		auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
		const uint32_t currentFrame = renderer->getCurrentFrame();

		const auto [width, height] = renderer->getSize();

		auto& commandBuffer = renderer->getRenderCommandBuffer();
		commandBuffer.begin();

		std::array<VkClearValue, 2> clearValues = {
			   VkClearValue{ 0.5f, 0.5f, 0.5f, 1.0f }, // for 1st attachment (color)
			   VkClearValue{ 1.0f, 0 } // for 2nd attachment (depthStencil)
		};
		commandBuffer.cmdBeginRenderPass(renderer->getMainRenderPass(), { {0, 0}, {width, height} },
										clearValues.data(), clearValues.size(), 
										renderer->getFrameBuffer().m_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

		commandBuffer.cmdSetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f, false);
		commandBuffer.cmdSetScissor(0, 0, width, height);
		commandBuffer.cmdSetDepthBias(0.0f, 0.0f, 0.0f);

		engine::reloadRenderList(uiRenderList, _uiNode.get(), false, 0);

		if (_imguiGraphics) {
			_imguiGraphics->update(delta);
			_statObserver->draw();
		}

		uiRenderList.render(commandBuffer, currentFrame, { nullptr, nullptr, nullptr });

		commandBuffer.cmdEndRenderPass();
		commandBuffer.end();
	}
}