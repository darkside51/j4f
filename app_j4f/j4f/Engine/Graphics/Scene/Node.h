#pragma once

#include "../../Core/Hierarchy.h"
#include "../../Core/Math/math.h"
#include "NodeGraphicLink.h"

namespace engine {
	
	class Node {
	public:
		virtual ~Node() = default;

		inline void calculateModelMatrix(const glm::mat4& parentModel) {
			_model = parentModel * _local;
			_dirtyModel = false;
			_modelChanged = true;
		}

		inline static bool updateModelMatrix(HierarchyRaw<Node>* node) {
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
			}

			return true;
		}

		inline void setLocalMatrix(const glm::mat4& m) {
			memcpy(&_local, &m, sizeof(glm::mat4));
			_dirtyModel = true;
		}

		inline const glm::mat4& model() const { return _model; }

	private:
		bool _dirtyModel = false;
		bool _modelChanged = false;
		glm::mat4 _local = glm::mat4(1.0f);
		glm::mat4 _model = glm::mat4(1.0f);
		NodeGraphicLink* _graphics = nullptr;
	};

	using H_Node = HierarchyRaw<Node>;
	using Hs_Node = HierarchyShared<Node>;
}