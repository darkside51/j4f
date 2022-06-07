#include "BitmapFont.h"

namespace engine {

	BitmapFont::BitmapFont(Font* font, const uint8_t fsz, const uint16_t w, const uint16_t h) : _font(font), _fontSize(fsz), _fontRenderer(new FontRenderer(w, h, 0)) {
		_texture = _fontRenderer->createFontTexture();
	}

	void BitmapFont::addSymbols(
		const char* text,
		int16_t x,
		int16_t y,
		const uint32_t color,
		const uint8_t sx_offset,
		const uint8_t sy_offset
	) {
		_fontRenderer->render(_font, _fontSize, text, x, y, color, sx_offset, sy_offset, [this](const char s, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) {
			TextureFrame f({ 
				float(x) / _fontRenderer->imgWidth, float(y) / _fontRenderer->imgHeight, 
				float(w) / _fontRenderer->imgWidth, float(y) / _fontRenderer->imgHeight,
				float(x) / _fontRenderer->imgWidth, float(h) / _fontRenderer->imgHeight,
				float(w) / _fontRenderer->imgWidth, float(h) / _fontRenderer->imgHeight 
			});
			
			_glyphs[s] = f;
		});
	}

}