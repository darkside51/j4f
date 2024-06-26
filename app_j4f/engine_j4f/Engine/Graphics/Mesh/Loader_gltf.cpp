#include "Loader_gltf.h"
#include "../../Core/Engine.h"
#include "../../Core/AssetManager.h"
#include "../../File/FileManager.h"
#include "../../Utils/base64.h"
#include "../../Utils/Json/Json.h"

#include <cmath>
#include <assert.h>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace gltf {

	SamplerFilter Parser::parseFilter(const uint16_t f) {
		switch (f) {
		case 9728:
			return SamplerFilter::NEAREST;
			break;
		case 9729:
			return SamplerFilter::LINEAR;
			break;
		case 9984:
			return SamplerFilter::NEAREST_MIPMAP_NEAREST;
			break;
		case 9985:
			return SamplerFilter::LINEAR_MIPMAP_NEAREST;
			break;
		case 9986:
			return SamplerFilter::NEAREST_MIPMAP_LINEAR;
			break;
		case 9987:
			return SamplerFilter::LINEAR_MIPMAP_LINEAR;
			break;
		default:
			return SamplerFilter::LINEAR;
			break;
		}
	};

	SamplerWrap Parser::parseWrap(const uint16_t w) {
		switch (w) {
		case 33071:
			return SamplerWrap::CLAMP_TO_EDGE;
			break;
		case 33648:
			return SamplerWrap::MIRRORED_REPEAT;
			break;
		default:
			return SamplerWrap::REPEAT;
			break;
		}
	};

	void Parser::parseScene(Scene& scene, const Json& js) {
		scene.name = js.value("name", "");
		auto nodesJs = js.find("nodes");
		if (nodesJs != js.end()) {
			scene.nodes = nodesJs->get<std::vector<uint16_t>>();
		}
	}

	vec4 mat4_to_vec4_rotation(const mat4& m) {
		const float fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
		const float fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
		const float fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
		const float fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

		uint8_t biggestIndex = 0u;
		float fourBiggestSquaredMinus1 = fourWSquaredMinus1;

		if (fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
			fourBiggestSquaredMinus1 = fourXSquaredMinus1;
			biggestIndex = 1u;
		}

		if (fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
			fourBiggestSquaredMinus1 = fourYSquaredMinus1;
			biggestIndex = 2u;
		}

		if (fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
			fourBiggestSquaredMinus1 = fourZSquaredMinus1;
			biggestIndex = 3u;
		}

		const float biggestVal = sqrtf(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
		const float mult = 0.25f / biggestVal;

		vec4 r;
		switch (biggestIndex) {
			case 0u:
			{
				r.w = biggestVal;
				r.x = (m[1][2] - m[2][1]) * mult;
				r.y = (m[2][0] - m[0][2]) * mult;
				r.z = (m[0][1] - m[1][0]) * mult;
			}
				break;
			case 1u:
			{
				r.w = (m[1][2] - m[2][1]) * mult;
				r.x = biggestVal;
				r.y = (m[0][1] + m[1][0]) * mult;
				r.z = (m[2][0] + m[0][2]) * mult;
			}
				break;
			case 2u:
			{
				r.w = (m[2][0] - m[0][2]) * mult;
				r.x = (m[0][1] + m[1][0]) * mult;
				r.y = biggestVal;
				r.z = (m[1][2] + m[2][1]) * mult;
			}
				break;
			case 3u:
			{
				r.w = (m[0][1] - m[1][0]) * mult;
				r.x = (m[2][0] + m[0][2]) * mult;
				r.y = (m[1][2] + m[2][1]) * mult;
				r.z = biggestVal;
			}
				break;
			default:
				assert(false);
				break;
		}

		return r;
	}

	void Parser::parseNode(Node& node, const Json& js) {
		node.name = js.value("name", "");
		node.mesh = js.value("mesh", 0xffff);
		node.skin = js.value("skin", 0xffff);

		auto weightsJs = js.find("weights");
		if (weightsJs != js.end()) {
			node.weights = weightsJs->get<std::vector<float>>();
		}

		auto childrenJs = js.find("children");
		if (childrenJs != js.end()) {
			node.children = childrenJs->get<std::vector<uint16_t>>();
		}

		auto matrixJs = js.find("matrix");
		if (matrixJs != js.end()) {
			// decompose matrix to TRS
			mat4 matrix;
			matrix[0][0] = (*matrixJs)[0]; matrix[0][1] = (*matrixJs)[1]; matrix[0][2] = (*matrixJs)[2]; matrix[0][3] = (*matrixJs)[3];
			matrix[1][0] = (*matrixJs)[4]; matrix[1][1] = (*matrixJs)[5]; matrix[1][2] = (*matrixJs)[6]; matrix[1][3] = (*matrixJs)[7];
			matrix[2][0] = (*matrixJs)[8]; matrix[2][1] = (*matrixJs)[9]; matrix[2][2] = (*matrixJs)[10]; matrix[2][3] = (*matrixJs)[11];
			matrix[3][0] = (*matrixJs)[12]; matrix[3][1] = (*matrixJs)[13]; matrix[3][2] = (*matrixJs)[14]; matrix[3][3] = (*matrixJs)[15];

			node.scale.x = sqrtf(matrix[0][0] * matrix[0][0] + matrix[0][1] * matrix[0][1] + matrix[0][2] * matrix[0][2]);
			node.scale.y = sqrtf(matrix[1][0] * matrix[1][0] + matrix[1][1] * matrix[1][1] + matrix[1][2] * matrix[1][2]);
			node.scale.z = sqrtf(matrix[2][0] * matrix[2][0] + matrix[2][1] * matrix[2][1] + matrix[2][2] * matrix[2][2]);

			node.translation.x = matrix[3][0];
			node.translation.y = matrix[3][1];
			node.translation.z = matrix[3][2];

			matrix[0][0] /= node.scale.x;
			matrix[0][1] /= node.scale.x;
			matrix[0][2] /= node.scale.x;
			matrix[0][3] /= node.scale.x;

			matrix[1][0] /= node.scale.y;
			matrix[1][1] /= node.scale.y;
			matrix[1][2] /= node.scale.y;
			matrix[1][3] /= node.scale.y;

			matrix[2][0] /= node.scale.z;
			matrix[2][1] /= node.scale.z;
			matrix[2][2] /= node.scale.z;
			matrix[2][3] /= node.scale.z;

			node.rotation = mat4_to_vec4_rotation(matrix);
			return;
		} 

		auto scaleJs = js.find("scale");
		if (scaleJs != js.end()) {
			node.scale.x = (*scaleJs)[0];
			node.scale.y = (*scaleJs)[1];
			node.scale.z = (*scaleJs)[2];
		}

		auto rotationJs = js.find("rotation");
		if (rotationJs != js.end()) {
			node.rotation.x = (*rotationJs)[0];
			node.rotation.y = (*rotationJs)[1];
			node.rotation.z = (*rotationJs)[2];
			node.rotation.w = (*rotationJs)[3];
		}

		auto translationJs = js.find("translation");
		if (translationJs != js.end()) {
			node.translation.x = (*translationJs)[0];
			node.translation.y = (*translationJs)[1];
			node.translation.z = (*translationJs)[2];
		}
	}

	void Parser::parseMesh(Mesh& mesh, const Json& js, const map_type<std::string, AttributesSemantic>& semantics) {
		mesh.name = js.value("name", "");

		auto weightsJs = js.find("weights");
		if (weightsJs != js.end()) {
			mesh.weights = weightsJs->get<std::vector<float>>();
		}

		auto&& primitivesJs = js["primitives"];
		const size_t primivesCount = primitivesJs.size();
		mesh.primitives.resize(primivesCount);
		for (size_t i = 0; i < primivesCount; ++i) {
			mesh.primitives[i].indices = primitivesJs[i].value("indices", 0xffff);
			mesh.primitives[i].material = primitivesJs[i].value("material", 0xffff);
			const uint8_t primitivesMode = primitivesJs[i].value("mode", 4);
			mesh.primitives[i].mode = static_cast<PrimitiveMode>(primitivesMode);

			auto&& attributes = primitivesJs[i]["attributes"];
			for (auto it = attributes.begin(); it != attributes.end(); ++it) {
				auto its = semantics.find(it.key());
				if (its != semantics.end()) {
					mesh.primitives[i].attributes.emplace(its->second, it.value().get<uint16_t>());
				}
			}
		}
	}

	void Parser::parseBuffer(Buffer& buffer, const Json& js, const std::string& folder, char* binData) {
		buffer.name = js.value("name", "");
		buffer.byteLength = js["byteLength"].get<uint32_t>();
		const std::string& uri = js.value("uri", "");

		if (!uri.empty()) {

			const std::string octetStreamHeader = "data:application/octet-stream;base64,";
			const std::string bufferHeader = "data:application/gltf-buffer;base64,";

			if (uri.find(octetStreamHeader) == 0) {
				const std::string data = base64_decode(uri.substr(octetStreamHeader.size()));
				buffer.data = new char[buffer.byteLength + 1];
				std::memmove(buffer.data, data.data(), buffer.byteLength);
				buffer.data[buffer.byteLength] = '\0';
			} else if (uri.find(bufferHeader) == 0) {
				const std::string data = base64_decode(uri.substr(bufferHeader.size()));
				buffer.data = new char[buffer.byteLength + 1];
				std::memmove(buffer.data, data.data(), buffer.byteLength);
				buffer.data[buffer.byteLength] = '\0';
			} else { // is file path
				auto & fm = engine::Engine::getInstance().getModule<engine::FileManager>();
				size_t fsize;
				buffer.data = fm.readFile((folder + uri), fsize);
			}
		} else if (binData) {
			buffer.data = new char[buffer.byteLength + 1];
			memcpy(buffer.data, binData, buffer.byteLength);
			buffer.data[buffer.byteLength] = '\0';
		}
	}
	
	void Parser::parseBufferView(BufferView& bufferView, const Json& js) {
		bufferView.name = js.value("name", "");
		bufferView.buffer = js["buffer"].get<uint16_t>();
		bufferView.length = js["byteLength"].get<uint32_t>();
		bufferView.stride = js.value("byteStride", 0xffff);
		bufferView.offset = js.value("byteOffset", 0);
		const uint16_t target = js.value("target", 0);
		switch (target) {
		case 34962:
			bufferView.target = BufferTarget::ARRAY_BUFFER;
			break;
		case 34963:
			bufferView.target = BufferTarget::ELEMENT_ARRAY_BUFFER;
			break;
		default:
			break;
		}
	}

	void Parser::parseAccessor(Accessor& accessor, const Json& js, const map_type<std::string, AccessorType>& accesorTypes) {
        accessor.name = js.value("name", "");
        accessor.bufferView = js.value("bufferView", 0xffff);
        accessor.count = js["count"].get<uint32_t>();
        accessor.offset = js.value("byteOffset", 0);
        accessor.normalized = js.value("normalized", false);

		const uint16_t componentType = js["componentType"].get<uint16_t>();
		switch (componentType) {
		case 5120:
            accessor.componentType = AccessorComponentType::BYTE;
			break;
		case 5121:
            accessor.componentType = AccessorComponentType::UNSIGNED_BYTE;
			break;
		case 5122:
            accessor.componentType = AccessorComponentType::SHORT;
			break;
		case 5123:
            accessor.componentType = AccessorComponentType::UNSIGNED_SHORT;
			break;
		case 5125:
            accessor.componentType = AccessorComponentType::UNSIGNED_INT;
			break;
		case 5126:
            accessor.componentType = AccessorComponentType::FLOAT;
			break;
		default:
			break;
		}

		const std::string& type = js["type"].get<std::string>();
        accessor.type = accesorTypes.at(type);

		if (auto minJs = js.find("min"); minJs != js.end()) {
            accessor.min = minJs->get<std::vector<float>>();
		}

		if (auto maxJs = js.find("max"); maxJs != js.end()) {
            accessor.max = maxJs->get<std::vector<float>>();
		}

		if (auto sparseJs = js.find("sparse"); sparseJs != js.end()) {
            accessor.sparse.emplace();
            accessor.sparse->count = sparseJs->at("count").get<uint32_t>();

			const Json& indices = sparseJs->at("indices");
            accessor.sparse->indices.bufferView = indices["bufferView"].get<uint16_t>();
            accessor.sparse->indices.offset = indices.value("byteOffset", 0);
			const uint16_t componentType = indices["componentType"].get<uint16_t>();
			switch (componentType) {
			case 5121:
                accessor.sparse->indices.componentType = AccessorComponentType::UNSIGNED_BYTE;
				break;
			case 5123:
                accessor.sparse->indices.componentType = AccessorComponentType::UNSIGNED_SHORT;
				break;
			case 5125:
                accessor.sparse->indices.componentType = AccessorComponentType::UNSIGNED_INT;
				break;
			default:
				break;
			}

			const Json& values = sparseJs->at("values");
            accessor.sparse->values.bufferView = values["bufferView"].get<uint16_t>();
            accessor.sparse->values.offset = values.value("byteOffset", 0);
		}
	}

	void Parser::parseAnimation(Animation& animation, const Json& js, const map_type<std::string, AnimationChannelPath>& animChannelTypes, const map_type<std::string, Interpolation>& interpolationTypes) {
		animation.name = js.value("name", "");
		// channels
		const Json& channels = js["channels"];
		const size_t channeslCount = channels.size();
		animation.channels.resize(channeslCount);
		for (size_t i = 0; i < channeslCount; ++i) {
			AnimationChannel& channel = animation.channels[i];
			const Json& channelJs = channels[i];
			channel.sampler = channelJs["sampler"].get<uint16_t>();
			const Json& target = channelJs["target"];
			channel.target_node = target.value("node", 0xffff);
			const std::string& path = target["path"].get<std::string>();
			channel.path = animChannelTypes.at(path);
		}
		// samplers
		const Json& samplers = js["samplers"];
		const size_t samplersCount = samplers.size();
		animation.samplers.resize(samplersCount);
		for (size_t i = 0; i < samplersCount; ++i) {
			AnimationSampler& sampler = animation.samplers[i];
			const Json& samplerJs = samplers[i];
			sampler.input = samplerJs["input"].get<uint16_t>();
			sampler.output = samplerJs["output"].get<uint16_t>();
			sampler.interpolation = interpolationTypes.at(samplerJs.value("interpolation", "LINEAR"));
		}
	}

	void Parser::parseSkin(Skin& skin, const Json& js) {
		skin.name = js.value("name", "");
		skin.inverseBindMatrices = js.value("inverseBindMatrices", 0xffff);
		skin.skeleton = js.value("skeleton", 0xffff);
		skin.joints = js["joints"].get<std::vector<uint16_t>>();
	}

	void Parser::parseSampler(Sampler& sampler, const Json& js) {
		sampler.magFilter = parseFilter(js.value("magFilter", 0xffff));
		sampler.minFilter = parseFilter(js.value("minFilter", 0xffff));
		sampler.wrapS = parseWrap(js.value("wrapS", 10497));
		sampler.wrapT = parseWrap(js.value("wrapT", 10497));
	}

	void Parser::parseTexture(Texture& texture, const Json& js) {
		texture.name = js.value("name", "");
		texture.sampler = js.value("sampler", 0xffff);
		texture.source = js.value("source", 0xffff);
	}

	void Parser::parseImage(Image& image, const Json& js, const map_type<std::string, MimeType>& mimeTypes) {
		image.name = js.value("name", "");
		image.uri = js.value("uri", "");
		if (auto mimeTypeJs = js.find("mimeType"); mimeTypeJs != js.end()) {
			image.mimeType = mimeTypes.at(mimeTypeJs->get<std::string>());
		}
		image.bufferView = js.value("bufferView", 0xffff);
	}

	void Parser::parseTextureInfo(TextureInfo& info, const Json& js) {
		info.index = js["index"].get<uint16_t>();
		info.texCoord = js.value("texCoord", 0);
		info.scale = js.value("scale", 1.0f);
		info.strength = js.value("strength", 1.0f);
	}

	void Parser::parseMaterial(Material& material, const Json& js, const map_type<std::string, AlphaMode>& alphaModes) {
		material.name = js.value("name", "");
		material.doubleSided = js.value("doubleSided", false);
		material.alphaCutoff = js.value("alphaCutoff", 0.5f);
		material.alphaMode = alphaModes.at(js.value("alphaMode", "OPAQUE"));

		if (auto pbrMetallicJs = js.find("pbrMetallicRoughness"); pbrMetallicJs != js.end()) {
			if (auto baseColorJs = pbrMetallicJs->find("baseColorFactor"); baseColorJs != pbrMetallicJs->end()) {
				material.pbrMetallicRoughness.baseColorFactor.x = baseColorJs->at(0);
				material.pbrMetallicRoughness.baseColorFactor.y = baseColorJs->at(1);
				material.pbrMetallicRoughness.baseColorFactor.z = baseColorJs->at(2);
				material.pbrMetallicRoughness.baseColorFactor.w = baseColorJs->at(3);
			}

			material.pbrMetallicRoughness.metallicFactor = pbrMetallicJs->value("metallicFactor", 1.0f);
			material.pbrMetallicRoughness.roughnessFactor = pbrMetallicJs->value("roughnessFactor", 1.0f);
			if (auto baseColorTextureJs = pbrMetallicJs->find("baseColorTexture"); baseColorTextureJs != pbrMetallicJs->end()) {
				parseTextureInfo(material.pbrMetallicRoughness.baseColorTexture, *baseColorTextureJs);
			}

			if (auto metallicRoughnessTextureJs = pbrMetallicJs->find("metallicRoughnessTexture"); metallicRoughnessTextureJs != pbrMetallicJs->end()) {
				parseTextureInfo(material.pbrMetallicRoughness.metallicRoughnessTexture, *metallicRoughnessTextureJs);
			}
		}

		if (auto normalTextureJs = js.find("normalTexture"); normalTextureJs != js.end()) {
			parseTextureInfo(material.normalTexture, *normalTextureJs);
		}

		if (auto occlusionTextureJs = js.find("occlusionTexture"); occlusionTextureJs != js.end()) {
			parseTextureInfo(material.normalTexture, *occlusionTextureJs);
		}

		if (auto emissiveTextureJs = js.find("emissiveTexture"); emissiveTextureJs != js.end()) {
			parseTextureInfo(material.emissiveTexture, *emissiveTextureJs);
		}

		if (auto emissiveFactorJs = js.find("emissiveFactor"); emissiveFactorJs != js.end()) {
			material.emissiveFactor.x = emissiveFactorJs->at(0);
			material.emissiveFactor.y = emissiveFactorJs->at(1);
			material.emissiveFactor.z = emissiveFactorJs->at(2);
		}
	}

	template <typename T, typename F, typename... Args>
	static void parseArray(std::vector<T>& arr, F&& f, std::string_view name, const Json& js, Args&&... args) {
		if (auto targetJs = js.find(name); targetJs != js.end()) {
			const size_t count = targetJs->size();
			arr.resize(count);
			for (size_t i = 0u; i < count; ++i) {
				f(arr[i], (*targetJs)[i], std::forward<Args>(args)...);
			}
		}
	}

	Layout Parser::loadModel(const std::string& file) {
		using namespace std::literals;
		std::string folder;
		const size_t lastDelimeter = file.find_last_of('/');
		if (lastDelimeter != std::string::npos) {
			folder = file.substr(0, lastDelimeter + 1);
		}

		/*std::string ext;
		const size_t lastDot = file.find_last_of('.');
		if (lastDot != std::string::npos) {
			ext = file.substr(lastDot + 1, file.length() - lastDot - 1);
		}*/

		Json js;

		char* binData = nullptr;
		size_t binSize = 0u;
		std::vector<char> bytes;
		
		//
		if (file.ends_with(".glb"sv)) {
			auto& fm = engine::Engine::getInstance().getModule<engine::FileManager>();
			fm.readFile(file, bytes);

			if (bytes[0u] == 'g' && bytes[1u] == 'l' && bytes[2u] == 'T' && bytes[3u] == 'F') {
			} else {
				return {};
			}

			uint32_t version;        // 4 bytes
			uint32_t length;         // 4 bytes
			uint32_t chunk0_length;  // 4 bytes
			uint32_t chunk0_format;  // 4 bytes;

			auto const swap4IfBigEndian = [](uint32_t & val) noexcept {
				if (engine::Engine::getInstance().endian() == engine::Engine::Endian::LittleEndian) {
					return;
				}
				//for bigendian
				uint32_t tmp = val;
				unsigned char* dst = reinterpret_cast<unsigned char*>(&val);
				unsigned char* src = reinterpret_cast<unsigned char*>(&tmp);

				dst[0u] = src[3u];
				dst[1u] = src[2u];
				dst[2u] = src[1u];
				dst[3u] = src[0u];
			};

			memcpy(&version, &bytes[4u], 4u);
			swap4IfBigEndian(version);
			memcpy(&length, &bytes[8u], 4u); // Total glb size, including header and all chunks.
			swap4IfBigEndian(length);
			memcpy(&chunk0_length, &bytes[12u], 4u);  // JSON data length
			swap4IfBigEndian(chunk0_length);
			memcpy(&chunk0_format, &bytes[16u], 4u);
			swap4IfBigEndian(chunk0_format);

			constexpr uint8_t headerLength = 20u;

			uint64_t header_and_json_size = headerLength + uint64_t(chunk0_length);

			// parse bin data
			uint32_t chunk1_length = 0u;  // 4 bytes
			uint32_t chunk1_format = 0u;  // 4 bytes;
			memcpy(&chunk1_length, bytes.data() + header_and_json_size, 4u);  // bin data length
			swap4IfBigEndian(chunk1_length);
			memcpy(&chunk1_format, bytes.data() + header_and_json_size + 4u, 4u);
			swap4IfBigEndian(chunk1_format);

			binData = bytes.data() + header_and_json_size + 8u;  // 4 bytes (bin_buffer_length) + 4 bytes(bin_buffer_format)
			binSize = chunk1_length;

			js = Json::parse(&bytes[headerLength], &bytes[headerLength] + chunk0_length);
		} else {
			engine::JsonLoadingParams jsParams(file);
			js = engine::Engine::getInstance().getModule<engine::AssetManager>().loadAsset<Json>(jsParams);
		}

		const static map_type<std::string, AttributesSemantic> semantics = {
			{"POSITION", AttributesSemantic::POSITION},
			{"NORMAL", AttributesSemantic::NORMAL},
			{"TANGENT", AttributesSemantic::TANGENT},
			{"COLOR", AttributesSemantic::COLOR},
			{"JOINTS_0", AttributesSemantic::JOINTS},
			{"WEIGHTS_0", AttributesSemantic::WEIGHT},
			{"TEXCOORD_0", AttributesSemantic::TEXCOORD_0},
			{"TEXCOORD_1", AttributesSemantic::TEXCOORD_1},
			{"TEXCOORD_2", AttributesSemantic::TEXCOORD_2},
			{"TEXCOORD_3", AttributesSemantic::TEXCOORD_3},
			{"TEXCOORD_4", AttributesSemantic::TEXCOORD_4}
		};

		const static map_type<std::string, AccessorType> accesorTypes = {
			{"SCALAR", AccessorType::SCALAR},
			{"VEC2", AccessorType::VEC2},
			{"VEC3", AccessorType::VEC3},
			{"VEC4", AccessorType::VEC4},
			{"MAT2", AccessorType::MAT2},
			{"MAT3", AccessorType::MAT3},
			{"MAT4", AccessorType::MAT4}
		};

		const static map_type<std::string, AnimationChannelPath> animChannelTypes = {
			{"translation", AnimationChannelPath::TRANSLATION},
			{"rotation", AnimationChannelPath::ROTATION},
			{"scale", AnimationChannelPath::SCALE},
			{"weights", AnimationChannelPath::WEIGHTS}
		};

		const static map_type<std::string, Interpolation> interpolationTypes = {
			{"LINEAR", Interpolation::LINEAR},
			{"STEP", Interpolation::STEP},
			{"CUBICSPLINE", Interpolation::CUBICSPLINE}
		};

		const static map_type<std::string, MimeType> mimeTypes = {
			{"image/jpeg", MimeType::JPEG},
			{"image/png", MimeType::PNG}
		};

		const static map_type<std::string, AlphaMode> alphaModes = {
			{"OPAQUE", AlphaMode::A_OPAQUE},
			{"BLEND", AlphaMode::A_BLEND},
			{"MASK", AlphaMode::A_MASK}
		};

		Layout layout;
		layout.asset.version = std::stof(js["asset"]["version"].get<std::string>());
		layout.scene = js["scene"].get<uint16_t>();

		parseArray(layout.scenes,		parseScene,			"scenes",		js);
		parseArray(layout.nodes,		parseNode,			"nodes",		js);
		parseArray(layout.meshes,		parseMesh,			"meshes",		js, semantics);
		parseArray(layout.buffers,		parseBuffer,		"buffers",		js, folder, binData);
		parseArray(layout.bufferViews,	parseBufferView,	"bufferViews",	js);
		parseArray(layout.accessors,	parseAccessor,		"accessors",	js, accesorTypes);
		parseArray(layout.animations,	parseAnimation,		"animations",	js, animChannelTypes, interpolationTypes);
		parseArray(layout.skins,		parseSkin,			"skins",		js);
		parseArray(layout.samplers,		parseSampler,		"samplers",		js);
		parseArray(layout.textures,		parseTexture,		"textures",		js);
		parseArray(layout.images,		parseImage,			"images",		js, mimeTypes);
		parseArray(layout.materials,	parseMaterial,		"materials",	js, alphaModes);

		return layout;
	}
}