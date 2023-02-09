#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftcache.h> // to enable native freetype cache system

#include <cstdint>
#include <string>
#include <functional>

namespace vulkan {
	class VulkanTexture;
}

namespace engine {

	struct FontData {
		char* fdata = nullptr;
		size_t fileSize = 0;

		FontData(char* data, const size_t size);
		FontData(const std::string& path);
		~FontData() {
			delete[] fdata; // or use cache for it?
		}
	};

	struct Font {
		FontData* fontData;

		FTC_Manager ftcManager;
		FTC_CMapCache ftcCMapCache;
		FTC_SBitCache ftcSBitCache;
		FTC_ImageCache ftcImageCache;
		FT_Library ftcLibrary;
		
		Font(FT_Library library, FontData* data);
		Font(FT_Library library, const std::string& path);
		~Font() { 
			ftcLibrary = nullptr;
			FTC_Manager_Done(ftcManager);
			delete fontData;
		}
	};

	struct FontRenderer {
		unsigned char* image = nullptr;
		uint16_t imgWidth = 0;
		uint16_t imgHeight = 0;

		FontRenderer() = default;
		FontRenderer(const uint16_t w, const uint16_t h, const uint8_t defaultFillValue = 0);
		~FontRenderer() {
			if (image) {
				delete image;
			}
		}

		vulkan::VulkanTexture* createFontTexture() const;

		void render(
			const Font* font,
			const uint8_t fontSize,
			const char* text,
			unsigned char* img,
			const uint16_t imgW,
			const uint16_t imgH,
			int16_t x, 
			int16_t y,
			const uint32_t color = 0xffffffff,
			const uint8_t sx_offset = 0, 
			const uint8_t sy_offset = 0
		);

		void render(
			const Font* font,
			const uint8_t fontSize,
			const char* text,
			int16_t x,
			int16_t y,
			const uint32_t color = 0xffffffff,
			const uint32_t outlineColor = 0xffffffff,
			const float outlineSize = 0.0f,
			const uint8_t sx_offset = 0,
			const uint8_t sy_offset = 0,
			std::function<void(const char s, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const int8_t dy)> addGlyphCallback = nullptr
		);
	};

	class FontsManager {
	public:
		FontsManager() {
			const FT_Error error = FT_Init_FreeType(&_library);
		}

		~FontsManager() {
			FT_Done_FreeType(_library);
		}

		inline FT_Library getLibrary() const { return _library; }

	private:
		FT_Library _library;
	};

}