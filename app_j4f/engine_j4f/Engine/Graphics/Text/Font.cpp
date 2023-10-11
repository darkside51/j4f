#include "Font.h"

#include "../../Engine/Core/Engine.h"
#include "../../File/FileManager.h"
#include "../Graphics.h"
#include "../Vulkan/vkRenderer.h"
#include "../Vulkan/vkTexture.h"

#include <freetype/ftimage.h>
#include <freetype/ftstroke.h>

#include <string_view>

namespace engine {

    FontData::FontData(char* data, const size_t size) : fdata(data), fileSize(size) { }

    FontData::FontData(const std::string& path) {
        fdata = Engine::getInstance().getModule<FileManager>().readFile(path, fileSize);
    }

    FT_Error ftc_face_requester(FTC_FaceID faceID, FT_Library lib, FT_Pointer reqData, FT_Face* face) {
        Font* f = reinterpret_cast<Font*>(reqData);
        return FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte*>(f->fontData->fdata), f->fontData->fileSize, 0, face);
    }

    Font::Font(FT_Library library, FontData* data) : fontData(data), ftcLibrary(library) {
        FTC_Manager_New(library, 0, 0, 0, ftc_face_requester, this, &ftcManager);
        FTC_CMapCache_New(ftcManager, &ftcCMapCache);
        FTC_SBitCache_New(ftcManager, &ftcSBitCache);
        FTC_ImageCache_New(ftcManager, &ftcImageCache);
    }

    Font::Font(FT_Library library, const std::string& path) : fontData(new FontData(path)), ftcLibrary(library) {
        FTC_Manager_New(library, 0, 0, 0, ftc_face_requester, this, &ftcManager);
        FTC_CMapCache_New(ftcManager, &ftcCMapCache);
        FTC_SBitCache_New(ftcManager, &ftcSBitCache);
        FTC_ImageCache_New(ftcManager, &ftcImageCache);
    }

    ////// fontRenderer

    bool fill_image(
        unsigned char* img,
        const uint16_t imgW,
        const uint16_t imgH,
        FT_Byte* buffer,
        FT_Byte width,
        FT_Byte height,
        const FT_Int x,
        const FT_Int y,
        const uint32_t color
    ) {
        if (
            x < 0               ||
            y < 0               ||
            (x + width) >= imgW ||
            (y)         >= imgH
            ) return false;

        for (uint32_t j = y, q = 0; q < height; ++j, ++q) {
            for (uint32_t i = x, p = 0; p < width; ++i, ++p) {
                if (buffer[q * width + p]) {
                    const auto value = buffer[q * width + p];
                    const uint32_t r = (color >> 24) & 0xff;
                    const uint32_t g = (color >> 16) & 0xff;
                    const uint32_t b = (color >> 8) & 0xff;
                    const uint32_t a = (value >> 0) & 0xff;
                    const uint32_t n = (j * imgW + i) * 4;

                    img[n + 0] = r;
                    img[n + 1] = g;
                    img[n + 2] = b;
                    img[n + 3] = a;
                }
            }
        }

        return true;
    }

    FontRenderer::FontRenderer(const uint16_t w, const uint16_t h, const uint8_t defaultFillValue) {
        image = new unsigned char[w * h * 4u];
        memset(image, defaultFillValue, sizeof(unsigned char) * w * h * 4u);
        imgWidth = w;
        imgHeight = h;
    }

    vulkan::VulkanTexture* FontRenderer::createFontTexture() const {
        auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
        auto texture = new vulkan::VulkanTexture(renderer, imgWidth, imgHeight);

        texture->setSampler(renderer->getSampler(
            VK_FILTER_LINEAR,
            VK_FILTER_LINEAR,
            VK_SAMPLER_MIPMAP_MODE_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
        ));

        texture->create(image, VK_FORMAT_R8G8B8A8_UNORM, 32u, false, false);
        return texture;
    }

	void FontRenderer::render(
		const Font* font,
		const uint8_t fontSize,
        std::wstring_view text,
		unsigned char* img,
        const uint16_t imgW,
        const uint16_t imgH,
		int16_t x, 
        int16_t y,
		const uint32_t color,
		const uint8_t sx_offset, 
		const uint8_t sy_offset
	) {
        FTC_ImageTypeRec_ ftcImageType;
        ftcImageType.face_id = 0;
        ftcImageType.width = fontSize;
        ftcImageType.height = fontSize;
        ftcImageType.flags = FT_LOAD_DEFAULT | FT_LOAD_RENDER;

        for (size_t i = 0, sz = text.length(); i < sz; ++i) {
            const auto glyphIndex = FTC_CMapCache_Lookup(font->ftcCMapCache, 0, 0, text[i]);

            FTC_SBit ftcSBit;
            FTC_SBitCache_Lookup(font->ftcSBitCache, &ftcImageType, glyphIndex, &ftcSBit, nullptr);

            fill_image(
                img,
                imgW,
                imgH,
                ftcSBit->buffer,
                ftcSBit->width,
                ftcSBit->height,
                x + ftcSBit->left,
                y + fontSize - ftcSBit->top,
                color);

            // increment pen position
            x += (ftcSBit->xadvance) + sx_offset;
            y += (ftcSBit->yadvance) + sy_offset;
        }
	}

    void FontRenderer::render(
        const FT_Render_Mode_ renderMode,
        const Font* font,
        const uint8_t fontSize,
        std::wstring_view text,
        int16_t x,
        int16_t y,
        const uint32_t color,
        const uint32_t outlineColor,
        const float outlineSize,
        const uint8_t sx_offset,
        const uint8_t sy_offset,
        std::function<void(const wchar_t s, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const int8_t dy)> addGlyphCallback
    ) {
        FTC_ImageTypeRec_ ftcImageType;
        ftcImageType.face_id = 0;
        ftcImageType.width = fontSize;
        ftcImageType.height = fontSize;
        ftcImageType.flags = FT_LOAD_DEFAULT;

        const bool useOutline = (renderMode == FT_RENDER_MODE_NORMAL) && (outlineSize != 0.0f);

        const int16_t x0 = x;
        FT_Stroker stroker;
        FT_Error err;

        if (useOutline) {
            err = FT_Stroker_New(font->ftcLibrary, &stroker);
            //outlineSize px outline
            FT_Stroker_Set(stroker, static_cast<FT_Fixed>(outlineSize * (1 << 6)), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
        }

        FT_Pos maxRowHeight = 0;

        for (size_t i = 0u, len = text.length(); i < len; ++i) {
            const wchar_t wc = text[i];
            const FT_UInt32 charCode = wc;
            const auto glyphIndex = FTC_CMapCache_Lookup(font->ftcCMapCache, 0, 0, charCode);
 
            FT_Glyph glyph;
            FT_BitmapGlyph bitmapGlyph;
            bool renderGlyph = true;

            FT_Pos glyph_width = 0;

            if (useOutline) {
                FTC_ImageCache_Lookup(font->ftcImageCache, &ftcImageType, glyphIndex, &glyph, nullptr);
                err = FT_Glyph_StrokeBorder(&glyph, stroker, false, false);
                err = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
                bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

                FT_BBox acbox;
                FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &acbox);
                const auto g_height = acbox.yMax - acbox.yMin;
                const auto g_width = acbox.xMax - acbox.xMin;

                glyph_width = g_width;

                maxRowHeight = std::max(maxRowHeight, g_height);

                renderGlyph = fill_image(
                        image,
                        imgWidth,
                        imgHeight,
                        bitmapGlyph->bitmap.buffer,
                        g_width,
                        g_height,
                        x,
                        y + bitmapGlyph->top - acbox.yMax,
                        outlineColor);

                if (addGlyphCallback) {
                    addGlyphCallback(wc, x, y + bitmapGlyph->top - acbox.yMax, g_width, g_height, acbox.yMax - bitmapGlyph->bitmap.rows + static_cast<int8_t>(outlineSize));
                }
            }

            if (renderGlyph) {
                FTC_ImageCache_Lookup(font->ftcImageCache, &ftcImageType, glyphIndex, &glyph, nullptr);
                FT_Glyph_To_Bitmap(&glyph, renderMode, nullptr, false);

                FT_BBox acbox;
                FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &acbox);
                bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

                const auto g_height = acbox.yMax - acbox.yMin;
                const auto g_width = acbox.xMax - acbox.xMin;

                maxRowHeight = std::max(maxRowHeight, g_height);
                glyph_width = std::max(glyph_width, g_width);

                fill_image(
                    image,
                    imgWidth,
                    imgHeight,
                    bitmapGlyph->bitmap.buffer,
                    g_width,
                    g_height,
                    x + outlineSize,
                    y + bitmapGlyph->top - acbox.yMax + outlineSize,
                    color);

                if (outlineSize == 0.0f) {
                    if (addGlyphCallback) {
                        addGlyphCallback(wc, x + outlineSize, y + bitmapGlyph->top - acbox.yMax + outlineSize, g_width, g_height, acbox.yMax - bitmapGlyph->bitmap.rows);
                    }
                }
            }

            x += glyph_width + sx_offset;

            if (x >= imgWidth) {
                if (x > imgWidth) --i;
                x = x0;
                y += maxRowHeight + sy_offset;
                maxRowHeight = 0;
            }
        }

        if (useOutline) {
            FT_Stroker_Done(stroker);
        }
    }
}