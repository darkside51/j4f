// ♣♠♦♥
#include "Mesh.h"
#include "MeshData.h"
#include "Loader_gltf.h"
#include "../Render/RenderHelper.h"
#include "AnimationTree.h"
#include "../VertexAttributes.h"
#include "../../Utils/Debug/Assert.h"
#include <limits>

namespace engine {

	void updateSkeletonAnimation(const CancellationToken& token, MeshSkeleton* skeleton, const float time, const Mesh_Animation* animation, const uint8_t updateFrame) {
		for (const auto& channel : animation->channels) {
			if (channel.sampler == 0xffff || channel.target_node == 0xffff) continue;

			Mesh_Node& target = skeleton->getNode(updateFrame, channel.target_node);
			const Mesh_Animation::AnimationSampler& sampler = animation->samplers[channel.sampler];

			for (size_t i = 0, sz = sampler.inputs.size() - 1; i < sz; ++i) {

				const float t0 = sampler.inputs[i];
				const float t1 = sampler.inputs[i + 1];

				if ((time >= t0) && (time < t1)) {

					const vec4f& v0 = sampler.outputs[i];

					switch (sampler.interpolation) {
					case Mesh_Animation::Interpolation::LINEAR:
					{
						const vec4f& v1 = sampler.outputs[i + 1];
						const float mix_c = (time - t0) / (t1 - t0);
						switch (channel.path) {
						case Mesh_Animation::AimationChannelPath::TRANSLATION:
						{
							if (!compare(v0, v1, 1e-4f)) {
								target.setTranslation(v0);
							} else {
								target.setTranslation(glm::mix(v0, v1, mix_c));
							}
						}
						break;
						case Mesh_Animation::AimationChannelPath::ROTATION:
						{
							if (!compare(v0, v1, 1e-4f)) {
								target.setRotation(quatf(v0.w, v0.x, v0.y, v0.z));
							} else {
								const quatf q1(v0.w, v0.x, v0.y, v0.z);
								const quatf q2(v1.w, v1.x, v1.y, v1.z);
								target.setRotation(glm::normalize(glm::slerp(q1, q2, mix_c)));
							}
						}
						break;
						case Mesh_Animation::AimationChannelPath::SCALE:
						{
							if (!compare(v0, v1, 1e-4f)) {
								target.setScale(v0);
							} else {
								target.setScale(glm::mix(v0, v1, mix_c));
							}
						}
						break;
						default:
							break;
						}
					}
					break;
					case Mesh_Animation::Interpolation::STEP:
					{
						switch (channel.path) {
						case Mesh_Animation::AimationChannelPath::TRANSLATION:
							target.setTranslation(v0);
							break;
						case Mesh_Animation::AimationChannelPath::ROTATION:
							target.setRotation(quatf(v0.w, v0.x, v0.y, v0.z));
							break;
						case Mesh_Animation::AimationChannelPath::SCALE:
							target.setScale(v0);
							break;
						default:
							break;
						}
					}
					break;
					case Mesh_Animation::Interpolation::CUBICSPLINE: // todo!
						break;
					default:
						break;
					}

					break;
				}
			}
		}

		if (token) {
			return;
		}

		skeleton->updateTransforms(updateFrame);

		if (!skeleton->_skins.empty() && !token) {
			skeleton->updateSkins(updateFrame);
		}
	}

	void updateSkeletonAnimationTree(const CancellationToken& token, MeshSkeleton* skeleton, MeshAnimationTree* animTree, const uint8_t updateFrame) {
		animTree->calculate(updateFrame); // расчет scale, rotation, translation для нодов анимации
		if (token) return;

		animTree->apply(skeleton, updateFrame);

		if (token) return;

		skeleton->updateTransforms(updateFrame);

		if (!skeleton->_skins.empty() && !token) {
			skeleton->updateSkins(updateFrame);
		}

        skeleton->setUpdatedFrameNum(updateFrame);
	}

    void applyAnimationFrameToSkeleton(const CancellationToken& token, MeshSkeleton* skeleton, MeshAnimationTree* animTree, const uint8_t updateFrame) {
        animTree->apply(skeleton, updateFrame);

        if (token) return;

        skeleton->updateTransforms(updateFrame);

        if (!skeleton->_skins.empty() && !token) {
            skeleton->updateSkins(updateFrame);
        }
    }

	MeshSkeleton::MeshSkeleton(Mesh_Data* mData, const uint8_t latency) :
		_skins(mData->skins), 
		_hierarchyes(latency),
		_nodes(latency),
		_skinsMatrices(latency),
		_animCalculationResult(latency),
		_latency(latency)
	{
		for (uint8_t i = 0; i < _latency; ++i) {
			_hierarchyes[i].reserve(mData->sceneNodes.size());
			_nodes[i].resize(mData->nodes.size());

			for (const uint16_t nodeId : mData->sceneNodes) {
				loadNode(mData, nodeId, nullptr, i);
			}
		}

		const size_t skinsSize = mData->skins.size();
		for (uint8_t i = 0; i < _latency; ++i) {
			_skinsMatrices[i].resize(skinsSize);
		}

		for (size_t i = 0; i < skinsSize; ++i) {
			const size_t matricesCount = mData->skins[i].inverseBindMatrices.size();
			for (uint8_t ii = 0; ii < latency; ++ii) {
				_skinsMatrices[ii][i].resize(matricesCount);
				memcpy(&(_skinsMatrices[ii][i][0]), &(mData->skins[i].inverseBindMatrices[0]), matricesCount * sizeof(mat4f));
			}
		}

		// setup bind pose
		for (uint8_t i = 0; i < _latency; ++i) {
			updateTransforms(i);
			updateSkins(i);
		}
	}

	MeshSkeleton::~MeshSkeleton() {
		for (size_t i = 0; i < _latency; ++i) {
			if (_animCalculationResult[i]) { _animCalculationResult[i]->cancel(); }

			std::vector<HierarchyRaw<Mesh_Node>*>& hi = _hierarchyes[i];
			for (HierarchyRaw<Mesh_Node>* h : hi) {
				delete h;
			}
		}
	}

	void MeshSkeleton::loadNode(const Mesh_Data* mData, const uint16_t nodeId, HierarchyRaw<Mesh_Node>* parent, const uint8_t h) {
		const gltf::Node& node = mData->nodes[nodeId];

		// parse node values
		Mesh_Node mNode;
		mNode.scale = vec3f(node.scale.x, node.scale.y, node.scale.z);
		mNode.rotation = quatf(node.rotation.w, node.rotation.x, node.rotation.y, node.rotation.z);
		mNode.translation = vec3f(node.translation.x, node.translation.y, node.translation.z);

		mNode.skinIndex = node.skin;

		auto* nodesHierarchy = new HierarchyRaw<Mesh_Node>(std::move(mNode));
		if (parent) {
			parent->addChild(nodesHierarchy);
		} else {
			_hierarchyes[h].push_back(nodesHierarchy);
		}

		_nodes[h][nodeId] = nodesHierarchy;

		for (const uint16_t cNodeId : node.children) {
			loadNode(mData, cNodeId, nodesHierarchy, h);
		}
	}

	void MeshSkeleton::updateAnimation(const float time, const Mesh_Animation* animation, float& currentAnimTime) {
		_updateFrameNum = (_updateFrameNum + 1) % _latency;

		if (time == 0.0f) return;

		currentAnimTime += time;

		if (currentAnimTime > animation->duration) {
			currentAnimTime -= animation->duration;
		}

		const float atime = animation->start + currentAnimTime;

		//_animCalculationResult[_updateFrameNum] = Engine::getInstance().getModule<ThreadPool>()->enqueue(TaskType::COMMON, 0, updateSkeletonAnimation, this, atime, animation, _updateFrameNum);
		_animCalculationResult[_updateFrameNum] = Engine::getInstance().getModule<ThreadPool2>().enqueue(TaskType::COMMON, updateSkeletonAnimation, this, atime, animation, _updateFrameNum);
	}

	void MeshSkeleton::updateAnimation(const float time, MeshAnimationTree* animTree) {
		_updateFrameNum = (_updateFrameNum + 1) % _latency;

		if (time == 0.0f) return;

		animTree->update(time, _updateFrameNum); // просто пересчет времени

		//_animCalculationResult[_updateFrameNum] = Engine::getInstance().getModule<ThreadPool>()->enqueue(TaskType::COMMON, 0, updateSkeletonAnimationTree, this, animTree, _updateFrameNum);
		_animCalculationResult[_updateFrameNum] = Engine::getInstance().getModule<ThreadPool2>().enqueue(TaskType::COMMON, updateSkeletonAnimationTree, this, animTree, _updateFrameNum);
	}

    void MeshSkeleton::applyFrame(MeshAnimationTree* animTree) {
        const auto frameNum = animTree->frame();
        if ((_latency > 1 && frameNum != _updateFrameNum) || _latency == 1) {
            {
                // new vision
                // check the target frame already complete previous work
                if (needSkipAnimCalculation(frameNum)) {
                    // wait process complete for target frame
                    return;
                }
            }

            _updateFrameNum = frameNum;
            _animCalculationResult[_updateFrameNum] = Engine::getInstance().getModule<ThreadPool2>().enqueue(
                    TaskType::COMMON, updateSkeletonAnimationTree, this, animTree, _updateFrameNum);
        }
    }

    void MeshSkeleton::updateSkins(const uint8_t updateFrame) {
		size_t skinId = 0;
		_dirtySkins = false;
		for (const Mesh_Skin& s : _skins) {
			const Mesh_Node& h = _nodes[updateFrame][s.skeletonRoot]->value();

			const bool emptyInverse = !memcmp(&(h.modelMatrix), &emptyMatrix, sizeof(mat4f));
			const size_t numJoints = static_cast<uint32_t>(s.joints.size());

			std::vector<mat4f>& skin_matrices = _skinsMatrices[updateFrame][skinId];
			if (emptyInverse) {
				for (size_t i = 0; i < numJoints; ++i) {
					if (auto&& n = _nodes[updateFrame][s.joints[i]]->value(); n.dirtyModelTransform) {
						skin_matrices[i] = (n.modelMatrix * s.inverseBindMatrices[i]);
						_dirtySkins = true;
					}
				}
			} else {
				const mat4f inverseTransform = glm::inverse(h.modelMatrix);
				for (size_t i = 0; i < numJoints; ++i) {
					if (auto&& n = _nodes[updateFrame][s.joints[i]]->value(); n.dirtyModelTransform) {
						skin_matrices[i] = inverseTransform * (n.modelMatrix * s.inverseBindMatrices[i]);
						_dirtySkins = true;
					}
				}
			}

			++skinId;
		}
	}

	void MeshSkeleton::updateTransforms(const uint8_t updateFrame) {
		for (HierarchyRaw<Mesh_Node>* h : _hierarchyes[updateFrame]) {
			//h->execute(updateHierarchyMatrix);
			h->execute_with<HierarchyMatrixUpdater>();
		}
	}
	////////////////////////////////
	////////////////////////////////

	void Mesh::createRenderData() {
		const size_t renderDataCount = _meshData->renderData.size();

		_renderDescriptor.renderData = new vulkan::RenderData*[renderDataCount];
		uint8_t primitiveMode = 0xff;

		for (size_t i = 0; i < renderDataCount; ++i) {
			const size_t partsCount = _meshData->renderData[i].layouts.size();
			_renderDescriptor.renderData[i] = new vulkan::RenderData();
			_renderDescriptor.renderData[i]->createRenderParts(partsCount);
			for (size_t j = 0; j < partsCount; ++j) {
				auto&& layout = _meshData->renderData[i].layouts[j];
				if (primitiveMode == 0xff) {
					primitiveMode = layout.primitiveMode;
				}

				ENGINE_BREAK_CONDITION(primitiveMode == layout.primitiveMode)

				_renderDescriptor.renderData[i]->renderParts[j] = vulkan::RenderData::RenderPart{
													layout.firstIndex,	// firstIndex
													layout.indexCount,	// indexCount
													0,					// vertexCount (parameter no used with indexed render)
													0,					// firstVertex
													1,					// instanceCount (can change later)
													0,					// firstInstance (can change later)
													layout.vbOffset,	// vbOffset
													layout.ibOffset		// ibOffset
				};

				// min & max corners calculation
				const Mesh_Node& node = _skeleton->_nodes[0][_meshData->meshes[i].nodeIndex]->value();

				/*
				vec3f s;
				vec3f t;
				quatf r;

				decomposeMatrix(node.modelMatrix, s, r, t);
				const vec3f corner1 = s * layout.minCorner;
				const vec3f corner2 = s * layout.maxCorner;
				*/

				const vec3f corner1 = node.modelMatrix * vec4f(layout.minCorner, 1.0f);
				const vec3f corner2 = node.modelMatrix * vec4f(layout.maxCorner, 1.0f);

				//const vec3f corner1 = layout.minCorner;
				//const vec3f corner2 = layout.maxCorner;

				_minCorner.x = std::min(std::min(_minCorner.x, corner1.x), corner2.x);
				_minCorner.y = std::min(std::min(_minCorner.y, corner1.y), corner2.y);
				_minCorner.z = std::min(std::min(_minCorner.z, corner1.z), corner2.z);

				_maxCorner.x = std::max(std::max(_maxCorner.x, corner1.x), corner2.x);
				_maxCorner.y = std::max(std::max(_maxCorner.y, corner1.y), corner2.y);
				_maxCorner.z = std::max(std::max(_maxCorner.z, corner1.z), corner2.z);
			}

			_renderDescriptor.renderData[i]->indexes = _meshData->indicesBuffer;
			_renderDescriptor.renderData[i]->vertexes = _meshData->verticesBuffer;
		}

		_renderDescriptor.renderDataCount = renderDataCount;
		
		vulkan::PrimitiveTopology topology = vulkan::PrimitiveTopology::TRIANGLE_LIST;
		bool enableRestartTopology = false;
		switch (_meshData->renderData[0].layouts[0].primitiveMode) {
			case 0:
				topology = vulkan::PrimitiveTopology::POINT_LIST;
				break;
			case 1:
				topology = vulkan::PrimitiveTopology::LINE_LIST;
				break;
			case 2:
				topology = vulkan::PrimitiveTopology::LINE_LIST;
				enableRestartTopology = true;
				break;
			case 3:
				topology = vulkan::PrimitiveTopology::LINE_STRIP;
				break;
			case 4:
				topology = vulkan::PrimitiveTopology::TRIANGLE_LIST;
				break;
			case 5:
				topology = vulkan::PrimitiveTopology::TRIANGLE_STRIP;
				break;
			case 6:
				topology = vulkan::PrimitiveTopology::TRIANGLE_FAN;
				break;
		}

		_renderState.topology = { topology, enableRestartTopology };
		_renderState.rasterizationState = vulkan::VulkanRasterizationState(vulkan::CullMode::CULL_MODE_BACK, vulkan::PoligonMode::POLYGON_MODE_FILL);
		_renderState.blendMode = vulkan::CommonBlendModes::blend_none;
		_renderState.depthState = vulkan::VulkanDepthState(true, true, VK_COMPARE_OP_LESS);
		_renderState.stencilState = vulkan::VulkanStencilState(false);
        _renderState.vertexDescription.bindings_strides.emplace_back(0, sizeOfVertex());
        _renderState.vertexDescription.attributes = getVertexInputAttributes();

		// fixed gpu layout works
		_fixedGpuLayouts.resize(4);
		_fixedGpuLayouts[0].second = { "camera_matrix", ViewParams::Ids::CAMERA_TRANSFORM };
		_fixedGpuLayouts[1].second = { "model_matrix" };
		_fixedGpuLayouts[2].second = { "skin_matrixes" };
		_fixedGpuLayouts[3].second = { "skin_matrixes_count" };
	}

	std::vector<VkVertexInputAttributeDescription> Mesh::getVertexInputAttributes() const {
        VertexAttributes attributes;
		for (uint8_t i = 0u; i < std::min(static_cast<uint8_t>(16u), static_cast<uint8_t>(gltf::AttributesSemantic::SEMANTICS_COUNT)); ++i) {
			if (_semanticMask & (1u << i)) {
				switch (static_cast<gltf::AttributesSemantic>(i)) {
					case gltf::AttributesSemantic::POSITION:
					case gltf::AttributesSemantic::NORMAL:
					{
                        attributes.set<float, 0u>(3u);
					}
						break;
					case gltf::AttributesSemantic::TANGENT:
					case gltf::AttributesSemantic::JOINTS:
					case gltf::AttributesSemantic::WEIGHT:
					{
                        attributes.set<float, 0u>(4u);
					}
						break;
                    case gltf::AttributesSemantic::COLOR:
                    {
                        attributes.set<uint8_t, 0u>(4u);
                    }
                        break;
					case gltf::AttributesSemantic::TEXCOORD_0:
					case gltf::AttributesSemantic::TEXCOORD_1:
					case gltf::AttributesSemantic::TEXCOORD_2:
					case gltf::AttributesSemantic::TEXCOORD_3:
					case gltf::AttributesSemantic::TEXCOORD_4:
					{
                        attributes.set<float, 0u>(2u);
					}
						break;
					default:
						break;
				}
			}
		}

        return VulkanAttributesProvider::convert(attributes);
	}

	uint32_t Mesh::sizeOfVertex() const { return _meshData ? (sizeof(float) * _meshData->vertexSize) : 0u; }

	void Mesh::draw(const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) {
		if (!_skeleton) return;

        // old variant
//		const uint8_t renderFrameNum = (_skeleton->_updateFrameNum + 1) % _skeleton->_latency;
//		_skeleton->checkAnimCalculation(renderFrameNum);

        //new vision
        const uint8_t renderFrameNum = _skeleton->getUpdatedFrameNum();

		for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
			vulkan::RenderData* r_data = _renderDescriptor.renderData[i];
			if (r_data == nullptr || r_data->pipeline == nullptr) continue;

			const Mesh_Node& node = _skeleton->_nodes[renderFrameNum][_meshData->meshes[i].nodeIndex]->value();
			
			if (_fixedGpuLayouts[2].first && _skeleton->dirtySkins()) {
				if (node.skinIndex != 0xffff) {
					r_data->setParamForLayout(_fixedGpuLayouts[2].first, &(_skeleton->_skinsMatrices[renderFrameNum][node.skinIndex][0]), false, _skeleton->_skinsMatrices[renderFrameNum][node.skinIndex].size());
				} else {
					r_data->setParamForLayout(_fixedGpuLayouts[2].first, const_cast<mat4f*>(&(engine::emptyMatrix)), false, 1);
				}
			}

			r_data->setParamForLayout(_fixedGpuLayouts[0].first, &const_cast<mat4f&>(cameraMatrix), false, 1);

			mat4f model = worldMatrix * node.modelMatrix;
			r_data->setParamForLayout(_fixedGpuLayouts[1].first, &model, true, 1);

			r_data->prepareRender(/*commandBuffer*/);
			r_data->render(commandBuffer, currentFrame);
		}
	}

	void Mesh::updateRenderData(const mat4f& worldMatrix, const bool worldMatrixChanged) { // called when mesh is visible
		if (!_skeleton) return;
        _skeleton->_requestAnimUpdate = true;

		_modelMatrixChanged |= worldMatrixChanged;

        // old variant
		//const uint8_t renderFrameNum = (_skeleton->_updateFrameNum + 1) % _skeleton->_latency;
		//_skeleton->checkAnimCalculation(renderFrameNum);

        // new vision
        const uint8_t renderFrameNum = _skeleton->getUpdatedFrameNum();

		for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
			vulkan::RenderData* r_data = _renderDescriptor.renderData[i];
			if (r_data == nullptr || r_data->pipeline == nullptr) continue;

			const Mesh_Node& node = _skeleton->_nodes[renderFrameNum][_meshData->meshes[i].nodeIndex]->value();
			
			// для организации инстансной отрисовки мешей в различных кадрах анимаций, нужно делать storage_buffer для матриц костей и для нужной модельки брать правильное смещение
			// + может быть нужно дописать возможность передачи параметра в r_data в нужное смещение от начала (либо копировать данные костей в общий буффер, а потом его устанавливать для gpuProgram)
			if (_fixedGpuLayouts[2].first && _skeleton->dirtySkins()) {
				if (node.skinIndex != 0xffff) {
					r_data->setParamForLayout(_fixedGpuLayouts[2].first, &(_skeleton->_skinsMatrices[renderFrameNum][node.skinIndex][0]), false, _skeleton->_skinsMatrices[renderFrameNum][node.skinIndex].size());
					//r_data->setParamForLayout(0u, _fixedGpuLayouts[2].first, &(_skeleton->_skinsMatrices[renderFrameNum][node.skinIndex][0]), _skeleton->_skinsMatrices[renderFrameNum][node.skinIndex].size());
					if (_fixedGpuLayouts[3].first) {
						r_data->setParamForLayout(_fixedGpuLayouts[3].first, _skeleton->_skinsMatrices[renderFrameNum][node.skinIndex].size(), true);
					}
				} else {
					r_data->setParamForLayout(_fixedGpuLayouts[2].first, const_cast<mat4f*>(&(engine::emptyMatrix)), false, 1);
					if (_fixedGpuLayouts[3].first) {
						r_data->setParamForLayout(_fixedGpuLayouts[3].first, 0, true);
					}
				}
			}

			if (node.dirtyModelTransform || _modelMatrixChanged) {
				mat4f model = worldMatrix * node.modelMatrix;
				r_data->setParamForLayout(_fixedGpuLayouts[1].first, &model, true, 1);
			}
		}

		_modelMatrixChanged = false;
	}

	void Mesh::createWithData(Mesh_Data* mData, const uint16_t semantic_mask, const uint8_t latency) {
		_meshData = mData;
		_semanticMask = semantic_mask;

		_minCorner = vec3f(std::numeric_limits<float>::max());
		_maxCorner = vec3f(std::numeric_limits<float>::min());

		if (_skeleton == nullptr) {
			_skeleton = std::make_shared<MeshSkeleton>(mData, latency);
		}

		// create render data
		createRenderData();
	}

	Mesh::~Mesh() {
		_skeleton = nullptr;
	}

	void Mesh::drawBoundingBox(const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) {
		if (!_skeleton) return;
		Engine::getInstance().getModule<Graphics>().getRenderHelper()->drawBoundingBox(_minCorner, _maxCorner, cameraMatrix, worldMatrix, commandBuffer, currentFrame, true);
		//Engine::getInstance().getModule<Graphics>()->getRenderHelper()->drawSphere((_minCorner + _maxCorner) * 0.5f, 1.0f, cameraMatrix, worldMatrix, commandBuffer, currentFrame, true);
	}

}