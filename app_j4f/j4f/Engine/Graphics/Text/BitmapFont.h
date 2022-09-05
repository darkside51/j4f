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
		BitmapFont(Font* font, const uint8_t fsz, const uint16_t w, const uint16_t h, const uint8_t fillValue = 0);

		~BitmapFont() {
			_font = nullptr;
			if (_texture) {
				delete _texture;
			}
			delete _fontRenderer;
		}

		void addSymbols(
			const char* text,
			int16_t x,
			int16_t y,
			const uint32_t color = 0xffffffff,
			const uint32_t outlineColor = 0xffffffff,
			const float outlineSize = 0.0f,
			const uint8_t sx_offset = 0,
			const uint8_t sy_offset = 0
		);

		void complete();

		vulkan::VulkanTexture* getTexture() const { return _texture; }
		vulkan::VulkanTexture* grabTexture() {
			vulkan::VulkanTexture* result = _texture;
			_texture = nullptr;
			return result;
		}

		inline std::shared_ptr<TextureFrame> getFrame(const char s) const {
			auto it = _glyphs.find(s);
			if (it != _glyphs.end()) {
				return it->second.first;
			}

			return nullptr;
		}

		std::shared_ptr<TextureFrame> createFrame(const char* text);

	private:
		Font* _font;
		uint8_t _fontSize;
		FontRenderer* _fontRenderer;
		vulkan::VulkanTexture* _texture = nullptr;
		std::unordered_map<char, std::pair<std::shared_ptr<TextureFrame>, int8_t>> _glyphs;
	};

}