#pragma once

#include <Engine/Core/Hierarchy.h>
#include <Engine/Core/Ref_ptr.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>
#include <Engine/Graphics/UI/ImGui/Imgui.h>
#include <Engine/Utils/ImguiStatObserver.h>
#include <memory>

namespace engine {
	class Node;
}

namespace game {
	using NodeHR = engine::HierarchyRaw<engine::Node>;

	class Scene{
	public:
		Scene();
		~Scene();

		void update(const float delta);
		void render(const float delta);

	private:
		std::unique_ptr<engine::ImguiStatObserver> _statObserver = nullptr;
		engine::ref_ptr<engine::ImguiGraphics> _imguiGraphics = nullptr;

		std::unique_ptr<NodeHR> _rootNode;
		std::unique_ptr<NodeHR> _uiNode;
	};
}