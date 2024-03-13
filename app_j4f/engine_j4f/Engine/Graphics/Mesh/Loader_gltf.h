#pragma once

#include "../../Utils/Json/json.hpp"

#include <array>
#include <vector>
#include <optional>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>

// gltf 2.0 format specification
// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.pdf

namespace gltf {
	using Json = nlohmann::json;

	template <typename K, typename T>
	using map_type = std::unordered_map<K, T>;

	struct vec3 {
		union {
			struct {
				float x, y, z;
			};
			std::array<float, 3> data;
		};

		vec3() : x(0.0f), y(0.0f), z(0.0f) {}
		vec3(const float a, const float b, const float c) : x(a), y(b), z(c) {}

		inline float operator[](const uint8_t i) const { return data[i]; }
		inline float& operator[](const uint8_t i) { return data[i]; }
	};

	struct vec4 {
		union {
			struct {
				float x, y, z, w;
			};
			std::array<float, 4> data;
		};

		vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {};
		vec4(const float a, const float b, const float c, const float d) : x(a), y(b), z(c), w(d) {}

		inline float operator[](const uint8_t i) const { return data[i]; }
		inline float& operator[](const uint8_t i) { return data[i]; }
	};

	struct mat4 {
		std::array<vec4, 4> data;

		mat4() = default;
		mat4(const float f) : data {
									vec4(f, 0.0f, 0.0f, 0.0f),
									vec4(0.0f, f, 0.0f, 0.0f),
									vec4(0.0f, 0.0f, f, 0.0f),
									vec4(0.0f, 0.0f, 0.0f, f)
		} {}

		inline vec4 operator[](const uint8_t i) const { return data[i]; }
		inline vec4& operator[](const uint8_t i) { return data[i]; }
	};

	enum class AccessorComponentType : uint8_t {
		BYTE = 0u,
		UNSIGNED_BYTE = 1u,
		SHORT = 2u,
		UNSIGNED_SHORT = 3u,
		UNSIGNED_INT = 4u,
		FLOAT = 5u
	};

	enum class AccessorType : uint8_t {
		SCALAR = 0u,
		VEC2 = 1u,
		VEC3 = 2u,
		VEC4 = 3u,
		MAT2 = 4u,
		MAT3 = 5u,
		MAT4 = 6u
	};

	enum class AttributesSemantic : uint8_t {
		POSITION = 0u,
		NORMAL = 1u,
		TANGENT = 2u,
		COLOR = 3u,
		JOINTS = 4u,
		WEIGHT = 5u,
		TEXCOORD_0 = 6u,
		TEXCOORD_1 = 7u,
		TEXCOORD_2 = 8u,
		TEXCOORD_3 = 9u,
		TEXCOORD_4 = 10u,
		SEMANTICS_COUNT = 11u
	};

	enum class BufferTarget : uint8_t {
		ARRAY_BUFFER = 0u,
		ELEMENT_ARRAY_BUFFER = 1u,
		ANY_DATA = 3u
	};

	enum class PrimitiveMode : uint8_t {
		POINTS = 0u,
		LINES = 1u,
		LINE_LOOP = 2u,
		LINE_STRIP = 3u,
		TRIANGLES = 4u,
		TRIANGLE_STRIP = 5u,
		TRIANGLE_FAN = 6u
	};

	enum class AnimationChannelPath : uint8_t {
		TRANSLATION = 0u,
		ROTATION = 1u,
		SCALE = 2u,
		WEIGHTS = 3u
	};

	enum class Interpolation : uint8_t {
		LINEAR = 0u,
		STEP = 1u,
		CUBICSPLINE = 2u
	};

	enum class MimeType : uint8_t {
		JPEG = 0u,
		PNG = 1u
	};

	enum class AlphaMode : uint8_t {
		A_OPAQUE = 0u,
		A_MASK = 1u,
		A_BLEND = 2u
	};

	enum class SamplerFilter : uint8_t {
		NEAREST = 0u,
		LINEAR = 1u,
		NEAREST_MIPMAP_NEAREST = 2u,
		LINEAR_MIPMAP_NEAREST = 3u,
		NEAREST_MIPMAP_LINEAR = 4u,
		LINEAR_MIPMAP_LINEAR = 5u
	};

	enum class SamplerWrap : uint8_t {
		CLAMP_TO_EDGE = 0u,
		MIRRORED_REPEAT = 1u,
		REPEAT = 2u
	};

	struct Scene {
		std::string name;
		std::vector<uint16_t> nodes;
	};

	struct Node {
		std::string name;
		uint16_t mesh = 0xffffu;
		uint16_t skin = 0xffffu;
		vec3 scale = {1.0f, 1.0f, 1.0f};
		vec3 translation = {0.0f, 0.0f, 0.0f};
		vec4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
		std::vector<float> weights;
		std::vector<uint16_t> children;
	};

	struct Primitives {
		std::map<AttributesSemantic, uint16_t> attributes;
		uint16_t indices;
		uint16_t material = 0xffffu;
		PrimitiveMode mode = PrimitiveMode::TRIANGLES;
	};

	struct Mesh {
		std::string name;
		std::vector<Primitives> primitives;
		std::vector<float> weights;
	};

	struct Buffer {
		std::string name;
		char* data = nullptr;
		uint32_t byteLength;

		~Buffer() {
			if (data) {
				delete data;
				data = nullptr;
			}
		}

		Buffer() = default;
		Buffer(Buffer&& b) noexcept : name(std::move(b.name)), data(b.data), byteLength(b.byteLength) {
			b.data = nullptr;
		}

        Buffer& operator=(Buffer&& b) noexcept {
			name = std::move(b.name);
			data = b.data;
			byteLength = b.byteLength;
			b.data = nullptr;
			return *this;
		}

		Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
	};

	struct BufferView {
		std::string name;
		uint16_t buffer;
		uint16_t stride = 0xffffu;
		uint32_t offset = 0u;
		uint32_t length;
		BufferTarget target = BufferTarget::ANY_DATA;
	};

	struct AccessorSparse {
		uint32_t count;
		struct {
			uint16_t bufferView;
			uint32_t offset = 0u;
			AccessorComponentType componentType; // may be UNSIGNED_BYTE or UNSIGNED_SHORT or UNSIGNED_INT
		} indices;

		struct {
			uint16_t bufferView;
			uint32_t offset = 0u;
		} values;
	};

	struct Accessor {
		std::string name;
		uint16_t bufferView = 0xffffu;
		uint32_t count;
		uint32_t offset = 0u;
		AccessorComponentType componentType;
		bool normalized = false;
		AccessorType type;
		std::vector<float> min; // [1 - 16] it can be 1, 2, 3, 4, 9, or 16
		std::vector<float> max; // [1 - 16] it can be 1, 2, 3, 4, 9, or 16
		std::optional<AccessorSparse> sparse = std::nullopt; // no required

		~Accessor() = default;

		Accessor() = default;
		Accessor(Accessor&& a) noexcept :
			name(std::move(a.name)), sparse(std::move(a.sparse)), bufferView(a.bufferView),
			count(a.count), offset(a.offset), componentType(a.componentType),
			normalized(a.normalized), type(a.type), min(a.min), max(a.max) {

		}

        Accessor& operator=(Accessor&& a) noexcept = default;
		Accessor(const Accessor&) = delete;
        Accessor& operator=(const Accessor&) = delete;
	};

	struct Asset {
		float version = 0.0f;
	};

	struct AnimationSampler {
		uint16_t input = 0u;
		uint16_t output = 0u;
		Interpolation interpolation = Interpolation::LINEAR;
	};

	struct AnimationChannel {
		uint16_t sampler = 0u;
		uint16_t target_node = 0u;
		AnimationChannelPath path = AnimationChannelPath::TRANSLATION;
	};

	struct Animation {
		std::string name;
		std::vector<AnimationChannel> channels;
		std::vector<AnimationSampler> samplers;
	};

	struct Image {
		std::string name;
		std::string uri;
		MimeType mimeType;
		uint16_t bufferView;
	};

	struct TextureInfo {
		uint16_t index = 0xffffu;
		uint16_t texCoord;
		float scale = 1.0f; // for normalTexture
		float strength = 1.0f; // for occolusionTexture
	};

	struct Material {
		std::string name;
		struct {
			vec4 baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
			TextureInfo baseColorTexture;
			TextureInfo metallicRoughnessTexture;
			float metallicFactor = 1.0f;
			float roughnessFactor = 1.0f;
		} pbrMetallicRoughness;

		TextureInfo normalTexture;
		TextureInfo occlusionTexture;
		TextureInfo emissiveTexture;
		vec3 emissiveFactor = {0.0f, 0.0f, 0.0f};
		AlphaMode alphaMode = AlphaMode::A_OPAQUE;
		float alphaCutoff = 0.5f;
		bool doubleSided = false;
	};

	struct Sampler {
		SamplerFilter magFilter = SamplerFilter::LINEAR;
		SamplerFilter minFilter = SamplerFilter::LINEAR;
		SamplerWrap wrapS = SamplerWrap::REPEAT;
		SamplerWrap wrapT = SamplerWrap::REPEAT;
	};

	struct Skin {
		std::string name;
		uint16_t inverseBindMatrices = 0xffffu; // The index of the accessor containing the floating-point 4x4 inverse-bind matrices.
		uint16_t skeleton = 0xffffu; // The index of the node used as a skeleton root.
		std::vector<uint16_t> joints; // Indices of skeleton nodes, used as joints in this skin.
	};

	struct Texture {
		std::string name;
		uint16_t sampler = 0xffffu;
		uint16_t source;
	};

	struct Layout {
		std::vector<Accessor> accessors;
		std::vector<Animation> animations;
		std::vector<Buffer> buffers;
		std::vector<BufferView> bufferViews;
		std::vector<Image> images;
		std::vector<Material> materials;
		std::vector<Mesh> meshes;
		std::vector<Node> nodes;
		std::vector<Sampler> samplers;
		std::vector<Scene> scenes;
		std::vector<Skin> skins;
		std::vector<Texture> textures;
		uint16_t scene;
		Asset asset;

		Layout() = default;
		Layout(Layout&&) noexcept = default;
        Layout& operator=(Layout&& d) noexcept {
			accessors = std::move(d.accessors);
			animations = std::move(d.animations);
			buffers = std::move(d.buffers);
			bufferViews = std::move(d.bufferViews);
			images = std::move(d.images);
			materials = std::move(d.materials);
			meshes = std::move(d.meshes);
			nodes = std::move(d.nodes);
			samplers = std::move(d.samplers);
			scenes = std::move(d.scenes);
			skins = std::move(d.skins);
			textures = std::move(d.textures);
			scene = d.scene;
			asset = std::move(d.asset);
			return *this;
		}

		Layout(const Layout&) = delete;
        Layout& operator=(const Layout&) = delete;
	};

	class Parser {
		static SamplerFilter parseFilter(const uint16_t f);
		static SamplerWrap parseWrap(const uint16_t w);
		static void parseScene(Scene& scene, const Json& js);
		static void parseNode(Node& node, const Json& js);
		static void parseMesh(Mesh& mesh, const Json& js, const map_type<std::string, AttributesSemantic>& semantics);
		static void parseBuffer(Buffer& buffer, const Json& js, const std::string& folder, char* binData);
		static void parseBufferView(BufferView& bufferView, const Json& js);
		static void parseAccessor(Accessor& accessor, const Json& js, const map_type<std::string, AccessorType>& accesorTypes);
		static void parseAnimation(Animation& animation, const Json& js, const map_type<std::string, AnimationChannelPath>& animChannelTypes, const map_type<std::string, Interpolation>& interpolationTypes);
		static void parseSkin(Skin& skin, const Json& js);
		static void parseSampler(Sampler& sampler, const Json& js);
		static void parseTexture(Texture& texture, const Json& js);
		static void parseImage(Image& image, const Json& js, const map_type<std::string, MimeType>& mimeTypes);
		static void parseTextureInfo(TextureInfo& info, const Json& js);
		static void parseMaterial(Material& material, const Json& js, const map_type<std::string, AlphaMode>& alphaModes);

	public:
		static Layout loadModel(const std::string& file);
	};
}