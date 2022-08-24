#include "Mesh.h"

#include "MeshData.h"
#include "Loader_gltf.h"

#include "../../Core/Engine.h"
#include "../Graphics.h"
#include "../Render/RenderHelper.h"
#include "../Vulkan/vkRenderer.h"
#include "AnimationTree.h"

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

					const glm::vec4& v0 = sampler.outputs[i];

					switch (sampler.interpolation) {
					case Mesh_Animation::Interpolation::LINEAR:
					{
						const glm::vec4& v1 = sampler.outputs[i + 1];
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
								target.setRotation(glm::quat(v0.w, v0.x, v0.y, v0.z));
							} else {
								const glm::quat q1(v0.w, v0.x, v0.y, v0.z);
								const glm::quat q2(v1.w, v1.x, v1.y, v1.z);
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
							target.setRotation(glm::quat(v0.w, v0.x, v0.y, v0.z));
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
		animTree->calculate(updateFrame);
		if (token) return;

		animTree->applyToSkeleton(skeleton, updateFrame);

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
				memcpy(&(_skinsMatrices[ii][i][0]), &(mData->skins[i].inverseBindMatrices[0]), matricesCount * sizeof(glm::mat4));
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
			_animCalculationResult[i].cancel();

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
		mNode.scale = glm::vec3(node.scale.x, node.scale.y, node.scale.z);
		mNode.rotation = glm::quat(node.rotation.w, node.rotation.x, node.rotation.y, node.rotation.z);
		mNode.translation = glm::vec3(node.translation.x, node.translation.y, node.translation.z);

		mNode.skinIndex = node.skin;

		HierarchyRaw<Mesh_Node>* nodesHierarchy = new HierarchyRaw<Mesh_Node>(std::move(mNode));
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

		_animCalculationResult[_updateFrameNum] = Engine::getInstance().getModule<ThreadPool>()->enqueue(TaskType::COMMON, 0, updateSkeletonAnimation, this, atime, animation, _updateFrameNum);
	}

	void MeshSkeleton::updateAnimation(const float time, MeshAnimationTree* animTree) {
		_updateFrameNum = (_updateFrameNum + 1) % _latency;

		if (time == 0.0f) return;

		animTree->update(time, _updateFrameNum); // просто пересчет времени

		_animCalculationResult[_updateFrameNum] = Engine::getInstance().getModule<ThreadPool>()->enqueue(TaskType::COMMON, 0, updateSkeletonAnimationTree, this, animTree, _updateFrameNum);
	}

	
	void MeshSkeleton::updateSkins(const uint8_t updateFrame) {
		size_t skinId = 0;
		for (const Mesh_Skin& s : _skins) {
			const Mesh_Node& h = _nodes[updateFrame][s.skeletonRoot]->value();

			const bool emptyInverse = !memcmp(&(h.modelMatrix), &emptyMatrix, sizeof(glm::mat4));
			const size_t numJoints = static_cast<uint32_t>(s.joints.size());

			std::vector<glm::mat4>& skin_matrices = _skinsMatrices[updateFrame][skinId];
			if (emptyInverse) {
				for (size_t i = 0; i < numJoints; ++i) {
					if (auto&& n = _nodes[updateFrame][s.joints[i]]->value(); n.dirtyModelTransform) {
						skin_matrices[i] = (n.modelMatrix * s.inverseBindMatrices[i]);
					}
				}
			} else {
				const glm::mat4 inverseTransform = glm::inverse(h.modelMatrix);
				for (size_t i = 0; i < numJoints; ++i) {
					if (auto&& n = _nodes[updateFrame][s.joints[i]]->value(); n.dirtyModelTransform) {
						skin_matrices[i] = inverseTransform * (n.modelMatrix * s.inverseBindMatrices[i]);
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

		for (size_t i = 0; i < renderDataCount; ++i) {
			const size_t partsCount = _meshData->renderData[i].layouts.size();
			_renderDescriptor.renderData[i] = new vulkan::RenderData();
			_renderDescriptor.renderData[i]->createRenderParts(partsCount);
			for (size_t j = 0; j < partsCount; ++j) {
				auto&& layout = _meshData->renderData[i].layouts[j];
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

				//glm::vec3 s;
				//glm::vec3 t;
				//glm::quat r;

				//decomposeMatrix(node.globalMatrix, s, r, t);
				//glm::vec3 corner1 = s * layout.minCorner;
				//glm::vec3 corner2 = s * layout.maxCorner;

				const glm::vec3 corner1 = node.modelMatrix * glm::vec4(layout.minCorner, 1.0f);
				const glm::vec3 corner2 = node.modelMatrix * glm::vec4(layout.maxCorner, 1.0f);

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

		_renderState.vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeOfVertex()));
		_renderState.topology = { vulkan::TRIANGLE_LIST, false };
		_renderState.rasterisationState = vulkan::VulkanRasterizationState(vulkan::CULL_MODE_BACK, vulkan::POLYGON_MODE_FILL);
		_renderState.blendMode = vulkan::CommonBlendModes::blend_none;
		_renderState.depthState = vulkan::VulkanDepthState(true, true, VK_COMPARE_OP_LESS);
		_renderState.stencilState = vulkan::VulkanStencilState(false);
	}

	std::vector<VkVertexInputAttributeDescription> Mesh::getVertexInputAttributes() const {
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
		vertexInputAttributes.reserve(static_cast<uint8_t>(gltf::AttributesSemantic::SEMANTICS_COUNT));
		uint32_t attribute_location = 0;
		uint32_t attribute_offset = 0;
		for (uint8_t i = 0; i < 16; ++i) {
			if (_semanticMask & (1 << i)) {
				switch (static_cast<gltf::AttributesSemantic>(i)) {
					case gltf::AttributesSemantic::POSITION:
					case gltf::AttributesSemantic::NORMAL:
					{
						vertexInputAttributes.emplace_back(
							VkVertexInputAttributeDescription{
								attribute_location++,		// location
								0,							// binding
								VK_FORMAT_R32G32B32_SFLOAT, // format
								attribute_offset			// offset
							}
						);
						attribute_offset += 12;
					}
						break;
					case gltf::AttributesSemantic::TANGENT:
					case gltf::AttributesSemantic::COLOR:
					case gltf::AttributesSemantic::JOINTS:
					case gltf::AttributesSemantic::WEIGHT:
					{
						vertexInputAttributes.emplace_back(
							VkVertexInputAttributeDescription{
								attribute_location++,			// location
								0,								// binding
								VK_FORMAT_R32G32B32A32_SFLOAT,	// format
								attribute_offset				// offset
							}
						);
						attribute_offset += 16;
					}
						break;
					case gltf::AttributesSemantic::TEXCOORD_0:
					case gltf::AttributesSemantic::TEXCOORD_1:
					case gltf::AttributesSemantic::TEXCOORD_2:
					case gltf::AttributesSemantic::TEXCOORD_3:
					case gltf::AttributesSemantic::TEXCOORD_4:
					{
						vertexInputAttributes.emplace_back(
							VkVertexInputAttributeDescription{
								attribute_location++,		// location
								0,							// binding
								VK_FORMAT_R32G32_SFLOAT,	// format
								attribute_offset			// offset
							}
						);
						attribute_offset += 8;
					}
						break;
					default:
						break;
				}
			}
		}

		vertexInputAttributes.shrink_to_fit();
		return vertexInputAttributes;
	}

	uint32_t Mesh::sizeOfVertex() const { return _meshData ? (sizeof(float) * _meshData->vertexSize) : 0; }

	void Mesh::setPipeline(vulkan::VulkanPipeline* p) {

		if (_renderDescriptor.renderData[0]->pipeline == nullptr || _renderDescriptor.renderData[0]->pipeline->program != p->program) {
			_camera_matrix = p->program->getGPUParamLayoutByName("camera_matrix");
			_model_matrix = p->program->getGPUParamLayoutByName("model_matrix");
			_skin_matrices = p->program->getGPUParamLayoutByName("skin_matrixes");
			_use_skin = p->program->getGPUParamLayoutByName("use_skin");
		}

		_renderDescriptor.setCameraMatrix(_camera_matrix);
		for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
			_renderDescriptor.renderData[i]->setPipeline(p);
		}
	}

	void Mesh::setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass) {

		if (_renderDescriptor.renderDataCount == 0) return;

		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = getVertexInputAttributes();
		
		if (vertexInputAttributes.empty()) return;

		_renderState.vertexDescription.attributesCount = static_cast<uint32_t>(vertexInputAttributes.size());
		_renderState.vertexDescription.attributes = vertexInputAttributes.data();

		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
		vulkan::VulkanPipeline* pipeline = renderer->getGraphicsPipeline(_renderState, program, renderPass);

		_renderState.vertexDescription.attributesCount = 0;
		_renderState.vertexDescription.attributes = nullptr;

		if (_renderDescriptor.renderData[0]->pipeline == pipeline) return;

		setPipeline(pipeline);
	}

	void Mesh::onPipelineAttributesChanged() {
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = getVertexInputAttributes();

		_renderState.vertexDescription.attributesCount = static_cast<uint32_t>(vertexInputAttributes.size());
		_renderState.vertexDescription.attributes = vertexInputAttributes.data();

		vulkan::VulkanPipeline* pipeline = Engine::getInstance().getModule<Graphics>()->getRenderer()->getGraphicsPipeline(_renderState, _renderDescriptor.renderData[0]->pipeline->program);
		
		_renderState.vertexDescription.attributesCount = 0;
		_renderState.vertexDescription.attributes = nullptr;

		setPipeline(pipeline);
	}

	void Mesh::draw(const glm::mat4& cameraMatrix, const glm::mat4& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) {
		if (!_skeleton) return;

		const uint8_t renderFrameNum = (_skeleton->_updateFrameNum + 1) % _skeleton->_latency;

		_skeleton->checkAnimCalculation(renderFrameNum);

		for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
			vulkan::RenderData* r_data = _renderDescriptor.renderData[i];
			if (r_data == nullptr || r_data->pipeline == nullptr) continue;

			const Mesh_Node& node = _skeleton->_nodes[renderFrameNum][_meshData->meshes[i].nodeIndex]->value();
			glm::mat4 model = worldMatrix * node.modelMatrix;

			if (_use_skin) {
				int useSkin = node.skinIndex != 0xffff ? 1 : 0;
				r_data->setParamForLayout(_use_skin, &useSkin, false, 1);
			}

			if (node.skinIndex != 0xffff && _skin_matrices) {
				r_data->setParamForLayout(_skin_matrices, &(_skeleton->_skinsMatrices[renderFrameNum][node.skinIndex][0]), false, _skeleton->_skinsMatrices[renderFrameNum][node.skinIndex].size());
			}

			r_data->setParamForLayout(_camera_matrix, &const_cast<glm::mat4&>(cameraMatrix), false, 1);
			r_data->setParamForLayout(_model_matrix, &model, true, 1);

			r_data->prepareRender(commandBuffer);
			r_data->render(commandBuffer, currentFrame);
		}
	}

	void Mesh::updateRenderData(const glm::mat4& worldMatrix) {
		if (!_skeleton) return;

		const uint8_t renderFrameNum = (_skeleton->_updateFrameNum + 1) % _skeleton->_latency;

		_skeleton->checkAnimCalculation(renderFrameNum);

		for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
			vulkan::RenderData* r_data = _renderDescriptor.renderData[i];
			if (r_data == nullptr || r_data->pipeline == nullptr) continue;

			const Mesh_Node& node = _skeleton->_nodes[renderFrameNum][_meshData->meshes[i].nodeIndex]->value();
			glm::mat4 model = worldMatrix * node.modelMatrix;

			if (_use_skin) {
				int32_t useSkin = node.skinIndex != 0xffff ? 1 : 0;
				r_data->setParamForLayout(_use_skin, &useSkin, true, 1);
			}

			if (node.skinIndex != 0xffff && _skin_matrices) {
				r_data->setParamForLayout(_skin_matrices, &(_skeleton->_skinsMatrices[renderFrameNum][node.skinIndex][0]), false, _skeleton->_skinsMatrices[renderFrameNum][node.skinIndex].size());
			}

			r_data->setParamForLayout(_model_matrix, &model, true, 1);
		}
	}

	void Mesh::createWithData(Mesh_Data* mData, const uint16_t semantic_mask, const uint8_t latency) {
		_meshData = mData;
		_semanticMask = semantic_mask;

		_minCorner = glm::vec3(std::numeric_limits<float>::max());
		_maxCorner = glm::vec3(std::numeric_limits<float>::min());

		if (_skeleton == nullptr) {
			_skeleton = std::make_shared<MeshSkeleton>(mData, latency);
		}

		// create render data
		createRenderData();
	}

	Mesh::~Mesh() {
		_skeleton = nullptr;

		_renderDescriptor.destroy();

		_camera_matrix = nullptr;
		_model_matrix = nullptr;
		_skin_matrices = nullptr;
		_use_skin = nullptr;
	}

	void Mesh::drawBoundingBox(const glm::mat4& cameraMatrix, const glm::mat4& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) {
		if (!_skeleton) return;
		Engine::getInstance().getModule<Graphics>()->getRenderHelper()->drawBoundingBox(_minCorner, _maxCorner, cameraMatrix, worldMatrix, commandBuffer, currentFrame, true);
	}

}