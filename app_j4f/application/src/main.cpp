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
#include <Engine/Core/Math/random.h>
#include <Engine/Core/Threads/FlowTask.h>
#include  <Engine/Core/Threads/Looper.h>

#include <Engine/ECS/Component.h>
#include <Engine/ECS/BitMask.h>

#include <Engine/Graphics/Scene/Shadows/CascadeShadowMap.h>

namespace engine {

	MeshGraphicsDataBuffer* meshesGraphicsBuffer;

	Camera* camera = nullptr;
	Camera* camera2 = nullptr;

	CascadeShadowMap* shadowMap = nullptr;

	Mesh* mesh = nullptr;
	Mesh* mesh2 = nullptr;
	Mesh* mesh3 = nullptr;
	MeshAnimationTree* animTree = nullptr;
	MeshAnimationTree* animTree2 = nullptr;

	vulkan::VulkanTexture* texture_1 = nullptr;
	vulkan::VulkanTexture* texture_text = nullptr;
	vulkan::VulkanTexture* texture_floor = nullptr;

	vulkan::VulkanGpuProgram* program_mesh_default = nullptr;
	vulkan::VulkanGpuProgram* program_mesh_shadow = nullptr;

	VkClearValue clearValues[2];

	RenderList sceneRenderList;

	glm::vec3 wasd(0.0f);

	//////////

	/// cascade shadow map
	constexpr uint8_t SHADOW_MAP_CASCADE_COUNT = 4;
	constexpr uint16_t SHADOWMAP_DIM = 4096;
	glm::vec3 lightPos = glm::vec3(-460.0f, -600.0f, 1000.0f);
	/// cascade shadow map

	class ApplicationCustomData : public InputObserver, public ICameraTransformChangeObserver {
	public:

		void onCameraTransformChanged(const Camera* camera) override {
			shadowMap->updateCascades(camera);
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

			camera->removeObserver(this);

			delete camera;
			delete camera2;

			delete mesh;
			delete mesh2;
			delete mesh3;
			delete animTree;
			delete animTree2;

			delete texture_text;

			delete meshesGraphicsBuffer;

			delete shadowMap;
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

			camera->makeProjection(3.1415926f / 4.0f, static_cast<float>(width) / static_cast<float>(height), 10.0f, 4000.0f);
			//camera->makeOrtho(-float(width) * 0.5f, float(width) * 0.5f, -float(height) * 0.5f, float(height) * 0.5f, 1.0f, 1000.0f);
			camera->setRotation(glm::vec3(-3.1415926f / 3.0f, 0.0f, 0.0f));
			camera->setPosition(glm::vec3(0.0f, -500.0f, 300.0f));

			camera2 = new Camera(width, height);
			camera2->makeOrtho(-float(width) * 0.5f, float(width) * 0.5f, -float(height) * 0.5f, float(height) * 0.5f, 1.0f, 1000.0f);
			camera2->setPosition(glm::vec3(0.0f, 0.0f, 200.0f));

			clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			////////////////////////
			shadowMap = new CascadeShadowMap(SHADOWMAP_DIM, SHADOW_MAP_CASCADE_COUNT, camera->getNearFar(), 200.0f, 2100.0f);
			shadowMap->setLamdas(1.0f, 1.3f, 1.425f);
			shadowMap->setLightPosition(lightPos);
		}

		void create() {
			using namespace vulkan;
			using namespace gltf;

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
			glm::vec2 lightMinMax(0.35f, 1.5f);
			glm::vec4 lightColor(1.0f, 1.0f, 1.1f, 1.0f);

			auto l = program_mesh_default->getGPUParamLayoutByName("lightDirection");
			program_mesh_default->setValueToLayout(l, &lightDir, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			auto l2 = program_mesh_default->getGPUParamLayoutByName("lightMinMax");
			program_mesh_default->setValueToLayout(l2, &lightMinMax, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			auto l3 = shadowPlainProgram->getGPUParamLayoutByName("lightMin");
			shadowPlainProgram->setValueToLayout(l3, &lightMinMax.x, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);

			auto l4 = program_mesh_default->getGPUParamLayoutByName("lightColor");
			auto l5 = shadowPlainProgram->getGPUParamLayoutByName("lightColor");
			program_mesh_default->setValueToLayout(l4, &lightColor, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);
			shadowPlainProgram->setValueToLayout(l5, &lightColor, nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, true);


			TextureLoadingParams tex_params;
			//tex_params.file = "resources/assets/models/zombiWarrior/textures/defaultMat_diffuse.png";
			tex_params.file = "resources/assets/models/chaman/textures/Ti-Pche_Mat_baseColor.png";
			tex_params.flags->async = 1;
			tex_params.flags->use_cache = 1;
			auto texture_zombi = assm->loadAsset<vulkan::VulkanTexture*>(tex_params);

			TextureLoadingParams tex_params2;
			//tex_params2.file = "resources/assets/models/zombi/textures/survivor_MAT_diffuse.png";
			tex_params2.file = "resources/assets/models/warcraft3/textures/Armor_2_baseColor.png";
			tex_params2.flags->async = 1;
			tex_params2.flags->use_cache = 1;
			auto texture_v = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);
			tex_params2.file = "resources/assets/models/warcraft3/textures/body_baseColor.png";
			auto texture_v2 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);
			tex_params2.file = "resources/assets/models/warcraft3/textures/Metal_baseColor.png";
			auto texture_v3 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params2);

			meshesGraphicsBuffer = new MeshGraphicsDataBuffer(10 * 1024 * 1024, 10 * 1024 * 1024); // or create with default constructor for unique buffer for mesh

			MeshLoadingParams mesh_params;
			//mesh_params.file = "resources/assets/models/zombiWarrior/scene.gltf";
			mesh_params.file = "resources/assets/models/chaman/scene.gltf";
			mesh_params.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params.latency = 3;
			mesh_params.flags->async = 1;
			mesh_params.graphicsBuffer = meshesGraphicsBuffer;

			MeshLoadingParams mesh_params2;
			//mesh_params2.file = "resources/assets/models/zombi/scene.gltf";
			mesh_params2.file = "resources/assets/models/warcraft3/scene.gltf";
			mesh_params2.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL, AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT, AttributesSemantic::TEXCOORD_0);
			mesh_params2.latency = 3;
			mesh_params2.flags->async = 1;
			mesh_params2.graphicsBuffer = meshesGraphicsBuffer;

			mesh = assm->loadAsset<Mesh*>(mesh_params, [program_gltf, texture_zombi, this](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_zombi, false);

				animTree = new MeshAnimationTree(0.0f, asset->getNodesCount(), asset->getSkeleton()->getLatency());
				animTree->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[2], 1.0f, asset->getSkeleton()->getLatency()));
				animTree->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(&asset->getMeshData()->animations[1], 0.0f, asset->getSkeleton()->getLatency()));

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->onPipelineAttributesChanged();
				});

			mesh2 = assm->loadAsset<Mesh*>(mesh_params, [program_gltf, texture_zombi](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_zombi, false);
				asset->setSkeleton(mesh->getSkeleton());

				asset->renderState().rasterisationState.cullmode = vulkan::CULL_MODE_NONE;
				asset->onPipelineAttributesChanged();
				});

			mesh3 = assm->loadAsset<Mesh*>(mesh_params2, [program_gltf, texture_v, texture_v2, texture_v3](Mesh* asset, const AssetLoadingResult result) {
				asset->setProgram(program_gltf);
				asset->setParamByName("u_texture", texture_v, false);
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
				});

			sceneRenderList.addDescriptor(&mesh->getRenderDescriptor());
			sceneRenderList.addDescriptor(&mesh2->getRenderDescriptor());
			sceneRenderList.addDescriptor(&mesh3->getRenderDescriptor());

			TextureLoadingParams tex_params_logo;
			tex_params_logo.file = "resources/assets/textures/vulkan_logo.png";
			tex_params_logo.flags->async = 1;
			tex_params_logo.flags->use_cache = 1;
			texture_1 = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_logo);

			TextureLoadingParams tex_params_floor;
			tex_params_floor.file = "resources/assets/textures/ground4.jpg";
			tex_params_floor.flags->async = 1;
			tex_params_floor.flags->use_cache = 1;
			texture_floor = assm->loadAsset<vulkan::VulkanTexture*>(tex_params_floor, [](vulkan::VulkanTexture* asset, const AssetLoadingResult result) {
				auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
				texture_floor->setSampler(
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

			FontRenderer fr(256, 256, 100);
			fr.render(f, 18, "abcdefghijklmnopqrstuv", 1, 0, 0xffffffff, 2, 0);
			fr.render(f, 18, "wxyzABCDEFGHIJKLMN", 1, 20, 0xffffffff, 2, 0);
			fr.render(f, 18, "OPQRSTUVWXYZ", 1, 40, 0xffffffff, 2, 0);
			fr.render(f, 18, "0123456789-+*/=", 1, 60, 0xffffffff, 2, 0);
			fr.render(f, 18, "&%#@!?<>,.()[];:@$^~", 1, 80, 0xffffffff, 2, 0);

			texture_text = fr.createFontTexture();
			texture_text->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);

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
			// mix test
			static float mix = 0.7f;
			static bool na = false;
			const float step = 1.5f * delta;
			static uint8_t animNum = 2;
			if (!na) {
				if (mix > -4.0f) {
					mix -= step;
				}
				else {
					mix = -4.0f;
					na = true;
				}
			}
			else {
				if (mix < 5.0f) {
					mix += step;
				}
				else {
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

			auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
			const uint32_t currentFrame = renderer->getCurrentFrame();

			camera->movePosition(250.0f * delta * engine::as_normalized(wasd));

			camera->calculateTransform();
			camera2->calculateTransform();


			if (animTree) {
				mesh->getSkeleton()->updateAnimation(delta, animTree);
			}

			if (animTree2) {
				mesh3->getSkeleton()->updateAnimation(delta, animTree2);
			}

			glm::mat4 wtr(1.0f);
			scaleMatrix(wtr, glm::vec3(0.5f));
			rotateMatrix_xyz(wtr, glm::vec3(1.57f, 0.45f, 0.0f));
			translateMatrixTo(wtr, glm::vec3(-100.0f, -0.0f, 0.0f));

			static float angle = 0.0f;
			angle -= delta;
			glm::mat4 wtr2(1.0f);
			scaleMatrix(wtr2, glm::vec3(0.5f));
			rotateMatrix_xyz(wtr2, glm::vec3(1.57f, -0.45f - angle, 0.0f));
			translateMatrixTo(wtr2, glm::vec3(100.0f, -0.0f, 0.0f));

			glm::mat4 wtr3(1.0f);
			scaleMatrix(wtr3, glm::vec3(30.0f));
			//rotateMatrix_xyz(wtr3, glm::vec3(3.14f, 0.0f, 0.0f));
			directMatrix_yz(wtr3, 0.0f, 1.0f);
			//directMatrix_yz(wtr3, -1.0f, 0.0f);
			//directMatrix_xy(wtr3, 0.0f, 1.0f);
			//directMatrix_xz(wtr3, 0.0f, 1.0f);

			//directMatrix(wtr3, glm::vec3(0.0, 0.0, -1.0));

			translateMatrixTo(wtr3, glm::vec3(0.0f, 0.0f, 0.0f));

			mesh->updateRenderData(wtr);
			mesh2->updateRenderData(wtr2);
			mesh3->updateRenderData(wtr3);

			const uint64_t wh = renderer->getWH();
			const uint32_t width = static_cast<uint32_t>(wh >> 0);
			const uint32_t height = static_cast<uint32_t>(wh >> 32);

			vulkan::VulkanCommandBuffer& commandBuffer = renderer->getRenderCommandBuffer();
			commandBuffer.begin();

			/////// shadow pass
			mesh->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh2->setProgram(program_mesh_shadow, shadowMap->getRenderPass());
			mesh3->setProgram(program_mesh_shadow, shadowMap->getRenderPass());

			//if (animTree2) {
				// у меня лайоуты совпадают у юниформов, для теней => не нужно вызывать переназначение, достаточно одного раза
				//mesh->updateRenderData(wtr);
				//mesh2->updateRenderData(wtr2);
				//mesh3->updateRenderData(wtr3);
			//}

			shadowMap->prepareToRender(commandBuffer);
			for (uint32_t i = 0; i < shadowMap->getCascadesCount(); ++i) {
				shadowMap->beginRenderPass(commandBuffer, i);

				//todo: проверять в какой каскад меш попадает и только там его и рисовать
				//mesh->setCameraMatrix(cascadesViewProjMatrixes[i]);
				//mesh2->setCameraMatrix(cascadesViewProjMatrixes[i]);
				//mesh3->setCameraMatrix(cascadesViewProjMatrixes[i]);

				//mesh->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
				//mesh2->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
				//mesh3->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
				sceneRenderList.render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));

				shadowMap->endRenderPass(commandBuffer);
			}

			mesh->setProgram(program_mesh_default);
			mesh2->setProgram(program_mesh_default);
			mesh3->setProgram(program_mesh_default);
			/////// shadow pass

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

			sceneRenderList.render(commandBuffer, currentFrame, &cameraMatrix);

			////////

			{
				//mesh->drawBoundingBox(cameraMatrix, wtr, commandBuffer, currentFrame);
				//mesh2->drawBoundingBox(cameraMatrix, wtr2, commandBuffer, currentFrame);
				//mesh3->drawBoundingBox(cameraMatrix, wtr3, commandBuffer, currentFrame);
			}

			{ // ortho matrix draw
				const glm::mat4& cameraMatrix2 = camera2->getMatrix();

				glm::mat4 wtr4(1.0f);
				//rotateMatrix_xyz(wtr4, glm::vec3(1.57f, 0.0f, angle));
				scaleMatrix(wtr4, glm::vec3(50.0f));
				rotateMatrix_xyz(wtr4, glm::vec3(0.0, angle, 0.0f));
				translateMatrixTo(wtr4, glm::vec3(-float(width) * 0.5f + 100.0f, float(height) * 0.5f - mesh3->getMaxCorner().y * 50.0f - 50.0f, 0.0f));
				mesh3->draw(cameraMatrix2, wtr4, commandBuffer, currentFrame);

				///////////
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

				auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
				auto&& autoBatcher = renderHelper->getAutoBatchRenderer();

				static auto&& pipeline = renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_TEXTURED);
				static const vulkan::GPUParamLayoutInfo* mvp_layout = pipeline->program->getGPUParamLayoutByName("mvp");

				//// floor
				static auto&& pipeline_shadow_test = CascadeShadowMap::getSpecialPipeline(ShadowMapSpecialPipelines::SH_PIPEINE_PLAIN);
				static const vulkan::GPUParamLayoutInfo* mvp_layout2 = pipeline_shadow_test->program->getGPUParamLayoutByName("mvp");

				const float tc = 12.0f;
				TexturedVertex floorVtx[4] = {
					{ {-1024.0f, -1024.0f, 0.0f},	{0.0f, tc} },
					{ {1024.0f, -1024.0f, 0.0f},	{tc, tc} },
					{ {-1024.0f, 1024.0f, 0.0f},	{0.0f, 0.0f} },
					{ {1024.0f, 1024.0f, 0.0f},		{tc, 0.0f} }
				};

				//vulkan::RenderData renderDataFloor(const_cast<vulkan::VulkanPipeline*>(renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_TEXTURED_DEPTH_RW)));
				//renderDataFloor.setParamForLayout(mvp_layout, &const_cast<glm::mat4&>(cameraMatrix), false);
				//renderDataFloor.setParamByName("u_texture", texture_floor, false);

				vulkan::RenderData renderDataFloor(const_cast<vulkan::VulkanPipeline*>(pipeline_shadow_test));
				renderDataFloor.setParamForLayout(mvp_layout2, &const_cast<glm::mat4&>(cameraMatrix), false);
				const glm::mat4& viewTransform = camera->getViewTransform();
				renderDataFloor.setParamByName("view", &const_cast<glm::mat4&>(viewTransform), false);
				renderDataFloor.setParamByName("u_texture", texture_floor, false);
				renderDataFloor.setParamByName("u_shadow_map", shadowMap->getTexture(), false);
				renderDataFloor.setParamByName("cascade_matrix", shadowMap->getVPMatrixes().data(), false, shadowMap->getCascadesCount());
				renderDataFloor.setParamByName("cascade_splits", shadowMap->getSplitDepthsPointer<glm::vec4>(), false, 1);
				
				autoBatcher->addToDraw(&renderDataFloor, sizeof(TexturedVertex), &floorVtx[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);

				vulkan::RenderData renderData(const_cast<vulkan::VulkanPipeline*>(pipeline));

				glm::mat4 wtr5(1.0f);
				translateMatrixTo(wtr5, glm::vec3(-float(width) * 0.5f + 150.0f, float(height) * 0.5f - 110.0f, 0.0f));

				glm::mat4 transform = cameraMatrix2 * wtr5;
				renderData.setParamForLayout(mvp_layout, &transform, false);
				renderData.setParamByName("u_texture", texture_1, false);

				autoBatcher->addToDraw(renderData.pipeline, sizeof(TexturedVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, renderData.params, commandBuffer, currentFrame);

				for (TexturedVertex& tv : vtx) {
					tv.position[0] += 350.0f;
				}

				autoBatcher->addToDraw(renderData.pipeline, sizeof(TexturedVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, renderData.params, commandBuffer, currentFrame);


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

	////////////////////////////////////////
	engine::EngineConfig cfg;
	engine::Engine::getInstance().init(cfg);
	return 123;
}