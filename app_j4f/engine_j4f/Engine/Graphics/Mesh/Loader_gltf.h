#pragma once

#include "../../Utils/Json/json.hpp"

#include <array>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>

// gltf 2.0 format specification
// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html

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

		vec3() = default;
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

		vec4() = default;
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

	enum class AccessorComponentType {
		BYTE = 0,
		UNSIGNED_BYTE = 1,
		SHORT = 2,
		UNSIGNED_SHORT = 3,
		UNSIGNED_INT = 4,
		FLOAT = 5
	};

	enum class AccessorType {
		SCALAR = 0,
		VEC2 = 1,
		VEC3 = 2,
		VEC4 = 3,
		MAT2 = 4,
		MAT3 = 5,
		MAT4 = 6
	};

	enum class AttributesSemantic : uint8_t {
		POSITION = 0,
		NORMAL = 1,
		TANGENT = 2,
		COLOR = 3,
		JOINTS = 4,
		WEIGHT = 5,
		TEXCOORD_0 = 6,
		TEXCOORD_1 = 7,
		TEXCOORD_2 = 8,
		TEXCOORD_3 = 9,
		TEXCOORD_4 = 10,
		SEMANTICS_COUNT = 11
	};

	enum class BufferTarget : uint8_t {
		ARRAY_BUFFER = 0,
		ELEMENT_ARRAY_BUFFER = 1,
		ANY_DATA = 3
	};

	enum class PrimitiveMode : uint8_t {
		POINTS = 0,
		LINES = 1,
		LINE_LOOP = 2,
		LINE_STRIP = 3,
		TRIANGLES = 4,
		TRIANGLE_STRIP = 5,
		TRIANGLE_FAN = 6
	};

	enum class AimationChannelPath : uint8_t {
		TRANSLATION = 0,
		ROTATION = 1,
		SCALE = 2,
		WEIGHTS = 3
	};

	enum class Interpolation : uint8_t {
		LINEAR = 0,
		STEP = 1,
		CUBICSPLINE = 2
	};

	enum class MimeType : uint8_t {
		JPEG = 0,
		PNG = 1
	};

	enum class AlphaMode : uint8_t {
		A_OPAQUE = 0,
		A_MASK = 1,
		A_BLEND = 2
	};

	enum class SamplerFilter : uint8_t {
		NEAREST = 0,
		LINEAR = 1,
		NEAREST_MIPMAP_NEAREST = 2,
		LINEAR_MIPMAP_NEAREST = 3,
		NEAREST_MIPMAP_LINEAR = 4,
		LINEAR_MIPMAP_LINEAR = 5
	};

	enum class SamplerWrap : uint8_t {
		CLAMP_TO_EDGE = 0,
		MIRRORED_REPEAT = 1,
		REPEAT = 2
	};

	struct Scene {
		std::string name;
		std::vector<uint16_t> nodes;
	};

	struct Node {
		std::string name;
		uint16_t mesh = 0xffff;
		uint16_t skin = 0xffff;
		vec3 scale;
		vec3 translation;
		vec4 rotation;
		std::vector<float> weights;
		std::vector<uint16_t> children;
	};

	struct Primitives {
		std::map<AttributesSemantic, uint16_t> attributes;
		uint16_t indices;
		uint16_t material = 0xffff;
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
		Buffer(Buffer&& b) noexcept : name(std::move(b.name)), data(std::move(b.data)), byteLength(b.byteLength) {
			b.data = nullptr;
		}

		const Buffer& operator=(Buffer&& b) noexcept {
			name = std::move(b.name);
			data = std::move(b.data);
			byteLength = b.byteLength;
			b.data = nullptr;
			return *this;
		}

		Buffer(const Buffer&) = delete;
		const Buffer& operator=(const Buffer&) = delete;
	};

	struct BufferView {
		std::string name;
		uint16_t buffer;
		uint16_t stride = 0xffff;
		uint32_t offset = 0;
		uint32_t length;
		BufferTarget target = BufferTarget::ANY_DATA;
	};

	struct AccessorSparse {
		uint32_t count;
		struct {
			uint16_t bufferView;
			uint32_t offset = 0;
			AccessorComponentType componentType; // may be UNSIGNED_BYTE or UNSIGNED_SHORT or UNSIGNED_INT
		} indices;

		struct {
			uint16_t bufferView;
			uint32_t offset = 0;
		} values;
	};

	struct Accessor {
		std::string name;
		uint16_t bufferView = 0xffff;
		uint32_t count;
		uint32_t offset = 0;
		AccessorComponentType componentType;
		bool normalized = false;
		AccessorType type;
		std::vector<float> min; // [1 - 16] it can be 1, 2, 3, 4, 9, or 16
		std::vector<float> max; // [1 - 16] it can be 1, 2, 3, 4, 9, or 16
		AccessorSparse* sparse = nullptr; // no required

		~Accessor() {
			if (sparse) {
				delete sparse;
				sparse = nullptr;
			}
		}

		Accessor() = default;
		Accessor(Accessor&& a) noexcept :
			name(std::move(a.name)), sparse(std::move(a.sparse)), bufferView(a.bufferView),
			count(a.count), offset(a.offset), componentType(a.componentType),
			normalized(a.normalized), type(a.type), min(a.min), max(a.max)
		{
			a.sparse = nullptr;
		}

		const Accessor& operator=(Accessor&& a) noexcept {
			name = std::move(a.name);
			sparse = std::move(a.sparse);
			bufferView = a.bufferView;
			count = a.count;
			offset = a.offset;
			componentType = a.componentType;
			normalized = a.normalized;
			type = a.type;
			min = a.min;
			max = a.max;
			a.sparse = nullptr;
			return *this;
		}

		Accessor(const Accessor&) = delete;
		const Accessor& operator=(const Accessor&) = delete;
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
		AimationChannelPath path = AimationChannelPath::TRANSLATION;
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
		uint16_t index = 0xffff;
		uint16_t texCoord;
		float scale = 1.0f; // for normalTexture
		float strength = 1.0f; // for occolusionTexture
	};

	struct Material {
		std::string name;
		struct {
			vec4 baseColorFactor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			TextureInfo baseColorTexture;
			TextureInfo metallicRoughnessTexture;
			float metallicFactor = 1.0f;
			float roughnessFactor = 1.0f;
		} pbrMetallicRoughness;

		TextureInfo normalTexture;
		TextureInfo occlusionTexture;
		TextureInfo emissiveTexture;
		vec3 emissiveFactor = vec3(0.0f, 0.0f, 0.0f);
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
		uint16_t inverseBindMatrices = 0xffff; // The index of the accessor containing the floating-point 4x4 inverse-bind matrices.
		uint16_t skeleton = 0xffff; // The index of the node used as a skeleton root.
		std::vector<uint16_t> joints; // Indices of skeleton nodes, used as joints in this skin.
	};

	struct Texture {
		std::string name;
		uint16_t sampler = 0xffff;
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
		const Layout& operator=(Layout&& d) noexcept {
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
		const Layout& operator=(const Layout&) = delete;
	};

	class Parser {
		static SamplerFilter parseFilter(const uint16_t f);
		static SamplerWrap parseWrap(const uint16_t w);
		static void parseScene(Scene& scene, const Json& js);
		static void parseNode(Node& node, const Json& js);
		static void parseMesh(Mesh& mesh, const Json& js, const map_type<std::string, AttributesSemantic>& semantics);
		static void parseBuffer(Buffer& buffer, const Json& js, const std::string& folder);
		static void parseBufferView(BufferView& bufferView, const Json& js);
		static void parseAcessor(Accessor& acessor, const Json& js, const map_type<std::string, AccessorType>& accesorTypes);
		static void parseAnimation(Animation& animation, const Json& js, const map_type<std::string, AimationChannelPath>& animChannelTypes, const map_type<std::string, Interpolation>& interpolationTypes);
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