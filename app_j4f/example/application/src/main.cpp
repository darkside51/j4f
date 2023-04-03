#include <Engine/Core/Application.h>
#include <Engine/Core/Engine.h>
#include <Engine/File/FileManager.h>
#include <Platform_inc.h>
#include <Engine/Log/Log.h>

////////////////////////////////////////////////////
#include <Engine/Core/AssetManager.h>
#include <Engine/Graphics/GpuProgramsManager.h>
#include <Engine/Graphics/Texture/TextureLoader.h>
#include <Engine/Graphics/Mesh/Mesh.h>
#include <Engine/Graphics/Mesh/MeshLoader.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>
#include <Engine/Graphics/Plain/Plain.h>
#include <Engine/Core/Math/functions.h>
#include <Engine/Graphics/Scene/Camera.h>
#include <Engine/Graphics/Render/RenderHelper.h>
#include <Engine/Graphics/Render/AutoBatchRender.h>
#include <Engine/Graphics/Render/RenderList.h>
#include <Engine/Input/Input.h>
#include <Engine/Utils/Debug/Profiler.h>
#include <Engine/Graphics/Text/FontLoader.h>
#include <Engine/Graphics/Text/BitmapFont.h>

#include <Engine/Core/Math/random.h>
#include <Engine/Core/Threads/FlowTask.h>
#include <Engine/Core/Threads/Looper.h>

#include <Engine/ECS/Component.h>

#include <Engine/Graphics/Features/Shadows/CascadeShadowMap.h>
#include <Engine/Graphics/Features/Shadows/ShadowMapHelper.h>

#include <Engine/Graphics/Scene/Node.h>
#include <Engine/Graphics/Scene/NodeRenderListHelper.h>

#include <Engine/Events/Bus.h>
#include <Engine/Utils/Debug/Assert.h>

#include <Engine/Graphics/Render/InstanceHelper.h>

#include <Engine/Utils/ImguiStatObserver.h>
#include <Engine/Graphics/UI/ImGui/Imgui.h>

#include <Engine/Graphics/Animation/AnimationManager.h>

//#include <format>

#include <charconv>

#include <unordered_set>

#include <mutex>

namespace engine {
	vulkan::VulkanGpuProgram* grass_default = nullptr;

	MeshGraphicsDataBuffer* meshesGraphicsBuffer;

	Camera* camera = nullptr;
	Camera* camera2 = nullptr;

	CascadeShadowMap* shadowMap = nullptr;
	BitmapFont *bitmapFont = nullptr;

	NodeRenderer<Mesh>* mesh = nullptr;
	NodeRenderer<Mesh>* mesh2 = nullptr;
	NodeRenderer<Mesh>* mesh3 = nullptr;
	NodeRenderer<Mesh>* mesh4 = nullptr;
	NodeRenderer<Mesh>* mesh5 = nullptr;
	NodeRenderer<Mesh>* mesh6 = nullptr;
	NodeRenderer<Mesh>* mesh7 = nullptr;

	std::vector<NodeRenderer<Mesh>*> testMehsesVec;

	NodeRenderer<Mesh>* grassMesh = nullptr;
	NodeRenderer<Plain>* plainTest = nullptr;

    NodeRenderer<ImguiGraphics> *imgui = nullptr;

	MeshAnimationTree* animTree = nullptr;
	MeshAnimationTree* animTree2 = nullptr;
	MeshAnimationTree* animTreeWindMill = nullptr;

	vulkan::VulkanTexture* texture_1 = nullptr;
	vulkan::VulkanTexture* texture_text = nullptr;
	vulkan::VulkanTexture* texture_floor_mask = nullptr;
	vulkan::VulkanTexture* texture_floor_normal = nullptr;
	vulkan::VulkanTexture* texture_array_test = nullptr;

	vulkan::VulkanGpuProgram* program_mesh_skin = nullptr;
	vulkan::VulkanGpuProgram* program_mesh = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_skin_shadow = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_shadow = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_with_stroke = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_skin_with_stroke = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_instance = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_instance_shadow = nullptr;

	VkClearValue clearValues[2];

	RenderList sceneRenderList;
	RenderList shadowRenderList;
	RenderList uiRenderList;

	glm::vec3 wasd(0.0f);

	///////////////////////////
	/// cascade shadow map
	constexpr uint8_t SHADOW_MAP_CASCADE_COUNT = 3;
	constexpr uint16_t SHADOWMAP_DIM = 1536; // VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	//constexpr uint16_t SHADOWMAP_DIM = 1024; // VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
	glm::vec3 lightPos = glm::vec3(-460.0f, -600.0f, 1000.0f) * 2.0f;
	//// cascade shadow map

	glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
	//glm::vec2 lightMinMax(0.075f, 3.0f);
	glm::vec2 lightMinMax(0.5f, 1.0f);
	const float saturation = 1.2f;

	H_Node* rootNode;
	H_Node* uiNode;
	std::vector<H_Node*> shadowCastNodes;
	bool cameraMatrixChanged = true;

    ImguiStatObserver *statObserver = nullptr;

	//constexpr bool renderBounds = false;
	bool renderBounds = false;

	glm::vec3 targetCameraRotation(0.0f, 0.0f, 0.0f);

	class SkyBoxRenderer : public RenderedEntity {
	public:
		~SkyBoxRenderer() {
			_verticesBuffer->destroy();
			_indicesBuffer->destroy();
			delete _verticesBuffer;
			delete _indicesBuffer;
		}
		void createRenderData() {

			createGpuData();

			const size_t renderDataCount = 1;

			_renderDescriptor.renderData = new vulkan::RenderData * [renderDataCount];

			for (size_t i = 0; i < renderDataCount; ++i) {
				const size_t partsCount = 1;
				_renderDescriptor.renderData[i] = new vulkan::RenderData();
				_renderDescriptor.renderData[i]->createRenderParts(partsCount);
				for (size_t j = 0; j < partsCount; ++j) {
					_renderDescriptor.renderData[i]->renderParts[j] = vulkan::RenderData::RenderPart{
														0,	// firstIndex
														36,	// indexCount
														0,	// vertexCount (parameter no used with indexed render)
														0,	// firstVertex
														1,	// instanceCount (can change later)
														0,	// firstInstance (can change later)
														0,	// vbOffset
														0	// ibOffset
					};
				}

				_renderDescriptor.renderData[i]->indexes = _indicesBuffer;
				_renderDescriptor.renderData[i]->vertexes = _verticesBuffer;
			}

			_renderDescriptor.renderDataCount = renderDataCount;

			_renderState.vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(SkyBoxVertex)));
			_renderState.topology = { vulkan::PrimitiveTopology::TRIANGLE_LIST, false };
			_renderState.rasterizationState = vulkan::VulkanRasterizationState(vulkan::CullMode::CULL_MODE_NONE, vulkan::PoligonMode::POLYGON_MODE_FILL);
			_renderState.blendMode = vulkan::CommonBlendModes::blend_none;
			_renderState.depthState = vulkan::VulkanDepthState(true, true, VK_COMPARE_OP_LESS);
			_renderState.stencilState = vulkan::VulkanStencilState(false);

			_vertexInputAttributes = SkyBoxVertex::getVertexAttributesDescription();
			if (!_vertexInputAttributes.empty()) {
				_renderState.vertexDescription.attributesCount = static_cast<uint32_t>(_vertexInputAttributes.size());
				_renderState.vertexDescription.attributes = _vertexInputAttributes.data();
			}

			// fixed gpu layout works
			_fixedGpuLayouts.resize(2);
			_fixedGpuLayouts[0].second = "camera_matrix";
			_fixedGpuLayouts[1].second = "model_matrix";

			auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();
			std::vector<engine::ProgramStageInfo> psi;
			psi.emplace_back(ProgramStage::VERTEX, "resources/shaders/skyBox.vsh.spv");
			psi.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/skyBox.psh.spv");
			vulkan::VulkanGpuProgram* program = gpuProgramManager->getProgram(psi);

			setProgram(program);
		}

		inline void updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged) {
			if (worldMatrixChanged) {
				_renderDescriptor.renderData[0]->setParamForLayout(_fixedGpuLayouts[1].first, &const_cast<glm::mat4&>(worldMatrix), false, 1);
			}
		}

		inline void updateModelMatrixChanged(const bool worldMatrixChanged) {}

	private:
		struct SkyBoxVertex {
			float position[3];
			float uv[3];

			inline static std::vector<VkVertexInputAttributeDescription> getVertexAttributesDescription() {
				std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(2);
				// attribute location 0: position
				vertexInputAttributs[0].binding = 0;
				vertexInputAttributs[0].location = 0;
				vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
				vertexInputAttributs[0].offset = offset_of(SkyBoxVertex, position);
				// attribute location 1: uv
				vertexInputAttributs[1].binding = 0;
				vertexInputAttributs[1].location = 1;
				vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				vertexInputAttributs[1].offset = offset_of(SkyBoxVertex, uv);

				return vertexInputAttributs;
			}
		};

		void createGpuData() {
			const std::vector<SkyBoxVertex> vtx = {
				{{ -1.0f, -1.0f, -1.0f},	{1.0f, 0.0f, 0.0f}},
				{{	1.0f, -1.0f, -1.0f},	{1.0f, 0.0f, 0.0f}},
				{{ -1.0f, 1.0f, -1.0f},		{1.0f, 0.0f, 0.0f}},
				{{	1.0f, 1.0f, -1.0f},		{1.0f, 0.0f, 0.0f}},

				{{ -1.0f, 1.0f, -1.0f},	{0.0f, 1.0f, 0.0f}},
				{{	1.0f, 1.0f, -1.0f},	{0.0f, 1.0f, 0.0f}},
				{{ -1.0f, 1.0f, 1.0f},	{0.0f, 1.0f, 1.0f}},
				{{	1.0f, 1.0f, 1.0f},	{0.0f, 1.0f, 1.0f}},

				{{ -1.0f, -1.0f, -1.0f},	{0.0f, 0.0f, 0.0f}},
				{{	-1.0f, 1.0f, -1.0f},	{0.0f, 0.0f, 0.0f}},
				{{ -1.0f, -1.0f, 1.0f},		{0.0f, 0.0f, 1.0f}},
				{{	-1.0f, 1.0f, 1.0f},		{0.0f, 0.0f, 1.0f}},

				{{ 1.0f, -1.0f, -1.0f},	{0.0f, 0.0f, 0.0f}},
				{{ 1.0f, 1.0f, -1.0f},	{0.0f, 0.0f, 0.0f}},
				{{ 1.0f, -1.0f, 1.0f},	{0.0f, 0.0f, 1.0f}},
				{{ 1.0f, 1.0f, 1.0f},	{0.0f, 0.0f, 1.0f}},

				{{ -1.0f, -1.0f, -1.0f},	{0.0f, 1.0f, 0.0f}},
				{{	1.0f, -1.0f, -1.0f},	{0.0f, 1.0f, 0.0f}},
				{{ -1.0f, -1.0f, 1.0f},		{0.0f, 1.0f, 1.0f}},
				{{	1.0f, -1.0f, 1.0f},		{0.0f, 1.0f, 1.0f}},

				{{ -1.0f, -1.0f, 1.0f},	{1.0f, 0.0f, 1.0f}},
				{{	1.0f, -1.0f, 1.0f},	{1.0f, 0.0f, 1.0f}},
				{{ -1.0f, 1.0f, 1.0f},	{1.0f, 0.0f, 1.0f}},
				{{	1.0f, 1.0f, 1.0f},	{1.0f, 0.0f, 1.0f}},
			};

			const std::vector<uint32_t> idx = {
				0,1,2,		2,1,3,
				4,5,6,		6,5,7,
				8,9,10,		10,9,11,
				12,13,14,	14,13,15,
				16,17,18,	18,17,19,
				20,21,22,	22,21,23
			};

			////////////////////////
			const uint32_t vertexBufferSize = static_cast<uint32_t>(vtx.size()) * sizeof(SkyBoxVertex);
			const uint32_t indexBufferSize = static_cast<uint32_t>(idx.size()) * sizeof(uint32_t);

			auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();

			vulkan::VulkanBuffer stage_vertices;
			vulkan::VulkanBuffer stage_indices;

			renderer->getDevice()->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stage_vertices,
				vertexBufferSize
			);

			renderer->getDevice()->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stage_indices,
				indexBufferSize
			);

			//
			_verticesBuffer = new vulkan::VulkanBuffer();
			renderer->getDevice()->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				0,
				_verticesBuffer,
				vertexBufferSize
			);

			_indicesBuffer = new vulkan::VulkanBuffer();
			renderer->getDevice()->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				0,
				_indicesBuffer,
				indexBufferSize
			);

			stage_vertices.upload(&vtx[0], vertexBufferSize, 0, false);
			stage_indices.upload(&idx[0], indexBufferSize, 0, false);

			auto&& copyPool = renderer->getDevice()->getCommandPool(vulkan::GPUQueueFamily::F_GRAPHICS);
			auto&& cmdBuffer = renderer->getDevice()->createVulkanCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, copyPool);
			cmdBuffer.begin();

			VkBufferCopy regionV{ 0, 0, stage_vertices.m_size };
			VkBufferCopy regionI{ 0, 0, stage_indices.m_size };
			cmdBuffer.cmdCopyBuffer(stage_vertices, *_verticesBuffer, &regionV);
			cmdBuffer.cmdCopyBuffer(stage_indices, *_indicesBuffer, &regionI);

			cmdBuffer.flush(renderer->getMainQueue());

			stage_vertices.destroy();
			stage_indices.destroy();
		}

		vulkan::VulkanBuffer* _verticesBuffer = nullptr;
		vulkan::VulkanBuffer* _indicesBuffer = nullptr;
	};

	////
	class GrassRenderer {
	public:
		GrassRenderer(const uint32_t instanceCount) : _instanceCount(instanceCount), _mesh(nullptr) {
			auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();
			std::vector<engine::ProgramStageInfo> psi;
			psi.emplace_back(ProgramStage::VERTEX, "resources/shaders/grass.vsh.spv");
			psi.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/grass.psh.spv");
			grass_default = gpuProgramManager->getProgram(psi);

			glm::vec3 lightDir = as_normalized(-lightPos);

			auto l = grass_default->getGPUParamLayoutByName("lightDirection");
			grass_default->setValueToLayout(l, &lightDir, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			auto l2 = grass_default->getGPUParamLayoutByName("lightMinMax");
			grass_default->setValueToLayout(l2, &lightMinMax, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			auto l4 = grass_default->getGPUParamLayoutByName("lightColor");
			grass_default->setValueToLayout(l4, &lightColor, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			auto l5 = grass_default->getGPUParamLayoutByName("saturation");
			grass_default->setValueToLayout(l5, &saturation, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			shadowMap->registerProgramAsReciever(grass_default);

			std::vector<glm::mat4> grassTransforms(instanceCount);
			const int step = static_cast<int>(sqrtf(instanceCount));
			
			for (size_t i = 0; i < instanceCount; ++i) {
				const float space = engine::random(28, 29);
				const float scale_xy = (engine::random(15, 25) * 1350.0f);
				const float scale_z = (engine::random(5, 50) * 330.0f);

				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(scale_xy, scale_xy, scale_z));
				rotateMatrix_xyz(wtr, glm::vec3(0.0f, 0.0f, engine::random(-3.1415926, 3.1415926)));
				translateMatrixTo(wtr, glm::vec3(-(step * space * 0.5f) + (i % step) * space, -(step * space * 0.5f) + (i / step) * space, 0.0f));
				grassTransforms[i] = std::move(wtr);
			}

			auto lssbo = grass_default->getGPUParamLayoutByName("models");
			grass_default->setValueToLayout(lssbo, grassTransforms.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, sizeof(glm::mat4) * instanceCount, true);
		}

		GrassRenderer(const TextureData& img) : _mesh(nullptr) {
			auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();
			std::vector<engine::ProgramStageInfo> psi;
			psi.emplace_back(ProgramStage::VERTEX, "resources/shaders/grass.vsh.spv");
			psi.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/grass.psh.spv");
			grass_default = gpuProgramManager->getProgram(psi);

			glm::vec3 lightDir = as_normalized(-lightPos);		

			auto l = grass_default->getGPUParamLayoutByName("lightDirection");
			grass_default->setValueToLayout(l, &lightDir, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			auto l2 = grass_default->getGPUParamLayoutByName("lightMinMax");
			grass_default->setValueToLayout(l2, &lightMinMax, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			auto l4 = grass_default->getGPUParamLayoutByName("lightColor");
			grass_default->setValueToLayout(l4, &lightColor, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			auto l5 = grass_default->getGPUParamLayoutByName("saturation");
			grass_default->setValueToLayout(l5, &saturation, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			shadowMap->registerProgramAsReciever(grass_default);

			std::vector<glm::mat4> grassTransforms;
			const float space = 26.0f;
			const uint32_t* data = reinterpret_cast<const uint32_t*>(img.data());
			for (size_t x = 0; x < img.width(); ++x) {
				for (size_t y = 0; y < img.height(); ++y) {
					const uint32_t v = data[y * img.width() + x];
					if ((v & 0x000000f0) >= 240) {
						glm::mat4 wtr(1.0f);

						const float l = vec_length(glm::vec2(-1024.0f + 26.0f * x, 1024.0f - 26.0f * y) - glm::vec2(570.0f, -250.0f));
						const float r = 100.0f;
						if (l >= r) {

							const int grassType = engine::random(0, 100) < 5 ? engine::random(2, 4) : engine::random(0, 1);
							if (grassType > 2) {
								const float scale_xyz = (engine::random(25.0f, 35.0f) * 400.0f);
								scaleMatrix(wtr, glm::vec3(scale_xyz, scale_xyz, scale_xyz));
							} else {
								const float scale_xyz = (engine::random(25.0f, 60.0f) * 490.0f);
								const float scale_z = (engine::random(25.0f, 60.0f) * 310.0f);
								scaleMatrix(wtr, glm::vec3(scale_xyz, scale_xyz, scale_z));
							}

							rotateMatrix_xyz(wtr, glm::vec3(0.0f, 0.0f, engine::random(-3.1415926, 3.1415926)));
							translateMatrixTo(wtr, glm::vec3(-1024.0f + engine::random(-10.0f, 10.0f) + x * space, 1024.0f + engine::random(-10.0f, 10.0f) - y * space, 0.0f));
							wtr[0][3] = grassType;
							grassTransforms.emplace_back(std::move(wtr));
						}

					}
				}
			}

			_instanceCount = grassTransforms.size();

			auto lssbo = grass_default->getGPUParamLayoutByName("models");
			grass_default->setValueToLayout(lssbo, grassTransforms.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, sizeof(glm::mat4) * _instanceCount, true);
		}

		~GrassRenderer() {
			if (_mesh) {
				delete _mesh;
				_mesh = nullptr;
			}
		}

		void setMesh(Mesh* asset) {
			_mesh = asset;

			auto&& rdescriptor = _mesh->getRenderDescriptor();
			for (size_t i = 0; i < rdescriptor.renderDataCount; ++i) {
				for (size_t j = 0; j < rdescriptor.renderData[i]->renderPartsCount; ++j) {
					rdescriptor.renderData[i]->renderParts[j].instanceCount = _instanceCount;
				}
			}
		}

		inline void updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged) {
			_mesh->updateRenderData(worldMatrix, worldMatrixChanged);
		}

		inline void updateModelMatrixChanged(const bool worldMatrixChanged) {
			_mesh->updateModelMatrixChanged(worldMatrixChanged);
		}

		inline vulkan::VulkanGpuProgram* setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			return _mesh->setProgram(program, renderPass);
		}

		inline const RenderDescriptor& getRenderDescriptor() const { return _mesh->getRenderDescriptor(); }
		inline RenderDescriptor& getRenderDescriptor() { return _mesh->getRenderDescriptor(); }

	private:
		uint32_t _instanceCount;
		Mesh* _mesh;
	};

	using InstanceMeshRenderer = engine::InstanceRenderer<Mesh, engine::SimpleInstanceStrategy>;

	NodeRenderer<GrassRenderer>* grassMesh2 = nullptr;
	NodeRenderer<SkyBoxRenderer>* skyBox = nullptr;
	NodeRenderer<InstanceMeshRenderer>* forest = nullptr;

	GraphicsTypeUpdateSystem<Mesh> meshUpdateSystem;
	GraphicsTypeUpdateSystem<GrassRenderer> grassUpdateSystem;
	GraphicsTypeUpdateSystem<SkyBoxRenderer> skyBoxUpdateSystem;
	GraphicsTypeUpdateSystem<InstanceMeshRenderer> instanceMeshUpdateSystem;
	GraphicsTypeUpdateSystem<Plain> plainUpdateSystem;
	
	class ApplicationCustomData : public InputObserver, public ICameraTransformChangeObserver {
	public:

		void onCameraTransformChanged(const Camera* camera) override {
			shadowMap->updateCascades(camera);
			cameraMatrixChanged = true;

			const glm::vec3& p = camera->getPosition();
			program_mesh->setValueByName("camera_position", &p, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			program_mesh_skin->setValueByName("camera_position", &p, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			program_mesh_with_stroke->setValueByName("camera_position", &p, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			program_mesh_skin_with_stroke->setValueByName("camera_position", &p, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			program_mesh_instance->setValueByName("camera_position", &p, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
		}

        void onEngineInitComplete() {
            initCamera();
            create();

            camera->addObserver(this);
            Engine::getInstance().getModule<Input>()->addObserver(this);

            statObserver = new ImguiStatObserver(ImguiStatObserver::Location::top_right);
        }

		ApplicationCustomData() {
			PROFILE_TIME_SCOPED(ApplicationLoading)
			log("ApplicationCustomData");

			FileManager* fm = Engine::getInstance().getModule<FileManager>();
			auto&& fs = fm->getFileSystem<DefaultFileSystem>();
			fm->mapFileSystem(fs);
		}

		~ApplicationCustomData() {
			log("~ApplicationCustomData");

            delete statObserver;

			delete bitmapFont;

			camera->removeObserver(this);

			delete camera;
			delete camera2;

			//delete mesh;
			//delete mesh2;
			//delete mesh3;
			//delete mesh4;
			//delete mesh5;
			//delete mesh6;
			delete animTree;
			delete animTree2;
			delete animTreeWindMill;

			delete texture_text;

			delete meshesGraphicsBuffer;

			delete shadowMap;

			delete rootNode;
			delete uiNode;
		}

		bool onInputPointerEvent(const PointerEvent& event) override {
            if (imgui && imgui->graphics() && imgui->graphics()->onInputPointerEvent(event)) {
                return true;
            }

			static uint8_t m1 = 0;

			static float x = event.x;
			static float y = event.y;

			switch (event.state) {
				case InputEventState::IES_PRESS:
					m1 |= event.button == PointerButton::PBUTTON_0;
					x = event.x;
					y = event.y;
					break;
				case InputEventState::IES_RELEASE:
					m1 &= ~(event.button == PointerButton::PBUTTON_0);
					x = event.x;
					y = event.y;
					break;
				case InputEventState::IES_NONE:
				{
					if (m1) {
						constexpr float k = 0.0075f;
						targetCameraRotation = (camera->getRotation() + glm::vec3(-k * (event.y - y), 0.0f, k * (event.x - x)));
						//camera->setRotation(camera->getRotation() + glm::vec3(-k * (event.y - y), 0.0f, k * (event.x - x)));
						x = event.x;
						y = event.y;
					}
				}
					break;
				default:
					break;
			}

			return false;
		}

		bool onInputWheelEvent(const float dx, const float dy) override {
            if (imgui && imgui->graphics() && imgui->graphics()->onInputWheelEvent(dx, dy)) {
                return true;
            }
			camera->movePosition(glm::vec3(0.0f, 0.0f, dy * 20.0f));
			return true;
		}

		bool onInpuKeyEvent(const KeyEvent& event) override {
            if (imgui && imgui->graphics() && imgui->graphics()->onInpuKeyEvent(event)) {
                return true;
            }

			switch (event.key) {
				case KeyboardKey::K_E:
					event.state == InputEventState::IES_RELEASE ? wasd.y -= 0.5f : wasd.y += 0.5f;
					break;
				case KeyboardKey::K_Q:
					event.state == InputEventState::IES_RELEASE ? wasd.y += 0.5f : wasd.y -= 0.5f;
					break;
				case KeyboardKey::K_W:
					event.state == InputEventState::IES_RELEASE ? wasd.z -= 1.0f : wasd.z += 1.0f;
					break;
				case KeyboardKey::K_S:
					event.state == InputEventState::IES_RELEASE ? wasd.z += 1.0f : wasd.z -= 1.0f;
					break;
				case KeyboardKey::K_A:
					event.state == InputEventState::IES_RELEASE ? wasd.x += 1.0f : wasd.x -= 1.0f;
					break;
				case KeyboardKey::K_D:
					event.state == InputEventState::IES_RELEASE ? wasd.x -= 1.0f : wasd.x += 1.0f;
					break;
				case KeyboardKey::K_ESCAPE:
					if (event.state != InputEventState::IES_RELEASE) break;
					Engine::getInstance().getModule<Device>()->leaveMainLoop();
					break;
				case KeyboardKey::K_ENTER:
					if (Engine::getInstance().getModule<Input>()->isAltPressed()) {
						if (event.state != InputEventState::IES_RELEASE) break;
						Engine::getInstance().getModule<Device>()->setFullscreen(!Engine::getInstance().getModule<Device>()->isFullscreen());
					}
					break;
				case KeyboardKey::K_TAB:
					if (event.state != InputEventState::IES_RELEASE) break;
					renderBounds = !renderBounds;
					break;
				case KeyboardKey::K_MINUS:
					if (event.state == InputEventState::IES_RELEASE) {
						Engine::getInstance().setGameTimeMultiply(std::max(0.0f, Engine::getInstance().getGameTimeMultiply() - 0.1f));
					}
					break;
				case KeyboardKey::K_EQUAL:
					if (event.state == InputEventState::IES_RELEASE) {
						if (Engine::getInstance().getModule<Input>()->isShiftPressed()) {
							Engine::getInstance().setGameTimeMultiply(Engine::getInstance().getGameTimeMultiply() + 0.1f);
						} else {
							Engine::getInstance().setGameTimeMultiply(1.0f);
						}
					}
					break;
				default:
					break;
			}

			return false;
		}

		bool onInpuCharEvent(const uint16_t code) override { return false; };

		void initCamera() {
			const uint64_t wh = Engine::getInstance().getModule<Graphics>()->getRenderer()->getWH();
			const uint32_t width = static_cast<uint32_t>(wh >> 0);
			const uint32_t height = static_cast<uint32_t>(wh >> 32);

			targetCameraRotation = glm::vec3(-engine::math_constants::pi / 3.0f, 0.0f, 0.0f);

			camera = new Camera(width, height);
			camera->enableFrustum();

			camera->makeProjection(engine::math_constants::pi / 4.0f, static_cast<float>(width) / static_cast<float>(height), 1.0f, 5000.0f);
			//camera->makeOrtho(-float(width) * 0.5f, float(width) * 0.5f, -float(height) * 0.5f, float(height) * 0.5f, 1.0f, 1000.0f);
			camera->setRotation(targetCameraRotation);
			camera->setPosition(glm::vec3(0.0f, -500.0f, 300.0f));

			camera2 = new Camera(width, height);
			camera2->enableFrustum();
			camera2->makeOrtho(-float(width) * 0.5f, float(width) * 0.5f, -float(height) * 0.5f, float(height) * 0.5f, 1.0f, 1000.0f);
			camera2->setPosition(glm::vec3(0.0f, 0.0f, 200.0f));

			//clearValues[0].color = { 0.5f, 0.78f, 0.99f, 1.0f };
			clearValues[0].color = { 0.25f, 0.25f, 0.25f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			//////////////////////////////////
			const glm::vec2 nearFar(1.0f, 5500.0f);
			shadowMap = new CascadeShadowMap(SHADOWMAP_DIM, SHADOW_MAP_CASCADE_COUNT, nearFar, 250.0f, 2500.0f);
			shadowMap->setLamdas(1.0f, 1.0f, 1.0f);
			shadowMap->setLightPosition(lightPos);
		}

		void create() {
			using namespace vulkan;
			using namespace gltf;

			rootNode = new H_Node();
			uiNode = new H_Node();

			auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
			auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();
			AssetManager* assm = Engine::getInstance().getModule<AssetManager>();

			std::vector<engine::ProgramStageInfo> psi0;
			psi0.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh.vsh.spv");
			psi0.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh.psh.spv");
			VulkanGpuProgram* program_gltf0 = gpuProgramManager->getProgram(psi0);

			std::vector<engine::ProgramStageInfo> psi1;
			psi1.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_skin.vsh.spv");
			psi1.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh_skin.psh.spv");
			VulkanGpuProgram* program_gltf1 = gpuProgramManager->getProgram(psi1);

			std::vector<engine::ProgramStageInfo> psi2;
			psi2.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_stroke.vsh.spv");
			psi2.emplace_back(ProgramStage::GEOMETRY, "resources/shaders/mesh_stroke.gsh.spv");
			psi2.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh_stroke.psh.spv");
			VulkanGpuProgram* program_gltf2 = gpuProgramManager->getProgram(psi2);

			std::vector<engine::ProgramStageInfo> psi3;
			psi3.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_skin_stroke.vsh.spv");
			psi3.emplace_back(ProgramStage::GEOMETRY, "resources/shaders/mesh_stroke.gsh.spv");
			psi3.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh_skin_stroke.psh.spv");
			VulkanGpuProgram* program_gltf3 = gpuProgramManager->getProgram(psi3);

			std::vector<engine::ProgramStageInfo> psi4;
			psi4.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_instance.vsh.spv");
			psi4.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh_instance.psh.spv");
			VulkanGpuProgram* program_gltf4 = gpuProgramManager->getProgram(psi4);

			program_mesh = program_gltf0;
			program_mesh_skin = program_gltf1;
			program_mesh_skin_shadow = CascadeShadowMap::getShadowProgram<MeshSkinnedShadow>();
			program_mesh_shadow = CascadeShadowMap::getShadowProgram<MeshStaticShadow>();
			program_mesh_instance_shadow = CascadeShadowMap::getShadowProgram<MeshStaticInstanceShadow>();
			program_mesh_with_stroke = program_gltf2;
			program_mesh_skin_with_stroke = program_gltf3;
			program_mesh_instance = program_gltf4;
			VulkanGpuProgram* shadowPlainProgram = const_cast<VulkanGpuProgram*>(CascadeShadowMap::getSpecialPipeline(ShadowMapSpecialPipelines::SH_PIPEINE_PLAIN)->program);

			auto assignGPUParams = [](VulkanGpuProgram* program) {
				glm::vec3 lightDir = as_normalized(-lightPos);
				program->setValueByName("lightDirection", &lightDir, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
				program->setValueByName("lightMinMax", &lightMinMax, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
				program->setValueByName("lightColor", &lightColor, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
				program->setValueByName("saturation", &saturation, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
				shadowMap->registerProgramAsReciever(program);
			};

			assignGPUParams(program_mesh);
			assignGPUParams(program_mesh_skin);
			assignGPUParams(shadowPlainProgram);
			assignGPUParams(program_mesh_with_stroke);
			assignGPUParams(program_mesh_skin_with_stroke);
			assignGPUParams(program_mesh_instance);

			TextureLoadingParams tex_params;
			tex_params.files = { 
				"resources/assets/models/chaman/textures/Ti-Pche_Mat_baseColor.png",
				"resources/assets/models/chaman/textures/Ti-Pche_Mat_normal.png",
				"resources/assets/models/chaman/textures/Ti-Pche_Mat_metallicRoughness.png"
			};
			tex_params.flags->async = 1;
			tex_params.flags->use_cache = 1;
			//tex_params.formatType = engine::TextureFormatType::SRGB;
			tex_params.imageViewTypeForce = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			auto texture_zombi = assm->loadAsset<vulkan::VulkanTexture*>(tex_params);

			TextureLoadingParams tex_params2;
			tex_params2.files = { 
				"resources/assets/models/warcraft3/textures/Armor_2.001_baseColor.png",
				"resources/assets/models/warcraft3/textures/Armor_2.001_normal.png",
				"resources/assets/models/warcraft3/textures/Armor_2_metallicRoughness.png"
			};
			tex_params2.flags->async = 1;
			tex_params2.flags->use_cache = 1;

			//tex_params2.formatType = engine::TextureFormatType::SRGB;
			tex_params2.imageViewTypeForce = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			auto texture_v = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);
			tex_params2.files = { 
				"resources/assets/models/warcraft3/textures/body_baseColor.png",
				"resources/assets/models/warcraft3/textures/body_normal.png",
				"resources/assets/models/warcraft3/textures/body_metallicRoughness.png"
			};
			auto texture_v2 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);
			tex_params2.files = { 
				"resources/assets/models/warcraft3/textures/Metal_baseColor.png",
				"resources/assets/models/warcraft3/textures/Metal_normal.png",
				"resources/assets/models/warcraft3/textures/Metal_metallicRoughness.png"
			};
			auto texture_v3 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);

			TextureLoadingParams tex_params3;
			tex_params3.flags->async = 1;
			tex_params3.flags->use_cache = 1;
			//tex_params3.formatType = engine::TextureFormatType::SRGB;
			tex_params3.imageViewTypeForce = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			tex_params3.files = { 
				"resources/assets/models/tree1/textures/tree2_baseColor.png",
				"resources/assets/models/tree1/textures/tree2_normal.png",
				"resources/assets/models/tree1/textures/tree2_metallicRoughness.png"
			};
			auto texture_t = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/tree1/textures/branches_baseColor.png",
				"resources/assets/models/tree1/textures/branches_normal.png"
			};
			auto texture_t2 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			//tex_params3.files = { 
			//	"resources/assets/models/pineTree/textures/Leavs_baseColor.png",
			//	"resources/assets/models/pineTree/textures/Trank_normal.png"
			//};
			//auto texture_t3 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			//tex_params3.files = { 
			//	"resources/assets/models/pineTree/textures/Trank_baseColor.png",
			//	"resources/assets/models/pineTree/textures/Trank_normal.png"
			//};
			//auto texture_t4 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = {
				"resources/assets/models/portal/textures/portal_diffuse.png",
				"resources/assets/models/portal/textures/portal_normal.png",
				"resources/assets/models/portal/textures/portal_specularGlossiness.png"
			};
			auto texture_t3 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/vikingHut/textures/texture1.jpg",
				"resources/assets/models/vikingHut/textures/Main_Material2_normal.jpg",
				"resources/assets/models/vikingHut/textures/Main_Material2_metallicRoughness.png"
			};
			
			auto texture_t5 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/grass/textures/new/grass75.png",
				"resources/assets/models/grass/textures/new/grass791.png",
				"resources/assets/models/grass/textures/new/grass79.png",
				"resources/assets/models/grass/textures/flowers16.png",
				"resources/assets/models/grass/textures/flowers26.png"
			};
			auto texture_t6 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/windmill/textures/standardSurface1_baseColor.png",
				"resources/assets/models/windmill/textures/standardSurface1_normal.png",
				"resources/assets/models/windmill/textures/standardSurface1_metallicRoughness.png"
			};
			auto texture_t7 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			TextureLoadingParams tex_params_logo;
			tex_params_logo.files = { "resources/assets/textures/sun2.jpg" };
			tex_params_logo.flags->async = 1;
			tex_params_logo.flags->use_cache = 1;
			texture_1 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_logo);

			meshesGraphicsBuffer = new MeshGraphicsDataBuffer(10 * 1024 * 1024, 10 * 1024 * 1024); // or create with default constructor for unique buffer for mesh

			MeshLoadingParams mesh_params;
			mesh_params.file = "resources/assets/models/chaman/scene.gltf";
			mesh_params.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params.latency = 3;
			mesh_params.flags->async = 1;
			mesh_params.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params2;
			mesh_params2.file = "resources/assets/models/warcraft3/scene.gltf";
			mesh_params2.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params2.latency = 3;
			mesh_params2.flags->async = 1;
			mesh_params2.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params3;
			mesh_params3.file = "resources/assets/models/tree1/scene.gltf";
			mesh_params3.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::TEXCOORD_0);
			mesh_params3.latency = 1;
			mesh_params3.flags->async = 1;
			mesh_params3.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params4;
			//mesh_params4.file = "resources/assets/models/pineTree/scene.gltf";
			mesh_params4.file = "resources/assets/models/portal/scene.gltf";
			mesh_params4.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::TEXCOORD_0);
			mesh_params4.latency = 1;
			mesh_params4.flags->async = 1;
			mesh_params4.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params5;
			mesh_params5.file = "resources/assets/models/vikingHut/scene.gltf";
			mesh_params5.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::TEXCOORD_0);
			mesh_params5.latency = 1;
			mesh_params5.flags->async = 1;
			mesh_params5.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params6;
			mesh_params6.file = "resources/assets/models/windmill/scene.gltf";
			mesh_params6.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::TEXCOORD_0);
			mesh_params6.latency = 3;
			mesh_params6.flags->async = 1;
			mesh_params6.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params_grass;
			mesh_params_grass.file = "resources/assets/models/grass/scene.gltf";
			mesh_params_grass.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TEXCOORD_0);
			mesh_params_grass.latency = 1;
			mesh_params_grass.flags->async = 1;
			mesh_params_grass.graphicsBuffer = meshesGraphicsBuffer;

			///////
			TextureLoadingParams tex_params_forest_tree;
			tex_params_forest_tree.flags->async = 1;
			tex_params_forest_tree.flags->use_cache = 1;
			tex_params_forest_tree.imageViewTypeForce = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			tex_params_forest_tree.files = {
				"resources/assets/models/pine/textures/Branches_lp_baseColor.png",
				"resources/assets/models/pine/textures/Branches_lp_normal.png",
				"resources/assets/models/pine/textures/Branches_lp_metallicRoughness2.png"
			};
			
			auto texture_forest_tree1 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_forest_tree);

			tex_params_forest_tree.files = {
				"resources/assets/models/pine/textures/Tree_bark_baseColor.jpeg",
				"resources/assets/models/pine/textures/Tree_bark_normal.png",
				"resources/assets/models/pine/textures/Tree_bark_metallicRoughness.png"

			};
			auto texture_forest_tree2 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_forest_tree);

			MeshLoadingParams mesh_params_forestTree;
			mesh_params_forestTree.file = "resources/assets/models/pine/tree.gltf";
			mesh_params_forestTree.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::TEXCOORD_0);
			mesh_params_forestTree.latency = 1;
			mesh_params_forestTree.flags->async = 1;
			mesh_params_forestTree.graphicsBuffer = meshesGraphicsBuffer;
			////////

			mesh = new NodeRenderer<Mesh>();
			mesh2 = new NodeRenderer<Mesh>();
			mesh3 = new NodeRenderer<Mesh>();
			mesh4 = new NodeRenderer<Mesh>();
			mesh5 = new NodeRenderer<Mesh>();
			mesh6 = new NodeRenderer<Mesh>();
			mesh7 = new NodeRenderer<Mesh>();
			//grassMesh = new NodeRenderer<Mesh>();
			grassMesh2 = new NodeRenderer<GrassRenderer>();
			forest = new NodeRenderer<InstanceMeshRenderer>();
			
			constexpr uint16_t unitsCount = 100;
			testMehsesVec.reserve(unitsCount);
			for (size_t i = 0; i < unitsCount; ++i) {
				meshUpdateSystem.registerObject(testMehsesVec.emplace_back(new NodeRenderer<Mesh>()));
			}

			meshUpdateSystem.registerObject(mesh);
			meshUpdateSystem.registerObject(mesh2);
			meshUpdateSystem.registerObject(mesh3);
			meshUpdateSystem.registerObject(mesh4);
			meshUpdateSystem.registerObject(mesh5);
			meshUpdateSystem.registerObject(mesh6);
			meshUpdateSystem.registerObject(mesh7);

			grassUpdateSystem.registerObject(grassMesh2);
			instanceMeshUpdateSystem.registerObject(forest);

			///////////
			{
				skyBox = new NodeRenderer<SkyBoxRenderer>();
				skyBoxUpdateSystem.registerObject(skyBox);

				SkyBoxRenderer* skyboxRenderer = new SkyBoxRenderer();
				skyboxRenderer->createRenderData();

				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(1400.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				//node->value().setBoundingVolume(BoundingVolume::make<SphereVolume>(glm::vec3(0.0f, 1.45f, 0.0f), 1.8f));
				rootNode->addChild(node);

				skyBox->setGraphics(skyboxRenderer);
				(*node)->setRenderObject(skyBox); // (*node)-> == node->value().
			}
			
			assm->loadAsset<Mesh*>(mesh_params, [texture_zombi, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh_skin_with_stroke);
				asset->setParamByName("u_texture", texture_zombi, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.5f, true);

				animTree = new MeshAnimationTree(0.0f, asset->getNodesCount(), asset->getSkeleton()->getLatency());
				animTree->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[2], 1.0f, asset->getSkeleton()->getLatency()));
				animTree->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[1], 0.0f, asset->getSkeleton()->getLatency()));

                auto&& animationManager = Engine::getInstance().getModule<Graphics>()->getAnimationManager();
                animationManager->registerAnimation(animTree);
                animationManager->addTarget(animTree, asset->getSkeleton().get());

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				//////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(25.0f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.45f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(-100.0f, -0.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<SphereVolume>(glm::vec3(0.0f, 1.45f, 0.0f), 1.8f));
				rootNode->addChild(node);

				mesh->setGraphics(asset);
				(*node)->setRenderObject(mesh);

				shadowCastNodes.push_back(node);
				});

			assm->loadAsset<Mesh*>(mesh_params, [texture_zombi](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh_skin);
				asset->setParamByName("u_texture", texture_zombi, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.5f, true);

				asset->setSkeleton(mesh->graphics()->getSkeleton());

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(20.0f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, -0.45f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(100.0f, 190.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<SphereVolume>(glm::vec3(0.0f, 1.45f, 0.0f), 1.8f));
				rootNode->addChild(node);

				mesh2->setGraphics(asset);
				(*node)->setRenderObject(mesh2);

				shadowCastNodes.push_back(node);
				});

			for (auto&& meshObj : testMehsesVec) {
				assm->loadAsset<Mesh*>(mesh_params, [meshObj, texture_zombi](Mesh* asset, const AssetLoadingResult result) {
					asset->setProgram(program_mesh_skin);
					asset->setParamByName("u_texture", texture_zombi, false);
					asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
					asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
					asset->setParamByName("lighting", 0.5f, true);

					asset->setSkeleton(mesh->graphics()->getSkeleton());

					asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
						renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
					});

					////////////////////
					glm::mat4 wtr(1.0f);
					scaleMatrix(wtr, glm::vec3(engine::random(18.0f, 22.0f)));
					rotateMatrix_xyz(wtr, glm::vec3(1.57f, engine::random(-3.1415f, 3.1415f), 0.0f));
					translateMatrixTo(wtr, glm::vec3(engine::random(-1024.0f, 1024.0f), engine::random(-1024.0f, 1024.0f), 0.0f));

					H_Node* node = new H_Node();
					node->value().setLocalMatrix(wtr);
					node->value().setBoundingVolume(BoundingVolume::make<SphereVolume>(glm::vec3(0.0f, 1.45f, 0.0f), 1.8f));
					rootNode->addChild(node);

					meshObj->setGraphics(asset);
					(*node)->setRenderObject(meshObj);

					shadowCastNodes.push_back(node);
					});
			}

			assm->loadAsset<Mesh*>(mesh_params2, [texture_v, texture_v2, texture_v3](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh_skin);
				asset->setParamByName("u_texture", texture_v, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.5f, true);

				asset->getRenderDataAt(3)->setParamByName("u_texture", texture_v3, false);
				asset->getRenderDataAt(7)->setParamByName("u_texture", texture_v2, false); // eye
				asset->getRenderDataAt(8)->setParamByName("u_texture", texture_v2, false); // head
				asset->getRenderDataAt(9)->setParamByName("u_texture", texture_v3, false); // helm
				asset->getRenderDataAt(11)->setParamByName("u_texture", texture_v3, false);
				asset->getRenderDataAt(13)->setParamByName("u_texture", texture_v3, false);

				//asset->changeRenderState([](vulkan::VulkanRenderState& renderState) { // for stroke
				//	renderState.rasterizationState.cullMode = vulkan::CULL_MODE_NONE;
				//});

				animTree2 = new MeshAnimationTree(0.0f, asset->getNodesCount(), asset->getSkeleton()->getLatency());

				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[1], 1.0f, asset->getSkeleton()->getLatency(), 1.0f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[2], 0.0f, asset->getSkeleton()->getLatency(), 1.0f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[11], 0.0f, asset->getSkeleton()->getLatency(), 1.25f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[10], 0.0f, asset->getSkeleton()->getLatency(), 1.0f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[0], 0.0f, asset->getSkeleton()->getLatency(), 1.4f));

                auto&& animationManager = Engine::getInstance().getModule<Graphics>()->getAnimationManager();
                animationManager->registerAnimation(animTree2);
                animationManager->addTarget(animTree2, asset->getSkeleton().get());

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(30.0f));
				directMatrix_yz(wtr, 0.0f, 1.0f);
				translateMatrixTo(wtr, glm::vec3(-100.0f, 210.0f, 0.0f));

				H_Node* node = new H_Node();
				(*node)->setLocalMatrix(wtr);
				(*node)->setBoundingVolume(BoundingVolume::make<SphereVolume>((asset->getMinCorner() + asset->getMaxCorner()) * 0.5f, 1.0f));
				//(*node)->setBoundingVolume(BoundingVolume::make<CubeVolume>(asset->getMinCorner(), asset->getMaxCorner()));
				(*node)->setRenderObject(mesh3);
				rootNode->addChild(node);

				mesh3->setGraphics(asset);
				
				shadowCastNodes.push_back(node);
				});

			assm->loadAsset<Mesh*>(mesh_params3, [texture_t, texture_t2, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh);
				asset->setParamByName("u_texture", texture_t, false);
				asset->getRenderDataAt(1)->setParamByName("u_texture", texture_t2, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.0f, true);

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(0.65f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.0f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(-150.0f, -170.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(asset->getMinCorner(), asset->getMaxCorner()));
				rootNode->addChild(node);

				mesh4->setGraphics(asset);
				(*node)->setRenderObject(mesh4);

				shadowCastNodes.push_back(node);
				});

			assm->loadAsset<Mesh*>(mesh_params4, [texture_t3, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh);
				asset->setParamByName("u_texture", texture_t3, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.0f, true);

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(0.125f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.0f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(570.0f, -250.0f, 10.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(asset->getMinCorner(), asset->getMaxCorner()));
				rootNode->addChild(node);

				mesh5->setGraphics(asset);
				(*node)->setRenderObject(mesh5);

				shadowCastNodes.push_back(node);
				});

			assm->loadAsset<Mesh*>(mesh_params5, [texture_t5, texture_t6, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh);
				asset->setParamByName("u_texture", texture_t5, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.0f, true);

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(35.0f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, 1.1f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(225.0f, 285.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(asset->getMinCorner(), asset->getMaxCorner()));
				rootNode->addChild(node);

				mesh6->setGraphics(asset);
				(*node)->setRenderObject(mesh6);

				shadowCastNodes.push_back(node);
				});

			assm->loadAsset<Mesh*>(mesh_params6, [texture_t7, texture_t6, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh);
				asset->setParamByName("u_texture", texture_t7, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.0f, true);

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				animTreeWindMill = new MeshAnimationTree(&asset->getMeshData()->animations[0],1.0f, asset->getSkeleton()->getLatency());

                auto&& animationManager = Engine::getInstance().getModule<Graphics>()->getAnimationManager();
                animationManager->registerAnimation(animTreeWindMill);
                animationManager->addTarget(animTreeWindMill, asset->getSkeleton().get());

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(8000.0f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, -1.15f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(-150.0f, 375.0f, -10.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(asset->getMinCorner(), asset->getMaxCorner() + glm::vec3(0.0f, 0.01f, 0.0f)));
				rootNode->addChild(node);

				mesh7->setGraphics(asset);
				(*node)->setRenderObject(mesh7);

				shadowCastNodes.push_back(node);
				});

			TextureData img("resources/assets/textures/t3.jpg");
			GrassRenderer* grenderer = new GrassRenderer(img);

			///////////////////////////////////////////////////////////////////////////
			std::vector<glm::mat4> instanceTransforms(100);
			{
				const uint32_t* data = reinterpret_cast<const uint32_t*>(img.data());

				const auto w = img.width();
				const auto h = img.height();
				const float cx = 2048.0f / w;
				const float cy = 2048.0f / h;

				for (size_t i = 0; i < instanceTransforms.size(); ++i) {
					uint16_t x = engine::random(0, w);
					uint16_t y = engine::random(0, h);
					uint32_t v = data[y * w + x];
					
					while ((v & 0x000000f0) < 240) {
						x = engine::random(0, w);
						y = engine::random(0, h);
						v = data[y * w + x];
					}

					const float l = vec_length(glm::vec2(-1024.0f + cx * x, 1024.0f - cy * y) - glm::vec2(570.0f, -250.0f));
					const float r = 150.0f;

					if (l < r) {
						glm::vec2 direction = as_normalized(glm::vec2(570.0f, -250.0f) - glm::vec2(-1024.0f + cx * x, 1024.0f - cy * y));
						x = std::clamp((570.0f + r * direction.x) + 1024.0f / cx, 0.0f, w - 1.0f);
						y = std::clamp((-250.0f + r * direction.y) - 1024.0f / -cy, 0.0f, h - 1.0f);

						uint32_t v = data[y * w + x];
						if ((v & 0x000000f0) < 240) {
							--i;
							continue;
						}
					}

					const float scale_xyz = (engine::random(300.0f, 800.0f));

					glm::mat4 wtr(scale_xyz);
					wtr[3][3] = 1.0f;
					rotateMatrix_xyz(wtr, glm::vec3(engine::random(-0.1f, 0.1f), engine::random(-0.1f, 0.1f), engine::random(-3.1415926, 3.1415926)));
					translateMatrixTo(wtr, glm::vec3(-1024.0f + cx * x, 1024.0f - cy * y, 0.0f));
					instanceTransforms[i] = std::move(wtr);
				}
			}

			InstanceMeshRenderer* forestRenderer = new InstanceMeshRenderer(std::move(instanceTransforms), nullptr, std::vector<vulkan::VulkanGpuProgram*>{ program_mesh_instance, program_mesh_instance_shadow });
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			TextureLoadingParams tex_params_floor_mask;
			tex_params_floor_mask.flags->async = 0;
			tex_params_floor_mask.flags->use_cache = 1;
			tex_params_floor_mask.texData = &img;
			tex_params_floor_mask.files = { "grass_mask" };
			texture_floor_mask = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_floor_mask);

			TextureLoadingParams tex_params_floor_normal;
			tex_params_floor_normal.flags->async = 1;
			tex_params_floor_normal.flags->use_cache = 1;
			tex_params_floor_normal.files = {
				"resources/assets/textures/normal1.jpg"
			};

			texture_floor_normal = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_floor_normal);

			assm->loadAsset<Mesh*>(mesh_params_grass, [texture_t6, grenderer, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(grass_default);
				asset->setParamByName("u_texture", texture_t6, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				/////////////////////
				//grassMesh->setGraphics(asset);
				glm::mat4 wtr(1.0f);
				//scaleMatrix(wtr, glm::vec3(10000.0f));
				//rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.0f, 0.0f));
				//translateMatrixTo(wtr, glm::vec3(200.0f, -100.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(glm::vec3(-1024.0f, -1024.0f, 0.0f), glm::vec3(1024.0f, 1024.0f, 40.0f)));
				rootNode->addChild(node);

				grenderer->setMesh(asset);
				grassMesh2->setGraphics(grenderer);
				(*node)->setRenderObject(grassMesh2);
				});

			assm->loadAsset<Mesh*>(mesh_params_forestTree, [texture_forest_tree1, texture_forest_tree2, forestRenderer, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_mesh_instance);
				asset->setParamByName("u_texture", texture_forest_tree2, false);
				asset->getRenderDataAt(1)->setParamByName("u_texture", texture_forest_tree1, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				asset->setParamByName("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
				asset->setParamByName("lighting", 0.25f, true);

				asset->changeRenderState([](vulkan::VulkanRenderState& renderState) {
					renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
				});

				////////////////////
				glm::mat4 wtr(1.0f);
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.0f, 0.0f));
				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(glm::vec3(-1024.0f, 0.0f, -1024.0f), glm::vec3(1024.0f, 200.0f, 1024.0f)));
				rootNode->addChild(node);

				forestRenderer->setGraphics(asset);
				forest->setGraphics(forestRenderer);
				(*node)->setRenderObject(forest);

				shadowCastNodes.push_back(node);
			});

            imgui = new NodeRenderer<ImguiGraphics>();
            {
                H_Node* node = new H_Node();
                uiNode->addChild(node);

                imgui->setGraphics(new ImguiGraphics());
                (*node)->setRenderObject(imgui);

                imgui->getRenderDescriptor()->order = 100;
            }

			plainTest = new NodeRenderer<Plain>();
			plainUpdateSystem.registerObject(plainTest);
			{
				glm::mat4 wtr(1.0f);
				//scaleMatrix(wtr, glm::vec3(1.0f));
				//rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.0f, 0.0f));
				//translateMatrixTo(wtr, glm::vec3(200.0f, -300.0f, 50.0f));
				translateMatrixTo(wtr, glm::vec3(0.0f, 0.0f, -1.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				uiNode->addChild(node);

				//plainTest->setGraphics(new Plain(glm::vec2(100.0f, 100.0f)));

				plainTest->setGraphics(new Plain(
				std::shared_ptr<TextureFrame>(new TextureFrame(
					{
						0.0f, 0.0f,
						100.0f, 0.0f,
						0.0f, 100.0f,
						100.0f, 100.0f
					},
					{
						0.0f, 1.0f,
						1.0f, 1.0f,
						0.0f, 0.0f,
						1.0f, 0.0f
					},
					{
						0, 1, 2, 2, 1, 3
					}
				)), nullptr));

				(*node)->setRenderObject(plainTest);

				plainTest->graphics()->setParamByName("u_texture", texture_floor_normal, false);
				plainTest->getRenderDescriptor()->order = 10;
			}

			TextureLoadingParams tex_params_floorArray;
			/*tex_params_floorArray.files = {
				"resources/assets/textures/swamp5.jpg",
				"resources/assets/textures/ground133.jpg"
			};*/
			tex_params_floorArray.files = {
				"resources/assets/textures/512x512/grass3.png",
				"resources/assets/textures/512x512/ground.jpg"
			};
			tex_params_floorArray.flags->async = 1;
			tex_params_floorArray.flags->use_cache = 1;
			//tex_params_floorArray.formatType = engine::TextureFormatType::SRGB;
			texture_array_test = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_floorArray, [](vulkan::VulkanTexture* asset, const AssetLoadingResult result) {
				auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
				texture_array_test->setSampler(
					renderer->getSampler(
						VK_FILTER_LINEAR,
						VK_FILTER_LINEAR,
						VK_SAMPLER_MIPMAP_MODE_LINEAR,
						VK_SAMPLER_ADDRESS_MODE_REPEAT,
						VK_SAMPLER_ADDRESS_MODE_REPEAT,
						VK_SAMPLER_ADDRESS_MODE_REPEAT,
						VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
					));
				});

			/////// freeType test
			FontLoadingParams font_loading_params("resources/assets/fonts/Roboto/Roboto-Regular.ttf");
			Font* f = assm->loadAsset<Font*>(font_loading_params);

			//FontRenderer fr(256, 256, 100);
			//fr.render(f, 18, "abcdefghijklmnopqrstuv", 1, 0, 0xffffffff, 2, 0);
			//fr.render(f, 18, "wxyzABCDEFGHIJKLMN", 1, 20, 0xffffffff, 2, 0);
			//fr.render(f, 18, "OPQRSTUVWXYZ", 1, 40, 0xffffffff, 2, 0);
			//fr.render(f, 18, "0123456789-+*/=", 1, 60, 0xffffffff, 2, 0);
			//fr.render(f, 18, "&%#@!?<>,.()[];:@$^~", 1, 80, 0xffffffff, 2, 0);
			//
			//texture_text = fr.createFontTexture();
			//texture_text->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);

			bitmapFont = new BitmapFont(f, 384, 256, {BitmapFontType::SDF, 14, 4, -11, 5}, 0);
            bitmapFont->addSymbols("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-+*/=&%#@!?<>,.()[];:@$^~_");
//            bitmapFont = new BitmapFont(f, 256, 256, {BitmapFontType::Usual, 16, 2, 2, 5}, 0);
//			bitmapFont->addSymbols("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-+*/=&%#@!?<>,.()[];:@$^~_", 2, 0, 0xccccccaa, 0x000000aa, 2.0f, 2, 2);


            //bitmapFont->addSymbols("abc", 2, 0, 0xccccccaa, 0x000000aa, 0.0f, 2, 2);

            //bitmapFont.addSymbols("wxyzABCDEFGHIJKLMN", 2, 20, 0xffffffff, 0x000000ff, 1.0f, 2, 0);
			//bitmapFont.addSymbols("OPQRSTUVWXYZ", 2, 40, 0xffffffff, 0x000000ff, 1.0f, 2, 0);
			//bitmapFont.addSymbols("0123456789-+*/=", 2, 60, 0xffffffff, 0x000000ff, 1.0f, 2, 0);
			//bitmapFont.addSymbols("&%#@!?<>,.()[];:@$^~", 2, 80, 0xffffffff, 0x000000ff, 1.0f, 2, 0);

			bitmapFont->complete();

			texture_text = bitmapFont->grabTexture();
			texture_text->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);

			auto texFrame = std::shared_ptr<TextureFrame>(new TextureFrame(
				{
					0.0f, 0.0f,
					256, 0.0f,
					0.0f, 256,
					256, 256
				},
				{
					0.0f, 1.0f,
					1.0f, 1.0f,
					0.0f, 0.0f,
					1.0f, 0.0f
				},
				{
					0, 1, 2, 2, 1, 3
				}
			));

			plainTest->graphics()->setFrame(texFrame);
			plainTest->graphics()->setParamByName("u_texture", texture_text, false);
			plainTest->graphics()->pipelineAttributesChanged();

            plainTest->graphics()->changeRenderState([](vulkan::VulkanRenderState& renderState) {
                renderState.blendMode = vulkan::CommonBlendModes::blend_alpha;
                renderState.depthState.depthWriteEnabled = false;
                renderState.depthState.depthTestEnabled = false;
            });

			plainTest->getNode()->setBoundingVolume(BoundingVolume::make<CubeVolume>(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(256, 256, 1.0f)));

            {
                std::vector<engine::ProgramStageInfo> psi;
                psi.emplace_back(ProgramStage::VERTEX, "resources/shaders/texture.vsh.spv");
                psi.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/textureSDF.psh.spv");
                vulkan::VulkanGpuProgram *program = gpuProgramManager->getProgram(psi);
                plainTest->setProgram(program);
            }

			//plainTest->graphics()->setFrame(bitmapFont->createFrame("hello world!"));
			//plainTest->getNode()->setBoundingVolume(nullptr);
			//const float wf = frame->_vtx[2] - frame->_vtx[0];
			//const float hf = frame->_vtx[5] - frame->_vtx[1];
			//plainTest->getNode()->setBoundingVolume(BoundingVolume::make<CubeVolume>(glm::vec3(0.0f, 0.0f, -0.1f), glm::vec3(wf, hf, 0.1f)));

			/*FontLibrary lib;
			Font font("resources/assets/fonts/Roboto/Roboto-Regular.ttf");
			Font font2("resources/assets/fonts/Roboto/Roboto-Italic.ttf");

			FontRenderer ft_render(lib, font, 18);
			FontRenderer ft_render2(lib, font2, 18);

			const uint16_t w = 256;
			const uint16_t h = 256;

			unsigned char* image2 = new unsigned char[w * h * 4];
			memset(image2, 100, sizeof(unsigned char) * w * h * 4);

			ft_render.render("abcdefghijklmnopqrstuv", &image2[0], w, h, 0, 0, 0x000000ff, 2, 0);
			ft_render.render("wxyzABCDEFGHIJKLMN", &image2[0], w, h, 0, 20, 0x000000ff, 2, 0);
			ft_render.render("OPQRSTUVWXYZabv", &image2[0], w, h, 0, 40, 0x000000ff, 2, 0);*/
			//ft_render.render("0123456789-+*/=", &image2[0], w, h, 0, 60, 0x000000ff, 2, 0);
			//ft_render.render("&%#@!?<>,.()[];:@$^~", &image2[0], w, h, 0, 80, 0x000000ff, 2, 0);

			//ft_render2.render("abcdefghijklmnopqrstuv", &image2[0], w, h, 0, 100, 0xffffffff, 2, 0);
			//ft_render2.render("wxyz", &image2[0], w, h, 0, 120, 0xffffffff, 2, 0);

			/*texture_text = new vulkan::VulkanTexture(renderer, w, h);

			texture_text->setSampler(renderer->getSampler(
				VK_FILTER_LINEAR,
				VK_FILTER_LINEAR,
				VK_SAMPLER_MIPMAP_MODE_NEAREST,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
			));

			texture_text->create(&image2[0], VK_FORMAT_R8G8B8A8_UNORM, 32, false, false);
			texture_text->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);

			delete image2;*/

			///////// flowTask example
			static int x = 1, y = 2;
			auto task = engine::makeFlowTaskResult(engine::TaskType::COMMON, [](const engine::CancellationToken&) {
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
				x = 10;
				return 1;
				}
			);

			auto task2 = engine::makeFlowTaskResult(engine::TaskType::COMMON, [](const engine::CancellationToken&) {
				std::this_thread::sleep_for(std::chrono::milliseconds(400));
				y = 12;
				return 1;
				}
			);

			auto depTask = engine::makeFlowTaskResult(engine::TaskType::COMMON, [](const engine::CancellationToken&) {
				engine::log("1. sum %d + %d = %d", x, y, x + y);
				return 1;
				}
			);

			auto depTask2 = engine::makeTaskResult(
				engine::TaskType::COMMON, [](const engine::CancellationToken&) {
					engine::log("2. sum %d + %d = %d", x, y, x + y);
					return 1;
				});

			//engine::addTaskToFlow(task, depTask);
			//engine::addTaskToFlow(task2, depTask);

			//engine::Engine::getInstance().getModule<engine::ThreadPool>()->enqueue(0, task);
			//engine::Engine::getInstance().getModule<engine::ThreadPool>()->enqueue(0, task2);

			//std::this_thread::sleep_for(std::chrono::milliseconds(10));

			//engine::addTaskToFlow(task, depTask2);
			//engine::addTaskToFlow(task2, depTask2);

			engine::BitMask32 mask;
			mask.setBit(0, 1);
			mask.setBit(1, 1);

			mask.setBit(33, 1);
			mask.setBit(34, 1);

			bool is1 = mask.checkBit(34);
			bool is2 = mask.checkBit(35);

			mask.nullify();

			auto&& bus = engine::Engine::getInstance().getModule<engine::Bus>();

			struct TestBusEvent {
				float x = 0.0f, y = 0.0f;
			};

			engine::EventSubscriber<TestBusEvent> subscriber([](const TestBusEvent& evt)->bool {
				const float x = evt.x;
				const float y = evt.y;
				return true;
			});

			bus->addSubscriber<TestBusEvent>(subscriber);

			auto&& subscriber2 = bus->addSubscriber<TestBusEvent>([](const auto& evt)->bool {
				const float x = evt.x;
				const float y = evt.y;
				return true;
			});

			bus->sendEvent<TestBusEvent>({ 1.0f, 2.0f });
		}

		void update(const float delta) { // update thread
            // mix test
            const float dt = delta * Engine::getInstance().getGameTimeMultiply();
            static float mix = 0.7f;
            static bool na = false;
            const float step = 1.5f * dt;
            static uint8_t animNum = 2;
            if (!na) {
                if (mix > -4.0f) {
                    mix -= step;
                } else {
                    mix = -4.0f;
                    na = true;
                }
            } else {
                if (mix < 5.0f) {
                    mix += step;
                } else {
                    mix = 5.0f;
                    na = false;
                    animNum = engine::random(1, 4);

                    if (animTree2) {
                        animTree2->getAnimator()->children()[animNum]->value().resetTime();
                    }
                }
            }

            if (animTree) {
                const float mix_val = std::clamp(mix, 0.0f, 1.0f);
                animTree->getAnimator()->children()[0]->value().setWeight(mix_val);
                animTree->getAnimator()->children()[1]->value().setWeight(1.0f - mix_val);
            }

            if (animTree2) {
                const float mix_val = std::clamp(mix, 0.0f, 1.0f);
                animTree2->getAnimator()->children()[0]->value().setWeight(mix_val);
                animTree2->getAnimator()->children()[animNum]->value().setWeight(1.0f - mix_val);
            }

            // check update for animation needed (object visible etc...)
            Engine::getInstance().getModule<Graphics>()->getAnimationManager()->update<MeshAnimationTree>(dt);

//            if (animTree) {
//                // v0
//                //mesh->graphics()->getSkeleton()->updateAnimation(delta, animTree);
//                // v1
//                //animTree->updateAnimation(dt, mesh->graphics()->getSkeleton().get());
//                // v2
//                //animTree->updateAnimation(dt);
//                mesh->graphics()->getSkeleton()->applyFrame(animTree);
//            }
//
//            if (animTree2) {
//                // v0
//                //mesh3->graphics()->getSkeleton()->updateAnimation(delta, animTree2);
//                // v1
//                //animTree2->updateAnimation(dt, mesh3->graphics()->getSkeleton().get());
//                // v2
//                //animTree2->updateAnimation(dt);
//                mesh3->graphics()->getSkeleton()->applyFrame(animTree2);
//            }
//
//            if (animTreeWindMill) {
//                // v0
//                //mesh7->graphics()->getSkeleton()->updateAnimation(delta, animTreeWindMill);
//                // v1
//                //animTreeWindMill->updateAnimation(dt, mesh7->graphics()->getSkeleton().get());
//                // v2
//                //animTreeWindMill->updateAnimation(dt);
//                mesh7->graphics()->getSkeleton()->applyFrame(animTreeWindMill);
//            }

            if (auto&& grassNode = grassMesh2->getNode()) {
                static float t = 0.0f;
                t += 2.0f * dt;

                if (t > math_constants::pi2) {
                    t -= math_constants::pi2;
                }

                grassNode->getRenderObject()->getRenderDescriptor()->setParamByName("u_time", &t, false);
                //const_cast<glm::mat4&>(grassNode->model())[0][0] = t;
                //grassNode->setLocalMatrix(glm::mat4(t));
            }
		}

		void draw(const float delta) {
			auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
			const uint32_t currentFrame = renderer->getCurrentFrame();

			const float moveCameraSpeed = 170.0f * delta;
			camera->movePosition(moveCameraSpeed * engine::as_normalized(wasd));

			const float rotateCameraSpeed = 24.0f * delta;
			camera->setRotation(camera->getRotation() + rotateCameraSpeed * (targetCameraRotation - camera->getRotation()));

			const float dt = delta * Engine::getInstance().getGameTimeMultiply();

			camera->calculateTransform();
			camera2->calculateTransform();

			//auto&& shadowProgram = const_cast<vulkan::VulkanGpuProgram*>(CascadeShadowMap::getSpecialPipeline(ShadowMapSpecialPipelines::SH_PIPEINE_PLAIN)->program);

			//shadowMap->updateShadowUniforms(shadowProgram, camera->getViewTransform());
			//shadowMap->updateShadowUniforms(program_mesh_default, camera->getViewTransform());

			shadowMap->updateShadowUniformsForRegesteredPrograms(camera->getViewTransform());

			////
			if (mesh3) {
				if (auto&& nm3 = mesh3->getNode()) {
					static uint32_t mesh3RemoveTest = 0;
					if (++mesh3RemoveTest == 5000) {
                        Engine::getInstance().getModule<Graphics>()->getAnimationManager()->removeTarget(animTree2, mesh3->graphics()->getSkeleton().get());
					}

                    if (mesh3RemoveTest == 10000) {
                        meshUpdateSystem.unregisterObject(mesh3);
						auto node = rootNode->getChildFromPtr(nm3);
						shadowCastNodes.erase(std::remove(shadowCastNodes.begin(), shadowCastNodes.end(), node), shadowCastNodes.end());
						rootNode->removeChild(node);
						mesh3 = nullptr;
                        Engine::getInstance().getModule<Graphics>()->getAnimationManager()->unregisterAnimation(animTree2);
                        delete animTree2;
                        animTree2 = nullptr;
                    }

				}
			}
            ////

			reloadRenderList(sceneRenderList, rootNode, cameraMatrixChanged, 0, engine::FrustumVisibleChecker(camera->getFrustum()));
			reloadRenderList(shadowRenderList, shadowCastNodes.data(), shadowCastNodes.size(), false, 0, engine::FrustumVisibleChecker(camera->getFrustum()));
			cameraMatrixChanged = false;

			meshUpdateSystem.updateRenderData();
			grassUpdateSystem.updateRenderData();
			skyBoxUpdateSystem.updateRenderData();
			plainUpdateSystem.updateRenderData();
			instanceMeshUpdateSystem.updateRenderData();


			const uint64_t wh = renderer->getWH();
			const uint32_t width = static_cast<uint32_t>(wh >> 0);
			const uint32_t height = static_cast<uint32_t>(wh >> 32);

			vulkan::VulkanCommandBuffer& commandBuffer = renderer->getRenderCommandBuffer();
			commandBuffer.begin();

			//////// shadow pass
			auto&& pr0 = mesh->setProgram(program_mesh_skin_shadow, shadowMap->getRenderPass());
			auto&& pr1 = mesh2->setProgram(program_mesh_skin_shadow, shadowMap->getRenderPass());
			auto&& pr2 = mesh3 ? mesh3->setProgram(program_mesh_skin_shadow, shadowMap->getRenderPass()) : nullptr;
			auto&& pr3 = mesh4->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			auto&& pr4 = mesh5->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			auto&& pr5 = mesh6->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			auto&& pr6 = mesh7->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			auto&& pr7 = forest->setProgram(program_mesh_instance_shadow, shadowMap->getRenderPass());
		
			for (auto&& m : testMehsesVec) {
				m->setProgram(program_mesh_skin_shadow, shadowMap->getRenderPass());
			}

			//if (animTree2) {
				// � ���� ������� ��������� � ���������, ��� ����� => �� ����� �������� ��������������, ���������� ������ ����
				//mesh->updateRenderData(wtr);
				//mesh2->updateRenderData(wtr2);
				//mesh3->updateRenderData(wtr3);
			//}

			renderShadowMap(shadowMap, shadowRenderList, commandBuffer, currentFrame);

			mesh->setProgram(pr0);
			mesh2->setProgram(pr1);
			if (mesh3) {
				mesh3->setProgram(pr2);
			}
			mesh4->setProgram(pr3);
			mesh5->setProgram(pr4);
			mesh6->setProgram(pr5);
			mesh7->setProgram(pr6);
			forest->setProgram(pr7);
			grassMesh2->setProgram(grass_default);

			for (auto&& m : testMehsesVec) {
				m->setProgram(program_mesh_skin);
			}

			//////// shadow pass

			commandBuffer.cmdBeginRenderPass(renderer->getMainRenderPass(), { {0, 0}, {width, height} }, &clearValues[0], 2, renderer->getFrameBuffer().m_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

			commandBuffer.cmdSetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f, false);
			commandBuffer.cmdSetScissor(0, 0, width, height);
			commandBuffer.cmdSetDepthBias(0.0f, 0.0f, 0.0f);
            //commandBuffer.cmdSetLineWidth(1.0f);

			const glm::mat4& cameraMatrix = camera->getMatrix();

			//mesh->draw(cameraMatrix, wtr, commandBuffer, currentFrame);
			//mesh2->draw(cameraMatrix, wtr2, commandBuffer, currentFrame);
			//mesh3->draw(cameraMatrix, wtr3, commandBuffer, currentFrame);

			//mesh->setCameraMatrix(cameraMatrix);
			//mesh2->setCameraMatrix(cameraMatrix);
			//mesh3->setCameraMatrix(cameraMatrix);

			//mesh->render(commandBuffer, currentFrame, &cameraMatrix);
			//mesh2->render(commandBuffer, currentFrame, &cameraMatrix);
			//mesh3->render(commandBuffer, currentFrame, &cameraMatrix);

			//if constexpr (renderBounds) {
			if (renderBounds) {
				renderNodesBounds(rootNode, cameraMatrix, commandBuffer, currentFrame, 0); // draw bounding boxes
			}

			sceneRenderList.render(commandBuffer, currentFrame, &cameraMatrix);

			////////
			if constexpr (0) {
				if (auto&& g = mesh->graphics()) {
					g->drawBoundingBox(cameraMatrix, mesh->getNode()->model(), commandBuffer, currentFrame);
				}

				if (auto&& g = mesh2->graphics()) {
					g->drawBoundingBox(cameraMatrix, mesh2->getNode()->model(), commandBuffer, currentFrame);
				}

				if (auto&& g = mesh3->graphics()) {
					g->drawBoundingBox(cameraMatrix, mesh3->getNode()->model(), commandBuffer, currentFrame);
				}
				
				if (auto&& g = mesh4->graphics()) {
					g->drawBoundingBox(cameraMatrix, mesh4->getNode()->model(), commandBuffer, currentFrame);
				}

				if (auto&& g = mesh5->graphics()) {
					g->drawBoundingBox(cameraMatrix, mesh5->getNode()->model(), commandBuffer, currentFrame);
				}

				if (auto&& g = mesh6->graphics()) {
					g->drawBoundingBox(cameraMatrix, mesh6->getNode()->model(), commandBuffer, currentFrame);
				}

				if (auto&& g = mesh7->graphics()) {
					g->drawBoundingBox(cameraMatrix, mesh7->getNode()->model(), commandBuffer, currentFrame);
				}
			}

			{
				auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
				auto&& autoBatcher = renderHelper->getAutoBatchRenderer();

				const uint32_t idxs[6] = {
					0, 1, 2,
					2, 1, 3
				};

				constexpr uint32_t vertexBufferSize = 4 * sizeof(TexturedVertex);
				constexpr uint32_t indexBufferSize = 6 * sizeof(uint32_t);

				//// floor
				static auto&& pipeline_shadow_test = CascadeShadowMap::getSpecialPipeline(ShadowMapSpecialPipelines::SH_PIPEINE_PLAIN);
				static const vulkan::GPUParamLayoutInfo* mvp_layout2 = pipeline_shadow_test->program->getGPUParamLayoutByName("mvp");

				const float tc = 16.0f;
				TexturedVertex floorVtx[4] = {
					{ {-1024.0f, -1024.0f, 0.0f},	{0.0f, tc} },
					{ {1024.0f, -1024.0f, 0.0f},	{tc, tc} },
					{ {-1024.0f, 1024.0f, 0.0f},	{0.0f, 0.0f} },
					{ {1024.0f, 1024.0f, 0.0f},		{tc, 0.0f} }
				};

				vulkan::RenderData renderDataFloor(const_cast<vulkan::VulkanPipeline*>(pipeline_shadow_test));
				renderDataFloor.setParamForLayout(mvp_layout2, &const_cast<glm::mat4&>(cameraMatrix), false);
				const glm::mat4& viewTransform = camera->getViewTransform();

				renderDataFloor.setParamByName("u_texture_arr", texture_array_test, false);
				renderDataFloor.setParamByName("u_texture_mask", texture_floor_mask, false);
				renderDataFloor.setParamByName("u_texture_normal", texture_floor_normal, false);
				renderDataFloor.setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				GPU_DEBUG_MARKER_INSERT(commandBuffer.m_commandBuffer, "project render shadow plain", 0.5f, 0.5f, 0.5f, 1.0f);
				autoBatcher->addToDraw(&renderDataFloor, sizeof(TexturedVertex), &floorVtx[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);

				//commandBuffer.cmdSetDepthBias(200.0f, 0.0f, 0.0f);
				////commandBuffer.cmdSetDepthBias(0.0f, 0.0f, 20.0f);
				autoBatcher->draw(commandBuffer, currentFrame);
				//commandBuffer.cmdSetDepthBias(0.0f, 0.0f, 0.0f);

				//// statl label
				auto statistic = Engine::getInstance().getModule<Statistic>();
				const bool vsync = Engine::getInstance().getModule<Graphics>()->config().v_sync;
				const char* gpuType = "";

				switch (renderer->getDevice()->gpuProperties.deviceType) {
					case VK_PHYSICAL_DEVICE_TYPE_OTHER:
						gpuType = "other";
						break;
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
						gpuType = "integrated";
						break;
					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
						gpuType = "discrete";
						break;
					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
						gpuType = "virtual";
						break;
					case VK_PHYSICAL_DEVICE_TYPE_CPU:
						gpuType = "cpu";
						break;
					default:
						break;
				}
#ifdef _DEBUG
				const char* buildType = "debug";
#else
				const char* buildType = "release";
#endif
				
				const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				const auto time = fmt::localtime(now);

				//std::shared_ptr<TextureFrame> frame = bitmapFont->createFrame(fmt_string("resolution: %dx%d\nv_sync: %s\ndraw calls: %d\nfps: %d\ncpu frame time: %f", width, height, vsync ? "on" : "off", statistic->drawCalls(), statistic->fps(), statistic->cpuFrameTime()));
				std::shared_ptr<TextureFrame> frame = bitmapFont->createFrame(
					fmt_string(
						"system time: {:%H:%M:%S}\nbuild type: {}\ngpu: {}({})\nresolution: {}x{}\nv_sync: {}\ndraw calls: {}\nfps: {}\ncpu frame time: {:.3}\nspeed mult: {:.3}\n\nWASD + mouse(camera control)",
						time, buildType, renderer->getDevice()->gpuProperties.deviceName, gpuType, width, height, vsync ? "on" : "off", statistic->drawCalls(), statistic->fps(), statistic->cpuFrameTime(), Engine::getInstance().getGameTimeMultiply()
					)
				);
				TextureFrameBounds frameBounds(frame.get());

				plainTest->graphics()->setFrame(frame);
				plainTest->getNode()->setBoundingVolume(BoundingVolume::make<CubeVolume>(glm::vec3(frameBounds.minx, frameBounds.miny, -0.1f), glm::vec3(frameBounds.maxx, frameBounds.maxy, 0.1f)));

				glm::mat4 wtr(1.0f);
                //scaleMatrix(wtr, glm::vec3(1.25f));
				translateMatrixTo(wtr, glm::vec3(-(width * 0.5f) + 16.0f, height * 0.5f - 30.0f, -1.0f));
				plainTest->getNode()->setLocalMatrix(wtr);

				const glm::mat4& camera2Matrix = camera2->getMatrix();

				reloadRenderList(uiRenderList, uiNode, false, 0, engine::FrustumVisibleChecker(camera2->getFrustum()));

				//if constexpr (renderBounds) {
				if (renderBounds) {
					renderNodesBounds(uiNode, camera2Matrix, commandBuffer, currentFrame, 0); // draw bounding boxes
				}

                imgui->graphics()->update(dt);
                statObserver->draw();
				uiRenderList.render(commandBuffer, currentFrame, &camera2Matrix);

				autoBatcher->draw(commandBuffer, currentFrame);
			}


			////////////////////
			if constexpr (1) { // test
				auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
				auto&& autoBatcher = renderHelper->getAutoBatchRenderer();
				auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();

				std::vector<engine::ProgramStageInfo> psiCTextured;
				psiCTextured.emplace_back(ProgramStage::VERTEX, "resources/shaders/texture.vsh.spv");
				psiCTextured.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/texture.psh.spv");
				vulkan::VulkanGpuProgram* programTextured = reinterpret_cast<vulkan::VulkanGpuProgram*>(gpuProgramManager->getProgram(psiCTextured));

				vulkan::VulkanPrimitiveTopology primitiveTopology = { vulkan::PrimitiveTopology::TRIANGLE_LIST, false };
				vulkan::VulkanRasterizationState rasterisation(vulkan::CullMode::CULL_MODE_NONE, vulkan::PoligonMode::POLYGON_MODE_FILL);
				vulkan::VulkanDepthState depthState(true, false, VK_COMPARE_OP_LESS);
				vulkan::VulkanStencilState stencilState(false);

				std::vector<VkVertexInputAttributeDescription> vertexInputAttributs = TexturedVertex::getVertexAttributesDescription();
				vulkan::VertexDescription vertexDescription;
				vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(TexturedVertex)));
				vertexDescription.attributesCount = static_cast<uint32_t>(vertexInputAttributs.size());
				vertexDescription.attributes = vertexInputAttributs.data();

				auto&& pipeline = renderer->getGraphicsPipeline(
					vertexDescription,
					primitiveTopology,
					rasterisation,
					vulkan::CommonBlendModes::blend_add,
					depthState,
					stencilState,
					programTextured
				);

				const vulkan::GPUParamLayoutInfo* mvp_layout = pipeline->program->getGPUParamLayoutByName("mvp");

				TexturedVertex vtx[4] = {
					{ {-100.0f, -100.0f, 0.0f}, {0.0f, 0.0f} },
					{ {100.0f, -100.0f, 0.0f}, {1.0f, 0.0f} },
					{ {-100.0f, 100.0f, 0.0f}, {0.0f, 1.0f} },
					{ {100.0f, 100.0f, 0.0f}, {1.0f, 1.0f} }
				};

				const uint32_t idxs[6] = {
					0, 1, 2,
					2, 1, 3
				};

				constexpr uint32_t vertexBufferSize = 4 * sizeof(TexturedVertex);
				constexpr uint32_t indexBufferSize = 6 * sizeof(uint32_t);

				vulkan::RenderData renderData(const_cast<vulkan::VulkanPipeline*>(pipeline));

				glm::mat4 wtr5(1.0f);
				translateMatrixTo(wtr5, lightPos);

				glm::mat4 billboardMatrix = engine::getBillboardViewMatrix(camera->getInvViewMatrix());

				glm::mat4 transform = camera->getMatrix() * wtr5 * billboardMatrix;
				renderData.setParamForLayout(mvp_layout, &transform, false);
				renderData.setParamByName("u_texture", texture_1, false);

				GPU_DEBUG_MARKER_INSERT(commandBuffer.m_commandBuffer, "project render vulkan sprite", 0.5f, 0.5f, 0.5f, 1.0f);
				autoBatcher->addToDraw(renderData.pipeline, sizeof(TexturedVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, renderData.params, commandBuffer, currentFrame);

				autoBatcher->draw(commandBuffer, currentFrame);
			}
			////////////////////

			if constexpr (0) { // test sprite on full screen
				auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
				auto&& autoBatcher = renderHelper->getAutoBatchRenderer();
				auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();

				std::vector<engine::ProgramStageInfo> psiCTextured;
				psiCTextured.emplace_back(ProgramStage::VERTEX, "resources/shaders/texture.vsh.spv");
				psiCTextured.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/texture.psh.spv");
				vulkan::VulkanGpuProgram* programTextured = reinterpret_cast<vulkan::VulkanGpuProgram*>(gpuProgramManager->getProgram(psiCTextured));

				vulkan::VulkanPrimitiveTopology primitiveTopology = { vulkan::PrimitiveTopology::TRIANGLE_LIST, false };
				vulkan::VulkanRasterizationState rasterisation(vulkan::CullMode::CULL_MODE_NONE, vulkan::PoligonMode::POLYGON_MODE_FILL);
				vulkan::VulkanDepthState depthState(false, false, VK_COMPARE_OP_LESS);
				vulkan::VulkanStencilState stencilState(false);

				std::vector<VkVertexInputAttributeDescription> vertexInputAttributs = TexturedVertex::getVertexAttributesDescription();
				vulkan::VertexDescription vertexDescription;
				vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(TexturedVertex)));
				vertexDescription.attributesCount = static_cast<uint32_t>(vertexInputAttributs.size());
				vertexDescription.attributes = vertexInputAttributs.data();

				auto&& pipeline = renderer->getGraphicsPipeline(
					vertexDescription,
					primitiveTopology,
					rasterisation,
					vulkan::CommonBlendModes::blend_add,
					depthState,
					stencilState,
					programTextured
				);

				const vulkan::GPUParamLayoutInfo* mvp_layout = pipeline->program->getGPUParamLayoutByName("mvp");

				TexturedVertex vtx[4] = {
					{ {-1.0f, -1.0f, 0.0f}, {0.45f, 0.95f} },
					{ {1.0f, -1.0f, 0.0f}, {0.55f, 0.95f} },
					{ {-1.0f, 1.0f, 0.0f}, {0.45f, 1.0f} },
					{ {1.0f, 1.0f, 0.0f}, {0.55f, 1.0f} }
				};

				const uint32_t idxs[6] = {
					0, 1, 2,
					2, 1, 3
				};

				constexpr uint32_t vertexBufferSize = 4 * sizeof(TexturedVertex);
				constexpr uint32_t indexBufferSize = 6 * sizeof(uint32_t);

				vulkan::RenderData renderData(const_cast<vulkan::VulkanPipeline*>(pipeline));

				glm::mat4 transform(1.0f);
				renderData.setParamForLayout(mvp_layout, &transform, false);
				renderData.setParamByName("u_texture", texture_1, false);

				GPU_DEBUG_MARKER_INSERT(commandBuffer.m_commandBuffer, "project render vulkan sprite", 0.5f, 0.5f, 0.5f, 1.0f);
				autoBatcher->addToDraw(renderData.pipeline, sizeof(TexturedVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, renderData.params, commandBuffer, currentFrame);

				autoBatcher->draw(commandBuffer, currentFrame);
			}

			if constexpr (0) { // ortho matrix draw
				const glm::mat4& cameraMatrix2 = camera2->getMatrix();

				if (animTree2) {

					static float angle = 0.0f;
					angle -= dt;

					glm::mat4 wtr4(1.0f);
					//rotateMatrix_xyz(wtr4, glm::vec3(1.57f, 0.0f, angle));
					scaleMatrix(wtr4, glm::vec3(50.0f));
					rotateMatrix_xyz(wtr4, glm::vec3(0.0, angle, 0.0f));
					translateMatrixTo(wtr4, glm::vec3(-float(width) * 0.5f + 100.0f, float(height) * 0.5f - mesh3->graphics()->getMaxCorner().y * 50.0f - 50.0f, 0.0f));
					mesh3->graphics()->draw(cameraMatrix2, wtr4, commandBuffer, currentFrame);
				}

				///////////

				auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
				auto&& autoBatcher = renderHelper->getAutoBatchRenderer();

				static auto&& pipeline = renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_TEXTURED);
				static const vulkan::GPUParamLayoutInfo* mvp_layout = pipeline->program->getGPUParamLayoutByName("mvp");

				TexturedVertex vtx[4] = {
					{ {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
					{ {252.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
					{ {0.0f, 100.0f, 0.0f}, {0.0f, 0.0f} },
					{ {252.0f, 100.0f, 0.0f}, {1.0f, 0.0f} }
				};

				const uint32_t idxs[6] = {
					0, 1, 2,
					2, 1, 3
				};

				constexpr uint32_t vertexBufferSize = 4 * sizeof(TexturedVertex);
				constexpr uint32_t indexBufferSize = 6 * sizeof(uint32_t);

				//////////////////////////////
				vulkan::RenderData renderData(const_cast<vulkan::VulkanPipeline*>(pipeline));

				glm::mat4 wtr5(1.0f);
				translateMatrixTo(wtr5, glm::vec3(-float(width) * 0.5f + 150.0f, float(height) * 0.5f - 110.0f, 0.0f));

				glm::mat4 transform = cameraMatrix2 * wtr5;
				renderData.setParamForLayout(mvp_layout, &transform, false);
				renderData.setParamByName("u_texture", texture_1, false);

				GPU_DEBUG_MARKER_INSERT(commandBuffer.m_commandBuffer, "project render vulkan sprite", 0.5f, 0.5f, 0.5f, 1.0f);
				autoBatcher->addToDraw(renderData.pipeline, sizeof(TexturedVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, renderData.params, commandBuffer, currentFrame);

				for (TexturedVertex& tv : vtx) {
					tv.position[0] += 350.0f;
				}

				vulkan::RenderData renderData2(const_cast<vulkan::VulkanPipeline*>(pipeline), renderData.params);

				GPU_DEBUG_MARKER_INSERT(commandBuffer.m_commandBuffer, "project render vulkan sprite", 0.5f, 0.5f, 0.5f, 1.0f);
				autoBatcher->addToDraw(renderData2.pipeline, sizeof(TexturedVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, renderData2.params, commandBuffer, currentFrame);

				/*TexturedVertex vtx2[4] = {
					{ {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
					{ {256.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
					{ {0.0f, 256.0f, 0.0f}, {0.0f, 0.0f} },
					{ {256.0f, 256.0f, 0.0f}, {1.0f, 0.0f} }
				};

				for (TexturedVertex& tv : vtx2) {
					tv.position[1] -= 320.0f;
				}*/

				//vulkan::RenderData renderData2(const_cast<vulkan::VulkanPipeline*>(pipeline));
				//renderData2.setParamForLayout(mvp_layout, &transform, false);
				//renderData2.setParamByName("u_texture", texture_text, false);

				/*static auto&& pipeline_shadow_test2 = renderHelper->getPipeline(CommonPipelines::TEST2);
				static const vulkan::GPUParamLayoutInfo* mvp_layout3 = pipeline_shadow_test2->program->getGPUParamLayoutByName("mvp");

				vulkan::RenderData renderData2(const_cast<vulkan::VulkanPipeline*>(pipeline_shadow_test2));
				renderData2.setParamForLayout(mvp_layout3, &transform, false);
				renderData2.setParamByName("u_texture", completeDepthTexture, false);
				//static glm::vec4 aaa(0.0f);
				//renderData2.setParamByName("aaa", &aaa, false);

				autoBatcher->addToDraw(&renderData2, sizeof(TexturedVertex), &vtx2[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);*/

				///////
				/* // cascades debug
				glm::mat4 wtr6(1.0f);
				translateMatrixTo(wtr6, glm::vec3(-float(width) * 0.5f + 150.0f, float(height) * 0.5f - 70.0f, 0.0f));
				glm::mat4 transform222 = cameraMatrix2 * wtr6;
				TexturedVertex vtx2[4] = {
					{ {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
					{ {256.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
					{ {0.0f, 256.0f, 0.0f}, {0.0f, 0.0f} },
					{ {256.0f, 256.0f, 0.0f}, {1.0f, 0.0f} }
				};

				for (TexturedVertex& tv : vtx2) {
					tv.position[1] -= 320.0f;
				}

				static auto&& pipeline_shadow_test2 = renderHelper->getPipeline(CommonPipelines::TEST2);
				static const vulkan::GPUParamLayoutInfo* mvp_layout3 = pipeline_shadow_test2->program->getGPUParamLayoutByName("mvp");

				vulkan::RenderData renderData2(const_cast<vulkan::VulkanPipeline*>(pipeline_shadow_test2));
				renderData2.setParamForLayout(mvp_layout3, &transform222, false);
				float cascade = 0;
				renderData2.setParamByName("cascade", &cascade, true);
				renderData2.setParamByName("u_texture", completeDepthTexture, false);

				autoBatcher->addToDraw(&renderData2, sizeof(TexturedVertex), &vtx2[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);
				autoBatcher->draw(commandBuffer, currentFrame);

				for (TexturedVertex& tv : vtx2) {
					tv.position[0] += 260.0f;
				}

				cascade = 1;
				renderData2.setParamByName("cascade", &cascade, true);
				autoBatcher->addToDraw(&renderData2, sizeof(TexturedVertex), &vtx2[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);
				autoBatcher->draw(commandBuffer, currentFrame);

				for (TexturedVertex& tv : vtx2) {
					tv.position[0] += 260.0f;
				}

				cascade = 2;
				renderData2.setParamByName("cascade", &cascade, true);
				autoBatcher->addToDraw(&renderData2, sizeof(TexturedVertex), &vtx2[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);
				autoBatcher->draw(commandBuffer, currentFrame);
				*/ // cascades debug
				///////

				autoBatcher->draw(commandBuffer, currentFrame);

				/*size_t vOffset;
				size_t iOffset;

				auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();

				auto&& vBuffer = renderHelper->addDynamicVerteces(&vtx[0], vertexBufferSize, vOffset);
				auto&& iBuffer = renderHelper->addDynamicIndices(&idxs[0], indexBufferSize, iOffset);

				static auto&& pipeline = renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_TEXTURED);
				static const vulkan::GPUParamLayoutInfo* mvp_layout = pipeline->program->getGPUParamLayoutByName("mvp");

				//vulkan::VulkanPushConstant pushConstats;

				glm::mat4 wtr5(1.0f);
				translateMatrixTo(wtr5, glm::vec3(-float(width) * 0.5f + 150.0f, float(height) * 0.5f - 110.0f, 0.0f));

				glm::mat4 transform = cameraMatrix2 * wtr5;

				vulkan::RenderData renderData(const_cast<vulkan::VulkanPipeline*>(pipeline));
				renderData.vertexes = &vBuffer;
				renderData.indexes = &iBuffer;

				vulkan::RenderData::RenderPart renderPart{
													static_cast<uint32_t>(iOffset / sizeof(uint32_t)),	// firstIndex
													6,													// indexCount
													0,													// vertexCount (parameter no used with indexed render)
													0,													// firstVertex
													1,													// instanceCount (can change later)
													0,													// firstInstance (can change later)
													vOffset,											// vbOffset
													0													// ibOffset
				};

				renderData.setRenderParts(&renderPart, 1);

				renderData.setParamForLayout(mvp_layout, &transform, false);
				renderData.setParamByName("u_texture", texture_1, false);

				renderData.prepareRender(commandBuffer);
				renderData.render(commandBuffer, currentFrame, 0, nullptr);

				renderData.setRenderParts(nullptr, 0);*/

				/////
				//const_cast<vulkan::VulkanGpuProgram*>(pipeline->program)->setValueToLayout(mvp_layout, &transform, &pushConstats);

				//const VkDescriptorSet set = texture_1->getSingleDescriptor();
				//if (set != VK_NULL_HANDLE) {
					//VkDescriptorSet* textureSets = static_cast<VkDescriptorSet*>(alloca(sizeof(VkDescriptorSet) * 1));
					//textureSets[0] = set;

					//commandBuffer.renderIndexed(pipeline, currentFrame, &pushConstats, 0, 0, nullptr, 1, textureSets, vBuffer, iBuffer, iOffset / sizeof(uint32_t), 6, 0, 1, 0, vOffset, 0);
				//}

			}
			////////

			commandBuffer.cmdEndRenderPass();
			commandBuffer.end();
		}

		void resize(const uint16_t w, const uint16_t h) {
			camera->resize(w, h);
			camera2->resize(w, h);
		}

	private:

	};

	Application::Application() noexcept {
		_customData = new ApplicationCustomData();
	}

    void Application::requestFeatures() {
        Engine::getInstance().getModule<Graphics>()->features().request<CascadeShadowMap>();
    }

	void Application::freeCustomData() {
		if (_customData) {
			delete _customData;
			_customData = nullptr;
			LOG_TAG(Application, "finished");
		}
	}

    void Application::onEngineInitComplete() {
        _customData->onEngineInitComplete();
    }

	void Application::nextFrame(const float delta) {
		_customData->draw(delta);
	}

	void Application::update(const float delta) {
		_customData->update(delta);
	}

	void Application::resize(const uint16_t w, const uint16_t h) {
		if (w != 0 && h != 0) {
			_customData->resize(w, h);
		}
	}

    Version Application::version() const noexcept {
        return Version(1, 0, 0);
    }

    const char* Application::getName() const noexcept {
        return "example";
    }
}

class A1 {
public:
	void f() {
		printf("A1::f\n");
	}
};

class B1 : public A1 {
public:
	void f() {
		printf("B1::f\n");
	}
};

template <typename T, typename... Args>
struct MyTuple {
	T value;
	MyTuple<Args...> tail;
};

template <typename T>
struct MyTuple<T> {
	T value;
};

struct alignas(8) MyBitStruct
{
	alignas(4) uint16_t a;
	alignas(4) uint16_t b;
	alignas(4) uint16_t c;
};

template <typename T>
struct RetType {
	using type = const T&;
};

template <>
struct RetType<int> {
	using type = int;
};

template <typename T>
using return_type = RetType<T>::type;

template<typename T>
auto test_func(const T& t) -> return_type<T> {
	return return_type<T>(t);
}

int main() {
	auto testInt = test_func(10);

	MyBitStruct mbbb;
	auto sssize = sizeof(MyBitStruct);

	MyTuple<int, int, float> myTuple;

	bool isSmartPtr1 = engine::is_smart_pointer<int>::value;
	bool isSmartPtr2 = engine::is_smart_pointer<std::shared_ptr<int>>::value;
	bool isSmartPtr3 = engine::is_smart_pointer_v<engine::linked_ptr<int>>;
	bool isSmartPtr4 = engine::is_smart_pointer_v<std::unique_ptr<int>>;
	bool isSmartPtr5 = engine::is_smart_pointer_v<int*>;

	A1 a1;
	a1.f();

	B1 b1;
	b1.f();

	class TestLinked : public engine::control_block<TestLinked> {
	public:
		inline void empty() const {}
		void destroy() {}
	private:
	};

	//engine::linked_weak_ptr<TestLinked> weak_test;
	//auto ptr1 = weak_test.lock();

	//engine::linked_weak_ptr_control<TestLinked> weak_control;

	engine::linked_ptr<TestLinked> testPtr(new TestLinked());
	engine::linked_ptr<TestLinked> testPtr2 = testPtr;
	{
		engine::linked_ptr<TestLinked> testPtr3 = testPtr;
		auto uc = testPtr.use_count();
	}

	auto uc = testPtr.use_count();
	testPtr->empty();

	engine::linked_ptr<TestLinked> testPtr4 = testPtr.get();
	uc = testPtr.use_count();

	engine::linked_ptr<TestLinked> testPtr5 = std::move(testPtr4);
	if (testPtr5) {
		uc = testPtr.use_count();
	}

	engine::linked_ptr<TestLinked> testPtr6;
	testPtr6 = testPtr5;

	engine::linked_ptr<TestLinked> testPtr7(new TestLinked());
	testPtr7 = testPtr6;

	struct A {
		A() : a(0) {}
		A(int aa) : a(aa) {}
		int a;
	};
	struct B {

		B() : a(0) {}
		B(int aa) : a(aa) {}
		int a;
	};

	engine::ComponentImpl<A> c1;
	engine::ComponentImpl<B> c2;
	engine::ComponentImpl<B> c3;
	engine::ComponentImpl<A> c4;

	auto cid1 = engine::Component::rtti<A>();
	auto cid2 = engine::Component::rtti<B>();

	auto cid11 = c1.rtti();
	auto cid12 = c2.rtti();
	auto cid13 = c3.rtti();
	auto cid14 = c4.rtti();

	auto&& c5 = engine::makeComponent<B>(110);
	auto cid15 = c5->rtti();

	delete c5;

	auto&& c6 = engine::allocateComponent<B>(engine::AllocatorPlaceholder(), 100);
	auto cid16 = c6->rtti();
	delete c6;

	ENGINE_BREAK_CONDITION(true);

	auto ri = engine::random(0, 10);
	auto rf = engine::random(-10.0f, 20.0f);
	/*
	{
		engine::ExecutionTime t("fmt_string2");
		for (size_t i = 0; i < 1000; ++i) {
			const char* s = engine::fmt_string("{}, {}, {}", "some text", 10 + i, i * 0.1f);
		}
	}

	{
		engine::ExecutionTime t("fmt_string");
		for (size_t i = 0; i < 1000; ++i) {
			const std::string s = engine::fmtString("{}, {}, {}", "some text", 10 + i, i * 0.1f);
		}
	}

	{
		engine::ExecutionTime t("fmt::format");
		for (size_t i = 0; i < 1000; ++i) {
			const std::string s = fmt::format("{}, {}, {}", "some text", 10 + i, i * 0.1f);
		}
	}

	{
		engine::ExecutionTime t("std::format");
		for (size_t i = 0; i < 1000; ++i) {
			const std::string s = std::format("{}, {}, {}", "some text", 10 + i, i * 0.1f);
		}
	}

	char buffer[1024];
	fmt::format_to(buffer, "{}", 42);
	*/

    half half_float_value = 0.5f;

	//////////////////////////////////
	engine::EngineConfig cfg;
	cfg.fpsDraw = 120;
	cfg.fpsLimitTypeDraw = engine::FpsLimitType::F_CPU_SLEEP;
	cfg.graphicsCfg = { engine::GpuType::DISCRETE, true, false,
                        engine::Version(1, 2, 182) }; // INTEGRATED, DISCRETE
	cfg.graphicsCfg.gpu_features.geometryShader = 1;
    //cfg.graphicsCfg.gpu_features.wideLines = 1;
	//cfg.graphicsCfg.gpu_features.fillModeNonSolid = 1; // example to enable POLYGON_MODE_LINE or POLYGON_MODE_POINT

	engine::Engine::getInstance().init(cfg);
	return 123;
}