#pragma once

#include "Font.h"
#include "../Render/TextureFrame.h"

#include <unordered_map>
#include<cstdint>

namespace vulkan {
	class VulkanTexture;
}

namespace engine {

	struct BitmapGlyph {
		TextureFrame frame;
		std::vector<float> vtx;
	};

	class BitmapFont {
	public:
		BitmapFont(Font* font, const uint8_t fsz, const uint16_t w, const uint16_t h);

		~BitmapFont() {
			_font = nullptr;
			delete _texture;
			delete _fontRenderer;
		}

		void addSymbols(
			const char* text,
			int16_t x,
			int16_t y,
			const uint32_t color = 0xffffffff,
			const uint8_t sx_offset = 0,
			const uint8_t sy_offset = 0
		);

	private:
		Font* _font;
		uint8_t _fontSize;
		FontRenderer* _fontRenderer;
		vulkan::VulkanTexture* _texture;
		std::unordered_map<char, TextureFrame> _glyphs;
	};

}