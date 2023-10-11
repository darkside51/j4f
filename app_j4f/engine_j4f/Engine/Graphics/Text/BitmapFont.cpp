#include "BitmapFont.h"

namespace engine {

	BitmapFont::BitmapFont(Font* font, const uint16_t w, const uint16_t h, BitmapFontParams&& params, const uint8_t fillValue) :
    _font(font),
    _params(std::move(params)),
    _fontRenderer(new FontRenderer(w, h, fillValue)) {}

    BitmapFont::BitmapFont(Font* font, const uint16_t w, const uint16_t h, const BitmapFontParams& params, const uint8_t fillValue) :
    _font(font),
    _params(params),
    _fontRenderer(new FontRenderer(w, h, fillValue)) {}

	void BitmapFont::complete() {
		_texture = _fontRenderer->createFontTexture();
	}

	void BitmapFont::addSymbols(
        std::wstring_view text,
        const int16_t x,
        const int16_t y,
		const uint32_t color,
		const uint32_t outlineColor,
		const float outlineSize,
		const uint8_t sx_offset,
		const uint8_t sy_offset
	) {
		_fontRenderer->render( (_params.type == BitmapFontType::Usual ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_SDF),
                               _font, _params.fontSize, text, x, y, color, outlineColor, outlineSize,
                              sx_offset + outlineSize, sy_offset + outlineSize,
                              [this](const wchar_t s, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const int8_t dy) {
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
					0u, 1u, 2u, 2u, 1u, 3u
				}
			));
			
			_glyphs[s] = f;
		});
	}

	std::shared_ptr<TextureFrame> BitmapFont::createFrame(std::wstring_view text) {
		std::shared_ptr<TextureFrame> result(new TextureFrame());
		const uint16_t len = text.length();
		result->_vtx.resize(8u * len);
		result->_uv.resize(8u * len);
		result->_idx.resize(6u * len);

		uint16_t x = 0u;
		uint16_t y = 0u;

		for (uint16_t i = 0u; i < len; ++i) {
			if (text[i] == '\n') {
				x = 0u;
				y += _params.fontSize + _params.offset_y;
				continue;
			}

			auto it = _glyphs.find(text[i]);

			if (it != _glyphs.end()) {
				const std::shared_ptr<TextureFrame>& f = it->second;
				for (uint8_t j = 0u; j < 8u; ++j) {
					result->_vtx[i * 8u + j] = f->_vtx[j] + (x * (1 - (j & 1))) - (y * (j & 1)); // (x * (1 - j & 1)) - offset by ox only, (y * (j & 1)) - offset by oy only;
					result->_uv[i * 8u + j] = f->_uv[j];
					if (j < 6u) {
						result->_idx[i * 6u + j] = i * 4u + f->_idx[j];
					}
				}
				const float wf = f->_vtx[2u] - f->_vtx[0u] + _params.offset_x;
				x += wf;
			} else { // if no has symbol: " " for example
                x += _params.space_width;
			}
		}

		return result;
	}
}