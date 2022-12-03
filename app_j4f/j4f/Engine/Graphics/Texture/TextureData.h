#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace engine {

	enum class TextureFormatType : uint8_t {
		UNORM = 0,
		SNORM = 1,
		USCALED = 2,
		SSCALED = 3,
		UINT = 4,
		SINT = 5,
		SFLOAT = 6,
		SRGB = 7
	};

	class TextureData {
	public:
		static void getInfo(const char* file, int* w, int* h, int* c);

		TextureData() = default;
		TextureData(const std::string& path, const TextureFormatType ft = TextureFormatType::UNORM);
		TextureData(const unsigned char* buffer, const size_t size, const TextureFormatType ft = TextureFormatType::UNORM);
		~TextureData();

		TextureData(const TextureData&) = delete;
		TextureData(TextureData&& rvalue) noexcept : _data(rvalue._data), _width(rvalue._width), _height(rvalue._height), _channels(rvalue._channels), _bpp(rvalue._bpp), _format(rvalue._format) {
			rvalue._data = nullptr;
		}

		TextureData& operator=(const TextureData&) = delete;
		TextureData& operator=(TextureData&& rvalue) noexcept {
			_data = rvalue._data;
			_width = rvalue._width;
			_height = rvalue._height;
			_channels = rvalue._channels;
			_bpp = rvalue._bpp;
			_format = rvalue._format;

			rvalue._data = nullptr;
			return *this;
		}

		inline operator bool() const { return _data != nullptr; }
		inline const unsigned char* data() const { return _data; }

		inline uint8_t bpp() const { return _bpp; }
		inline VkFormat format() const { return _format; }

		inline int width() const { return _width; }
		inline int height() const { return _height; }
		inline int channels() const { return _channels; }

	private:
		unsigned char* _data = nullptr;
		int _width = 0;
		int _height = 0;
		int _channels = 0;
		uint8_t _bpp = 0;
		VkFormat _format = VK_FORMAT_UNDEFINED;
	};
}