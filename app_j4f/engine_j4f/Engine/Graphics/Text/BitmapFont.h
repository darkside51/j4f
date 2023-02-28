#pragma once

#include "Font.h"
#include "../Render/TextureFrame.h"

#include <unordered_map>
#include <cstdint>
#include <memory>

namespace vulkan {
	class VulkanTexture;
}

namespace engine {

	struct BitmapGlyph {
		TextureFrame frame;
		std::vector<float> vtx;
	};

    enum class BitmapFontType : uint8_t {
        Usual,
        SDF
    };

    struct BitmapFontParams {
        BitmapFontType type;
        uint8_t fontSize = 0;
        int8_t space_y = 0;
        int8_t space_x = 0;
        uint8_t empty_width = 0;
    };

	class BitmapFont {
	public:
		BitmapFont(Font* font, const uint16_t w, const uint16_t h, BitmapFontParams&& params, const uint8_t fillValue = 0);
        BitmapFont(Font* font, const uint16_t w, const uint16_t h, const BitmapFontParams& params, const uint8_t fillValue = 0);

		~BitmapFont() {
			_font = nullptr;
			if (_texture) {
				delete _texture;
			}
			delete _fontRenderer;
		}

		void addSymbols(
			const char* text,
            const int16_t x = 0,
            const int16_t y = 0,
			const uint32_t color = 0xffffffff,
			const uint32_t outlineColor = 0x00000000,
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
				return it->second;
			}

			return nullptr;
		}

		std::shared_ptr<TextureFrame> createFrame(const char* text);

	private:
		Font* _font;
        BitmapFontParams _params;
		FontRenderer* _fontRenderer;
		vulkan::VulkanTexture* _texture = nullptr;
		std::unordered_map<char, std::shared_ptr<TextureFrame>> _glyphs;
	};

}