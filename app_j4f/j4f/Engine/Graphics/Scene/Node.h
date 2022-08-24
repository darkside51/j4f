#pragma once

#include "../../Core/Hierarchy.h"
#include "../../Core/Math/math.h"
#include "../Render/RenderList.h"

#include "Camera.h"
#include "NodeGraphicsLink.h"
#include "BoundingVolume.h"

namespace engine {
	
	class Node {
		friend struct NodeMatrixUpdater;
	public:
		virtual ~Node() {
			if (_graphics) {
				delete _graphics;
				_graphics = nullptr;
			}

			if (_boundingVolume) {
				delete _boundingVolume;
				_boundingVolume = nullptr;
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

		inline const BoundingVolume* getBoundingVolume() const { return _boundingVolume; }
		inline void setBoundingVolume(const BoundingVolume* v) {
			if (_boundingVolume) {
				delete _boundingVolume;
			}
			_boundingVolume = v;
		}

	private:
		bool _dirtyModel = false;
		bool _modelChanged = false;
		glm::mat4 _local = glm::mat4(1.0f);
		glm::mat4 _model = glm::mat4(1.0f);
		const RenderObject* _graphics = nullptr;
		const BoundingVolume* _boundingVolume = nullptr;
	};

	using H_Node = HierarchyRaw<Node>;
	using Hs_Node = HierarchyShared<Node>;

	struct NodeMatrixUpdater {
		inline static bool _(H_Node* node, Camera* camera = nullptr, const bool dirtyVisible = false) {
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

			if (const Frustum* frustum = (camera ? camera->getFrustum() : nullptr); frustum && (dirtyVisible || mNode._modelChanged)) {
				if (const BoundingVolume* volume = mNode._boundingVolume) {
					return volume->checkFrustum(frustum);
				} else {
					return true;
				}
			} else {
				return true;
			}

			return true;
		}
	};

	struct RenderListEmplacer {
		inline static bool _(H_Node* node, RenderList& list, Camera* camera = nullptr, const bool dirtyVisible = false) {
			if (NodeMatrixUpdater::_(node, camera, dirtyVisible)) {
				if (const RenderObject* renderObject = node->value().getRenderObject()) {
					list.addDescriptor(renderObject->getRenderDescriptor());
				}
				return true;
			}

			return false;
		}
	};

	inline void reloadRenderList(RenderList& list, H_Node* node, Camera* camera = nullptr, const bool dirtyVisible = false) {
		list.clear();
		node->execute_with<RenderListEmplacer>(list, camera, dirtyVisible);
		list.sort();
	}
}