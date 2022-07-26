#include "TextureData.h"

#include "../../Core/Engine.h"
#include "../../File/FileManager.h"

#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace engine {

	///////////////////////////////////////////////////////////
	inline void getTextureInfo(const char* file, int* w, int* h, int* c) {
		stbi_info(file, w, h, c); // texture dimensions + channels without load
	}

	inline unsigned char* loadImageDataFromBuffer(const unsigned char* buffer, const size_t size, int* w, int* h, int* c) {
		return stbi_load_from_memory(buffer, size, w, h, c, 4);
	}

	inline void freeImageData(unsigned char* img) {
		stbi_image_free(img);
	}
	///////////////////////////////////////////////////////////

	TextureData::TextureData(const std::string& path, const TextureFormatType ft) {
		auto&& engine = Engine::getInstance();
		FileManager* fm = engine.getModule<engine::FileManager>();

		size_t fsize;
		
		if (const char* imgBuffer = fm->readFile(path, fsize)) {
			_data = loadImageDataFromBuffer(reinterpret_cast<const unsigned char*>(imgBuffer), fsize, &_width, &_height, &_channels);
			_bpp = 32; // todo!

			switch (ft) {
				case TextureFormatType::SRGB:
					_format = VK_FORMAT_R8G8B8A8_SRGB; // todo!
					break;
				default: 
					_format = VK_FORMAT_R8G8B8A8_UNORM; // todo! check bits
					break;
			}

			delete imgBuffer;
		}
	}

	TextureData::TextureData(const unsigned char* buffer, const size_t size, const TextureFormatType ft) {
		_data = loadImageDataFromBuffer(buffer, size, &_width, &_height, &_channels);
		_bpp = 32; // todo!
		switch (ft) {
			case TextureFormatType::SRGB:
				_format = VK_FORMAT_R8G8B8A8_SRGB; // todo!
				break;
			default:
				_format = VK_FORMAT_R8G8B8A8_UNORM; // todo! check bits
				break;
		}
	}

	TextureData::~TextureData() {
		if (_data) {
			freeImageData(_data);
			_data = nullptr;
		}
	}

	void TextureData::getInfo(const char* file, int* w, int* h, int* c) {
		getTextureInfo(file, w, h, c);
	}
}