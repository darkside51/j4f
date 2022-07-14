#pragma once

#include "../../Core/Hierarchy.h"
#include "../../Core/Math/math.h"
#include "NodeGraphicsLink.h"
#include "../Render/RenderList.h"

namespace engine {
	
	class Node {
		friend struct NodeMatrixUpdater;
	public:
		virtual ~Node() {
			if (_graphicsLink) {
				delete _graphicsLink;
				_graphicsLink = nullptr;
			}
		}

		inline void calculateModelMatrix(const glm::mat4& parentModel) {
			_model = parentModel * _local;
			_dirtyModel = false;
			_modelChanged = true;
		}

		inline void setLocalMatrix(const glm::mat4& m) {
			memcpy(&_local, &m, sizeof(glm::mat4));
			_dirtyModel = true;
		}

		inline const glm::mat4& model() const { return _model; }
		inline const GraphicsLink* getGraphicsLink() const { return _graphicsLink; }

		inline void setGraphicsLink(GraphicsLink* g) {
			if (_graphicsLink) {
				delete _graphicsLink;
			}
			_graphicsLink = g;
		}

		template <typename T>
		inline void makeGraphicsLink(NodeRenderer<T>* r) {
			if (_graphicsLink) {
				delete _graphicsLink;
			}

			_graphicsLink = new GraphicsLink(this);
			r->setNodeLink(_graphicsLink);
		}

	private:
		bool _dirtyModel = false;
		bool _modelChanged = false;
		glm::mat4 _local = glm::mat4(1.0f);
		glm::mat4 _model = glm::mat4(1.0f);
		GraphicsLink* _graphicsLink = nullptr;
	};

	using H_Node = HierarchyRaw<Node>;
	using Hs_Node = HierarchyShared<Node>;

	class Camera;

	struct NodeMatrixUpdater {
		inline static bool _(H_Node* node, Camera* camera = nullptr) {
			// todo: check visible with camera

			Node& mNode = node->value();
			mNode._modelChanged = false;

			auto&& parent = node->getParent();
			if (parent) {
				mNode._dirtyModel |= parent->value()._modelChanged;
			}

			if (mNode._dirtyModel) {
				if (parent) {
					mNode.calculateModelMatrix(parent->value()._model);
				} else {
					memcpy(&mNode._model, &mNode._local, sizeof(glm::mat4));
					mNode._dirtyModel = false;
					mNode._modelChanged = true;
				}

				if (mNode._graphicsLink) {
					mNode._graphicsLink->updateNodeTransform();
				}
			}

			return true;
		}
	};

	struct RenderListEmplacer {
		inline static bool _(H_Node* node, RenderList& list, Camera* camera = nullptr) {
			if (auto&& link = node->value().getGraphicsLink()) {
				if (NodeMatrixUpdater::_(node, camera)) {
					list.addDescriptor(link->getGraphics()->getRenderDescriptor());
				}
			}
			return true;
		}
	};

	inline void reloadRenderList(RenderList& list, H_Node* node, Camera* camera = nullptr) {
		list.clear();
		node->execute_with<RenderListEmplacer>(list, camera);
		list.sort();
	}
}