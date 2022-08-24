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
			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
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

		inline const glm::mat4& localMatrix() const { return _local; }
		inline const glm::mat4& model() const { return _model; }

		inline const RenderObject* getRenderObject() const { return _graphics; }
		inline void setRenderObject(const RenderObject* r) {
			if (_graphics) {
				delete _graphics;
			}
			_graphics = r;
		}

	private:
		bool _dirtyModel = false;
		bool _modelChanged = false;
		glm::mat4 _local = glm::mat4(1.0f);
		glm::mat4 _model = glm::mat4(1.0f);
		const RenderObject* _graphics = nullptr;
	};

	using H_Node = HierarchyRaw<Node>;
	using Hs_Node = HierarchyShared<Node>;

	class Camera;

	struct NodeMatrixUpdater {
		inline static bool _(H_Node* node, Camera* camera = nullptr, const bool checkVisible = false) {
			Node& mNode = node->value();
			mNode._modelChanged = false;

			const auto&& parent = node->getParent();
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
			}

			if (checkVisible || mNode._modelChanged) {
				// todo: check visible with camera
			}

			return true;
		}
	};

	struct RenderListEmplacer {
		inline static bool _(H_Node* node, RenderList& list, Camera* camera = nullptr, const bool checkVisible = false) {
			if (NodeMatrixUpdater::_(node, camera, checkVisible)) {
				if (const RenderObject* renderObject = node->value().getRenderObject()) {
					list.addDescriptor(renderObject->getRenderDescriptor());
				}
				return true;
			}

			return false;
		}
	};

	inline void reloadRenderList(RenderList& list, H_Node* node, Camera* camera = nullptr, const bool checkVisible = false) {
		list.clear();
		node->execute_with<RenderListEmplacer>(list, camera, checkVisible);
		list.sort();
	}
}