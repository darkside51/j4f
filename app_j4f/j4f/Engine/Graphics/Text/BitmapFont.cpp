#include "BitmapFont.h"

namespace engine {

	BitmapFont::BitmapFont(Font* font, const uint8_t fsz, const uint16_t w, const uint16_t h, const uint8_t fillValue) : _font(font), _fontSize(fsz), _fontRenderer(new FontRenderer(w, h, fillValue)) {}

	void BitmapFont::complete() {
		_texture = _fontRenderer->createFontTexture();
	}

	void BitmapFont::addSymbols(
		const char* text,
		int16_t x,
		int16_t y,
		const uint32_t color,
		const uint32_t outlineColor,
		const float outlineSize,
		const uint8_t sx_offset,
		const uint8_t sy_offset
	) {
		_fontRenderer->render(_font, _fontSize, text, x, y, color, outlineColor, outlineSize, sx_offset + outlineSize, sy_offset + outlineSize, [this](const char s, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const int8_t dy) {
			std::shared_ptr<TextureFrame> f(new TextureFrame(
				{
					0.0f, float(dy),
					float(w), float(dy),
					0.0f, float(h + dy),
					float(w), float(h + dy)
				},
				{ 
					float(x) / _fontRenderer->imgWidth,		float(h + y) / _fontRenderer->imgHeight, 
					float(w + x) / _fontRenderer->imgWidth, float(h + y) / _fontRenderer->imgHeight,
					float(x) / _fontRenderer->imgWidth,		float(y) / _fontRenderer->imgHeight,
					float(w + x) / _fontRenderer->imgWidth, float(y) / _fontRenderer->imgHeight 
				},
				{
					0, 1, 2, 2, 1, 3
				}
			));
			
			_glyphs[s] = f;
		});
	}


	std::shared_ptr<TextureFrame> BitmapFont::createFrame(const char* text) {
		std::shared_ptr<TextureFrame> result(new TextureFrame());
		const uint16_t len = strlen(text);
		result->_vtx.resize(8 * len);
		result->_uv.resize(8 * len);
		result->_idx.resize(6 * len);

		uint16_t x = 0;
		uint16_t y = 0;
		for (uint16_t i = 0; i < len; ++i) {
			if (text[i] == '\n') {
				x = 0;
				y += _fontSize + 2;
				continue;
			}

			auto it = _glyphs.find(text[i]);

			if (it != _glyphs.end()) {
				const std::shared_ptr<TextureFrame>& f = it->second;
				for (uint8_t j = 0; j < 8; ++j) {
					result->_vtx[i * 8 + j] = f->_vtx[j] + (x * (1 - (j & 1))) - (y * (j & 1)); // (x * (1 - j & 1)) - offset by ox only, (y * (j & 1)) - offset by oy only;
					result->_uv[i * 8 + j] = f->_uv[j];
					if (j < 6) {
						result->_idx[i * 6 + j] = i * 4 + f->_idx[j];
					}
				}
				const float wf = f->_vtx[2] - f->_vtx[0];
				x += wf;
			} else {
				x += _fontSize / 4.0f;
			}
		}

		return result;
	}
}