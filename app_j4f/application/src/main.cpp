#include <Engine/Core/Application.h>
#include <Engine/Core/Engine.h>
#include <Engine/File/FileManager.h>
#include <Engine/File/FileSystem.h>
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
#include <Engine/Core/Math/math.h>
#include <Engine/Graphics/Vulkan/vkHelper.h>
#include <Engine/Graphics/Scene/Camera.h>
#include <Engine/Graphics/Render/RenderHelper.h>
#include <Engine/Graphics/Render/AutoBatchRender.h>
#include <Engine/Graphics/Render/RenderList.h>
#include <Engine/Core/Memory/MemoryChunk.h>
#include <Engine/Input/Input.h>
#include <Engine/Core/Handler.h>
#include <Engine/Utils/Debug/Profiler.h>
//#include <Engine/Graphics/Text/TextImage.h>
#include <Engine/Graphics/Text/FontLoader.h>
#include <Engine/Graphics/Text/BitmapFont.h>

#include <Engine/Core/Math/random.h>
#include <Engine/Core/Threads/FlowTask.h>
#include <Engine/Core/Threads/Looper.h>

#include <Engine/ECS/Component.h>
#include <Engine/Core/BitMask.h>

#include <Engine/Graphics/Scene/Shadows/CascadeShadowMap.h>

#include <Engine/Graphics/Scene/Node.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>
#include <Engine/Graphics/Scene/BoundingVolume.h>

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

	NodeRenderer<Mesh>* grassMesh = nullptr;
	NodeRenderer<Plain>* plainTest = nullptr;
	
	MeshAnimationTree* animTree = nullptr;
	MeshAnimationTree* animTree2 = nullptr;
	MeshAnimationTree* animTreeWindMill = nullptr;

	vulkan::VulkanTexture* texture_1 = nullptr;
	vulkan::VulkanTexture* texture_text = nullptr;
	vulkan::VulkanTexture* texture_floor_mask = nullptr;
	vulkan::VulkanTexture* texture_floor_normal = nullptr;
	vulkan::VulkanTexture* texture_array_test = nullptr;

	vulkan::VulkanGpuProgram* program_mesh_default = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_shadow = nullptr;

	VkClearValue clearValues[2];

	RenderList sceneRenderList;
	RenderList shadowRenderList;
	RenderList uiRenderList;

	glm::vec3 wasd(0.0f);

	////////////////////
	/// cascade shadow map
	constexpr uint8_t SHADOW_MAP_CASCADE_COUNT = 4;
	constexpr uint16_t SHADOWMAP_DIM = 4096; // VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	//constexpr uint16_t SHADOWMAP_DIM = 1024; // VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
	glm::vec3 lightPos = glm::vec3(-460.0f, -600.0f, 1000.0f);
	/// cascade shadow map

	glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
	//glm::vec2 lightMinMax(0.075f, 3.0f);
	glm::vec2 lightMinMax(0.4f, 1.65f);

	H_Node* rootNode;
	H_Node* uiNode;
	bool cameraMatrixChanged = true;

	constexpr bool renderBounds = false;

	//
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

			shadowMap->registerProgramAsReciever(program_mesh_default);

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

			shadowMap->registerProgramAsReciever(program_mesh_default);

			std::vector<glm::mat4> grassTransforms;
			const float space = 26.0f;
			const uint32_t* data = reinterpret_cast<const uint32_t*>(img.data());
			for (size_t x = 0; x < img.width(); ++x) {
				for (size_t y = 0; y < img.height(); ++y) {
					const uint32_t v = data[y * img.width() + x];
					if ((v & 0x000000f0) >= 240) {
						glm::mat4 wtr(1.0f);

						const int grassType = engine::random(0, 100) < 5 ? engine::random(2, 4) : engine::random(0, 1);
						if (grassType > 1) {
							const float scale_xyz = (engine::random(25.0f, 35.0f) * 400.0f);
							scaleMatrix(wtr, glm::vec3(scale_xyz, scale_xyz, scale_xyz));
						} else {
							const float scale_xyz = (engine::random(25.0f, 60.0f) * 500.0f);
							const float scale_z = (engine::random(25.0f, 60.0f) * 350.0f);
							scaleMatrix(wtr, glm::vec3(scale_xyz, scale_xyz, scale_z));
						}

						rotateMatrix_xyz(wtr, glm::vec3(0.0f, 0.0f, engine::random(-3.1415926, 3.1415926)));
						translateMatrixTo(wtr, glm::vec3(-1024.0f + x * space, 1024.0f - y * space, grassType));
						grassTransforms.emplace_back(std::move(wtr));
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

		inline void setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			_mesh->setProgram(program, renderPass);
		}

		inline const RenderDescriptor& getRenderDescriptor() const { return _mesh->getRenderDescriptor(); }
		inline RenderDescriptor& getRenderDescriptor() { return _mesh->getRenderDescriptor(); }

	private:
		uint32_t _instanceCount;
		Mesh* _mesh;
	};

	NodeRenderer<GrassRenderer>* grassMesh2 = nullptr;

	class ApplicationCustomData : public InputObserver, public ICameraTransformChangeObserver {
	public:

		void onCameraTransformChanged(const Camera* camera) override {
			shadowMap->updateCascades(camera);
			cameraMatrixChanged = true;
		}

		ApplicationCustomData() {
			PROFILE_TIME_SCOPED(ApplicationLoading)
			log("ApplicationCustomData");

			FileManager* fm = Engine::getInstance().getModule<FileManager>();
			auto&& fs = fm->getFileSystem<DefaultFileSystem>();
			fm->mapFileSystem(fs);

			Engine::getInstance().getModule<Input>()->addObserver(this);

			initCamera();
			create();

			camera->addObserver(this);
		}

		~ApplicationCustomData() {
			log("~ApplicationCustomData");

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

			static bool m1 = false;

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
						constexpr float k = 0.005f;
						camera->setRotation(camera->getRotation() + glm::vec3(-k * (event.y - y), 0.0f, k * (event.x - x)));
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
			camera->movePosition(glm::vec3(0.0f, 0.0f, dy * 20.0f));
			return true;
		}

		bool onInpuKeyEvent(const KeyEvent& event) override {

			switch (event.key) {
				case KeyBoardKey::K_E:
					event.state == InputEventState::IES_RELEASE ? wasd.y -= 0.5f : wasd.y += 0.5f;
					break;
				case KeyBoardKey::K_Q:
					event.state == InputEventState::IES_RELEASE ? wasd.y += 0.5f : wasd.y -= 0.5f;
					break;
				case KeyBoardKey::K_W:
					event.state == InputEventState::IES_RELEASE ? wasd.z -= 1.0f : wasd.z += 1.0f;
					break;
				case KeyBoardKey::K_S:
					event.state == InputEventState::IES_RELEASE ? wasd.z += 1.0f : wasd.z -= 1.0f;
					break;
				case KeyBoardKey::K_A:
					event.state == InputEventState::IES_RELEASE ? wasd.x += 1.0f : wasd.x -= 1.0f;
					break;
				case KeyBoardKey::K_D:
					event.state == InputEventState::IES_RELEASE ? wasd.x -= 1.0f : wasd.x += 1.0f;
					break;
				case KeyBoardKey::K_ESCAPE:
					if (event.state != InputEventState::IES_RELEASE) break;
					Engine::getInstance().getModule<Device>()->leaveMainLoop();
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

			camera = new Camera(width, height);
			camera->enableFrustum();

			camera->makeProjection(engine::math_constants::pi / 4.0f, static_cast<float>(width) / static_cast<float>(height), 1.0f, 4000.0f);
			//camera->makeOrtho(-float(width) * 0.5f, float(width) * 0.5f, -float(height) * 0.5f, float(height) * 0.5f, 1.0f, 1000.0f);
			camera->setRotation(glm::vec3(-engine::math_constants::pi / 3.0f, 0.0f, 0.0f));
			camera->setPosition(glm::vec3(0.0f, -500.0f, 300.0f));

			camera2 = new Camera(width, height);
			camera2->enableFrustum();
			camera2->makeOrtho(-float(width) * 0.5f, float(width) * 0.5f, -float(height) * 0.5f, float(height) * 0.5f, 1.0f, 1000.0f);
			camera2->setPosition(glm::vec3(0.0f, 0.0f, 200.0f));

			//clearValues[0].color = { 0.5f, 0.78f, 0.99f, 1.0f };
			clearValues[0].color = { 0.25f, 0.25f, 0.25f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			//////////////////////////////////
			shadowMap = new CascadeShadowMap(SHADOWMAP_DIM, SHADOW_MAP_CASCADE_COUNT, camera->getNearFar(), 250.0f, 1600.0f);
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

			std::vector<engine::ProgramStageInfo> psi;
			psi.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_skin.vsh.spv");
			psi.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh.psh.spv");
			VulkanGpuProgram* program_gltf = gpuProgramManager->getProgram(psi);

			program_mesh_default = program_gltf;
			program_mesh_shadow = CascadeShadowMap::getShadowProgram<Mesh>();
			VulkanGpuProgram* shadowPlainProgram = const_cast<VulkanGpuProgram*>(CascadeShadowMap::getSpecialPipeline(ShadowMapSpecialPipelines::SH_PIPEINE_PLAIN)->program);

			glm::vec3 lightDir = as_normalized(-lightPos);

			auto l = program_mesh_default->getGPUParamLayoutByName("lightDirection");
			program_mesh_default->setValueToLayout(l, &lightDir, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			auto l2 = program_mesh_default->getGPUParamLayoutByName("lightMinMax");
			program_mesh_default->setValueToLayout(l2, &lightMinMax, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			auto l3 = shadowPlainProgram->getGPUParamLayoutByName("lightMin");
			auto l31 = shadowPlainProgram->getGPUParamLayoutByName("lightDirection");
			shadowPlainProgram->setValueToLayout(l3, &lightMinMax.x, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			shadowPlainProgram->setValueToLayout(l31, &lightDir, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			auto l4 = program_mesh_default->getGPUParamLayoutByName("lightColor");
			auto l5 = shadowPlainProgram->getGPUParamLayoutByName("lightColor");
			program_mesh_default->setValueToLayout(l4, &lightColor, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			shadowPlainProgram->setValueToLayout(l5, &lightColor, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			shadowMap->registerProgramAsReciever(program_mesh_default);
			shadowMap->registerProgramAsReciever(shadowPlainProgram);

			TextureLoadingParams tex_params;
			tex_params.files = { 
				"resources/assets/models/chaman/textures/Ti-Pche_Mat_baseColor.png",
				"resources/assets/models/chaman/textures/Ti-Pche_Mat_normal.png",
			};
			tex_params.flags->async = 1;
			tex_params.flags->use_cache = 1;
			//tex_params.formatType = engine::TextureFormatType::SRGB;
			tex_params.imageViewTypeForce = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			auto texture_zombi = assm->loadAsset<vulkan::VulkanTexture*>(tex_params);

			TextureLoadingParams tex_params2;
			tex_params2.files = { 
				"resources/assets/models/warcraft3/textures/Armor_2_baseColor.png",
				"resources/assets/models/warcraft3/textures/Armor_2.001_normal.png"
				//"resources/assets/models/warcraft3/textures/Armor_2_normal.png"
			};
			tex_params2.flags->async = 1;
			tex_params2.flags->use_cache = 1;

			//tex_params2.formatType = engine::TextureFormatType::SRGB;
			tex_params2.imageViewTypeForce = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			auto texture_v = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);
			tex_params2.files = { 
				"resources/assets/models/warcraft3/textures/body_baseColor.png",
				"resources/assets/models/warcraft3/textures/body_normal.png"
			};
			auto texture_v2 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);
			tex_params2.files = { 
				"resources/assets/models/warcraft3/textures/Metal_baseColor.png",
				"resources/assets/models/warcraft3/textures/Metal_normal.png"
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
			};
			auto texture_t = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/tree1/textures/branches_baseColor.png",
				"resources/assets/models/tree1/textures/branches_normal.png",
			};
			auto texture_t2 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/pineTree/textures/Leavs_baseColor.png",
				"resources/assets/models/pineTree/textures/Trank_normal.png"
			};
			auto texture_t3 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/pineTree/textures/Trank_baseColor.png",
				"resources/assets/models/pineTree/textures/Trank_normal.png"
			};
			auto texture_t4 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/vikingHut/textures/texture1.jpg",
				"resources/assets/models/vikingHut/textures/Main_Material2_normal.jpg"
			};

			auto texture_t5 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/grass/textures/grass76.png",
				"resources/assets/models/grass/textures/grass77.png",
				"resources/assets/models/grass/textures/flowers16.png",
				"resources/assets/models/grass/textures/flowers26.png",
				"resources/assets/models/grass/textures/grass2.png",
			};
			auto texture_t6 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			tex_params3.files = { 
				"resources/assets/models/windmill/textures/standardSurface1_baseColor.png",
				"resources/assets/models/windmill/textures/standardSurface1_normal.png"
			};
			auto texture_t7 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params3);

			TextureLoadingParams tex_params_logo;
			tex_params_logo.files = { "resources/assets/textures/vulkan_logo.png" };
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
			mesh_params3.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params3.latency = 1;
			mesh_params3.flags->async = 1;
			mesh_params3.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params4;
			mesh_params4.file = "resources/assets/models/pineTree/scene.gltf";
			mesh_params4.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params4.latency = 1;
			mesh_params4.flags->async = 1;
			mesh_params4.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params5;
			mesh_params5.file = "resources/assets/models/vikingHut/scene.gltf";
			mesh_params5.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params5.latency = 1;
			mesh_params5.flags->async = 1;
			mesh_params5.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params6;
			mesh_params6.file = "resources/assets/models/windmill/scene.gltf";
			mesh_params6.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TANGENT, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params6.latency = 3;
			mesh_params6.flags->async = 1;
			mesh_params6.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params_grass;
			mesh_params_grass.file = "resources/assets/models/grass/scene.gltf";
			mesh_params_grass.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::TEXCOORD_0);
			mesh_params_grass.latency = 1;
			mesh_params_grass.flags->async = 1;
			mesh_params_grass.graphicsBuffer = meshesGraphicsBuffer;

			mesh = new NodeRenderer<Mesh>();
			mesh2 = new NodeRenderer<Mesh>();
			mesh3 = new NodeRenderer<Mesh>();
			mesh4 = new NodeRenderer<Mesh>();
			mesh5 = new NodeRenderer<Mesh>();
			mesh6 = new NodeRenderer<Mesh>();
			mesh7 = new NodeRenderer<Mesh>();
			//grassMesh = new NodeRenderer<Mesh>();
			grassMesh2 = new NodeRenderer<GrassRenderer>();
			
			assm->loadAsset<Mesh*>(mesh_params, [program_gltf, texture_zombi, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_zombi, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				animTree = new MeshAnimationTree(0.0f, asset->getNodesCount(), asset->getSkeleton()->getLatency());
				animTree->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[2], 1.0f, asset->getSkeleton()->getLatency()));
				animTree->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[1], 0.0f, asset->getSkeleton()->getLatency()));

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->pipelineAttributesChanged();

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(25.0f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.45f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(-100.0f, -0.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<SphereVolume>(glm::vec3(0.0f, 1.45f, 0.0f), 1.8f));
				rootNode->addChild(node);

				mesh->setGraphics(asset);
				mesh->setNode(node->value());

				shadowRenderList.addDescriptor(&asset->getRenderDescriptor());
				});

			assm->loadAsset<Mesh*>(mesh_params, [program_gltf, texture_zombi](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_zombi, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				asset->setSkeleton(mesh->graphics()->getSkeleton());

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->pipelineAttributesChanged();

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(20.0f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, -0.45f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(100.0f, -0.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<SphereVolume>(glm::vec3(0.0f, 1.45f, 0.0f), 1.8f));
				rootNode->addChild(node);

				mesh2->setGraphics(asset);
				mesh2->setNode(node->value());

				shadowRenderList.addDescriptor(&asset->getRenderDescriptor());
				});

			assm->loadAsset<Mesh*>(mesh_params2, [program_gltf, texture_v, texture_v2, texture_v3](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_v, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				asset->getRenderDataAt(3)->setParamByName("u_texture", texture_v3, false);
				asset->getRenderDataAt(7)->setParamByName("u_texture", texture_v2, false); // eye
				asset->getRenderDataAt(8)->setParamByName("u_texture", texture_v2, false); // head
				asset->getRenderDataAt(9)->setParamByName("u_texture", texture_v3, false); // helm
				asset->getRenderDataAt(11)->setParamByName("u_texture", texture_v3, false);
				asset->getRenderDataAt(13)->setParamByName("u_texture", texture_v3, false);

				animTree2 = new MeshAnimationTree(0.0f, asset->getNodesCount(), asset->getSkeleton()->getLatency());

				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[1], 1.0f, asset->getSkeleton()->getLatency(), 1.0f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[2], 0.0f, asset->getSkeleton()->getLatency(), 1.0f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[11], 0.0f, asset->getSkeleton()->getLatency(), 1.25f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[10], 0.0f, asset->getSkeleton()->getLatency(), 1.0f));
				animTree2->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[0], 0.0f, asset->getSkeleton()->getLatency(), 1.4f));

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(30.0f));
				directMatrix_yz(wtr, 0.0f, 1.0f);
				translateMatrixTo(wtr, glm::vec3(0.0f, 0.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<SphereVolume>((asset->getMinCorner() + asset->getMaxCorner()) * 0.5f, 1.0f));
				//node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(asset->getMinCorner(), asset->getMaxCorner()));
				rootNode->addChild(node);

				mesh3->setGraphics(asset);
				mesh3->setNode(node->value());

				shadowRenderList.addDescriptor(&asset->getRenderDescriptor());
				});

			assm->loadAsset<Mesh*>(mesh_params3, [program_gltf, texture_t, texture_t2, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_t, false);
				asset->getRenderDataAt(1)->setParamByName("u_texture", texture_t2, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->pipelineAttributesChanged();

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
				mesh4->setNode(node->value());

				shadowRenderList.addDescriptor(&asset->getRenderDescriptor());
				});

			assm->loadAsset<Mesh*>(mesh_params4, [program_gltf, texture_t3, texture_t4, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_t3, false);
				asset->getRenderDataAt(1)->setParamByName("u_texture", texture_t4, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->pipelineAttributesChanged();

				////////////////////
				glm::mat4 wtr(1.0f);
				scaleMatrix(wtr, glm::vec3(0.75f));
				rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.0f, 0.0f));
				translateMatrixTo(wtr, glm::vec3(570.0f, -250.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				node->value().setBoundingVolume(BoundingVolume::make<CubeVolume>(asset->getMinCorner(), asset->getMaxCorner()));
				rootNode->addChild(node);

				mesh5->setGraphics(asset);
				mesh5->setNode(node->value());

				shadowRenderList.addDescriptor(&asset->getRenderDescriptor());
				});

			assm->loadAsset<Mesh*>(mesh_params5, [program_gltf, texture_t5, texture_t6, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_t5, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->pipelineAttributesChanged();

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
				mesh6->setNode(node->value());

				shadowRenderList.addDescriptor(&asset->getRenderDescriptor());
				});

			assm->loadAsset<Mesh*>(mesh_params6, [program_gltf, texture_t7, texture_t6, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_t7, false);
				asset->setParamByName("u_shadow_map", shadowMap->getTexture(), false);

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->pipelineAttributesChanged();

				animTreeWindMill = new MeshAnimationTree(&asset->getMeshData()->animations[0],1.0f, asset->getSkeleton()->getLatency());

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
				mesh7->setNode(node->value());

				shadowRenderList.addDescriptor(&asset->getRenderDescriptor());
				});

			TextureData img("resources/assets/textures/t.jpg");
			GrassRenderer* grenderer = new GrassRenderer(img);

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

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->pipelineAttributesChanged();

				/////////////////////
				//grassMesh->setGraphics(asset);
				glm::mat4 wtr(1.0f);
				//scaleMatrix(wtr, glm::vec3(10000.0f));
				//rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.0f, 0.0f));
				//translateMatrixTo(wtr, glm::vec3(200.0f, -100.0f, 0.0f));

				H_Node* node = new H_Node();
				node->value().setLocalMatrix(wtr);
				rootNode->addChild(node);

				grenderer->setMesh(asset);
				grassMesh2->setGraphics(grenderer);
				grassMesh2->setNode(node->value());
				});

			plainTest = new NodeRenderer<Plain>();
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

				plainTest->setNode(node->value());

				plainTest->graphics()->setParamByName("u_texture", texture_floor_normal, false);
				plainTest->getRenderDescriptor()->order = 10;
			}

			TextureLoadingParams tex_params_floorArray;
			tex_params_floorArray.files = { 
				"resources/assets/textures/swamp5.jpg",
				"resources/assets/textures/sand5.jpg"
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

			bitmapFont = new BitmapFont(f, 16, 256, 256, 0);
			bitmapFont->addSymbols("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-+*/=&%#@!?<>,.()[];:@$^~_", 2, 0, 0xffffffff, 0x000000ff, 1.0f, 2, 2);
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
			plainTest->graphics()->renderState().blendMode = vulkan::CommonBlendModes::blend_alpha;
			plainTest->graphics()->pipelineAttributesChanged();
			plainTest->getNode()->setBoundingVolume(BoundingVolume::make<CubeVolume>(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(256, 256, 1.0f)));

			plainTest->graphics()->setFrame(bitmapFont->createFrame("hello world!"));
			plainTest->getNode()->setBoundingVolume(nullptr);
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

			engine::addTaskToFlow(task, depTask);
			engine::addTaskToFlow(task2, depTask);

			engine::Engine::getInstance().getModule<engine::ThreadPool>()->enqueue(0, task);
			engine::Engine::getInstance().getModule<engine::ThreadPool>()->enqueue(0, task2);

			//std::this_thread::sleep_for(std::chrono::milliseconds(10));

			engine::addTaskToFlow(task, depTask2);
			engine::addTaskToFlow(task2, depTask2);

			engine::BitMask32 mask;
			mask.setBit(0, 1);
			mask.setBit(1, 1);

			mask.setBit(33, 1);
			mask.setBit(34, 1);

			bool is1 = mask.checkBit(34);
			bool is2 = mask.checkBit(35);

			mask.nullify();

		}

		void draw(const float delta) {

			auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
			const uint32_t currentFrame = renderer->getCurrentFrame();

			camera->movePosition(100.0f * delta * engine::as_normalized(wasd));

			camera->calculateTransform();
			camera2->calculateTransform();

			//auto&& shadowProgram = const_cast<vulkan::VulkanGpuProgram*>(CascadeShadowMap::getSpecialPipeline(ShadowMapSpecialPipelines::SH_PIPEINE_PLAIN)->program);

			//shadowMap->updateShadowUniforms(shadowProgram, camera->getViewTransform());
			//shadowMap->updateShadowUniforms(program_mesh_default, camera->getViewTransform());

			shadowMap->updateShadowUniformsForRegesteredPrograms(camera->getViewTransform());

			// mix test
			static float mix = 0.7f;
			static bool na = false;
			const float step = 1.5f * delta;
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

			////
			if (animTree) {
				animTree->updateAnimation(delta, mesh->graphics()->getSkeleton());
				//mesh->graphics()->getSkeleton()->updateAnimation(delta, animTree);
			}

			if (animTree2) {
				animTree2->updateAnimation(delta, mesh3->graphics()->getSkeleton());
				//mesh3->graphics()->getSkeleton()->updateAnimation(delta, animTree2);
			}

			if (animTreeWindMill) {
				animTreeWindMill->updateAnimation(delta, mesh7->graphics()->getSkeleton());
				//mesh7->graphics()->getSkeleton()->updateAnimation(delta, animTreeWindMill);
			}

			////////
			if (auto&& grassNode = grassMesh2->getNode()) {
				static float t = 0.0f;
				t += 2.0f * delta;

				if (t > math_constants::pi2) {
					t -= math_constants::pi2;
				}

				const_cast<glm::mat4&>(grassNode->model())[0][0] = t;
				//grassNode->setLocalMatrix(glm::mat4(t));
			}

			//rootNode->execute_with<NodeMatrixUpdater>();
			reloadRenderList(sceneRenderList, rootNode, camera->getFrustum(), cameraMatrixChanged, 0);
			cameraMatrixChanged = false;

			mesh->updateRenderData();
			mesh2->updateRenderData();
			mesh3->updateRenderData();
			mesh4->updateRenderData();
			mesh5->updateRenderData();
			mesh6->updateRenderData();
			mesh7->updateRenderData();
			grassMesh2->updateRenderData();

			plainTest->updateRenderData();

			const uint64_t wh = renderer->getWH();
			const uint32_t width = static_cast<uint32_t>(wh >> 0);
			const uint32_t height = static_cast<uint32_t>(wh >> 32);

			vulkan::VulkanCommandBuffer& commandBuffer = renderer->getRenderCommandBuffer();
			commandBuffer.begin();

			//////// shadow pass
			mesh->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh2->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh3->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh4->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh5->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh6->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh7->setProgram(program_mesh_shadow, shadowMap->getRenderPass());

			//if (animTree2) {
				// у меня лайоуты совпадают у юниформов, для теней => не нужно вызывать переназначение, достаточно одного раза
				//mesh->updateRenderData(wtr);
				//mesh2->updateRenderData(wtr2);
				//mesh3->updateRenderData(wtr3);
			//}

			shadowMap->prepareToRender(commandBuffer);
			switch (shadowMap->techique()) {
				case ShadowMapTechnique::SMT_GEOMETRY_SH:
				{
					shadowMap->beginRenderPass(commandBuffer, 0);
					shadowRenderList.render(commandBuffer, currentFrame, nullptr);
					shadowMap->endRenderPass(commandBuffer);
				}
					break;
				default:
				{
					for (uint32_t i = 0; i < shadowMap->getCascadesCount(); ++i) {
						shadowMap->beginRenderPass(commandBuffer, i);

						//todo: проверять в какой каскад меш попадает и только там его и рисовать
						//mesh->setCameraMatrix(cascadesViewProjMatrixes[i]);
						//mesh2->setCameraMatrix(cascadesViewProjMatrixes[i]);
						//mesh3->setCameraMatrix(cascadesViewProjMatrixes[i]);

						//mesh->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
						//mesh2->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
						//mesh3->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
						//sceneRenderList.render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));

						shadowRenderList.render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));

						shadowMap->endRenderPass(commandBuffer);
					}
				}
					break;
			}

			mesh->setProgram(program_mesh_default);
			mesh2->setProgram(program_mesh_default);
			mesh3->setProgram(program_mesh_default);
			mesh4->setProgram(program_mesh_default);
			mesh5->setProgram(program_mesh_default);
			mesh6->setProgram(program_mesh_default);
			mesh7->setProgram(program_mesh_default);
			grassMesh2->setProgram(grass_default);
			//////// shadow pass

			commandBuffer.cmdBeginRenderPass(renderer->getMainRenderPass(), { {0, 0}, {width, height} }, &clearValues[0], 2, renderer->getFrameBuffer().m_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

			commandBuffer.cmdSetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f, false);
			commandBuffer.cmdSetScissor(0, 0, width, height);

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

			if constexpr (renderBounds) {
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

				const float tc = 8.0f;
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

				//// statl label
				auto statistic = Engine::getInstance().getModule<Statistic>();
				const bool vsync = Engine::getInstance().getModule<Graphics>()->config().v_sync;
				//std::shared_ptr<TextureFrame> frame = bitmapFont->createFrame(fmt_string("resolution: %dx%d\nv_sync: %s\ndraw calls: %d\nfps: %d\ncpu frame time: %f", width, height, vsync ? "on" : "off", statistic->drawCalls(), statistic->fps(), statistic->cpuFrameTime()));
				std::shared_ptr<TextureFrame> frame = bitmapFont->createFrame(fmt_string("resolution: {}x{}\nv_sync: {}\ndraw calls: {}\nfps: {}\ncpu frame time: {:.3}", width, height, vsync ? "on" : "off", statistic->drawCalls(), statistic->fps(), statistic->cpuFrameTime()));
				TextureFrameBounds frameBounds(frame.get());

				plainTest->graphics()->setFrame(frame);
				plainTest->getNode()->setBoundingVolume(BoundingVolume::make<CubeVolume>(glm::vec3(frameBounds.minx, frameBounds.miny, -0.1f), glm::vec3(frameBounds.maxx, frameBounds.maxy, 0.1f)));

				glm::mat4 wtr(1.0f);
				translateMatrixTo(wtr, glm::vec3(width * 0.5f - 250.0f, height * 0.5f - 30.0f, -1.0f));
				plainTest->getNode()->setLocalMatrix(wtr);

				const glm::mat4& camera2Matrix = camera2->getMatrix();

				reloadRenderList(uiRenderList, uiNode, camera2->getFrustum(), false, 0);

				if constexpr (renderBounds) {
					renderNodesBounds(uiNode, camera2Matrix, commandBuffer, currentFrame, 0); // draw bounding boxes
				}

				uiRenderList.render(commandBuffer, currentFrame, &camera2Matrix);

				autoBatcher->draw(commandBuffer, currentFrame);
			}

			if constexpr (0) { // ortho matrix draw
				const glm::mat4& cameraMatrix2 = camera2->getMatrix();

				if (animTree2) {

					static float angle = 0.0f;
					angle -= delta;

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

	Application::Application() {
		_customData = new ApplicationCustomData();
	}

	void Application::freeCustomData() {
		if (_customData) {
			delete _customData;
			_customData = nullptr;
		}
	}

	void Application::nextFrame(const float delta) {
		_customData->draw(delta);
	}

	void Application::resize(const uint16_t w, const uint16_t h) {
		if (w != 0 && h != 0) {
			_customData->resize(w, h);
		}
	}
}

int main() {
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

	engine::ComponentHolder<A> c1;
	engine::ComponentHolder<B> c2;
	engine::ComponentHolder<B> c3;
	engine::ComponentHolder<A> c4;

	auto cid1 = engine::Component::rtti<A>();
	auto cid2 = engine::Component::rtti<B>();

	auto cid11 = c1.rtti();
	auto cid12 = c2.rtti();
	auto cid13 = c3.rtti();
	auto cid14 = c4.rtti();

	auto&& c5 = engine::makeComponent<B>(110);
	auto cid15 = c5->rtti();

	delete c5;

	auto ri = engine::random(0, 10);
	auto rf = engine::random(-10.0f, 20.0f);

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

	char buffer[1024];
	fmt::format_to(buffer, "{}", 42);

	////////////////////////////////////////
	engine::EngineConfig cfg;
	engine::Engine::getInstance().init(cfg);
	return 123;
}