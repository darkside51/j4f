#include "Font.h"

#include "../../Engine/Core/Engine.h"
#include "../../File/FileManager.h"
#include "../Graphics.h"
#include "../Vulkan/vkRenderer.h"
#include "../Vulkan/vkTexture.h"

namespace engine {

    FontData::FontData(char* data, const size_t size) : fdata(data), fileSize(size) { }

    FontData::FontData(const std::string& path) {
        fdata = Engine::getInstance().getModule<FileManager>()->readFile(path, fileSize);
    }

    FT_Error ftc_face_requester(FTC_FaceID faceID, FT_Library lib, FT_Pointer reqData, FT_Face* face) {
        Font* f = reinterpret_cast<Font*>(reqData);
        return FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte*>(f->fontData->fdata), f->fontData->fileSize, 0, face);
    }

    Font::Font(FT_Library library, FontData* data) : fontData(data) {
        FTC_Manager_New(library, 0, 0, 0, ftc_face_requester, this, &ftcManager);
        FTC_CMapCache_New(ftcManager, &ftcCMapCache);
        FTC_SBitCache_New(ftcManager, &ftcSBitCache);
    }

    Font::Font(FT_Library library, const std::string& path) : fontData(new FontData(path)) {
        FTC_Manager_New(library, 0, 0, 0, ftc_face_requester, this, &ftcManager);
        FTC_CMapCache_New(ftcManager, &ftcCMapCache);
        FTC_SBitCache_New(ftcManager, &ftcSBitCache);
    }

    ////// fontRenderer

    void fill_image(
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
        for (uint32_t j = y, q = 0; q < height; ++j, ++q) {
            for (uint32_t i = x, p = 0; p < width; ++i, ++p) {
                if (i < 0 || j < 0 || (i + width) >= imgW || j >= imgH) {
                    continue;
                }

                if (buffer[q * width + p]) {

                    const uint32_t r = (color >> 24) & 0xff;
                    const uint32_t g = (color >> 16) & 0xff;
                    const uint32_t b = (color >> 8) & 0xff;
                    const uint32_t a = (color >> 0) & 0xff;
                    const uint32_t n = (j * imgW + i) * 4;

                    img[n + 0] = r;
                    img[n + 1] = g;
                    img[n + 2] = b;
                    img[n + 3] = a;
                }
            }
        }
    }

    FontRenderer::FontRenderer(const uint16_t w, const uint16_t h, const uint8_t defaultFillValue) {
        image = new unsigned char[w * h * 4];
        memset(image, defaultFillValue, sizeof(unsigned char) * w * h * 4);
        imgWidth = w;
        imgHeight = h;
    }

    vulkan::VulkanTexture* FontRenderer::createFontTexture() const {
        auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
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

        texture->create(image, VK_FORMAT_R8G8B8A8_UNORM, 32, false, false);
        return texture;
    }

	void FontRenderer::render(
		const Font* font,
		const uint8_t fontSize,
		const char* text,
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

        for (size_t i = 0, sz = strlen(text); i < sz; ++i) {
            const auto glyphIndex = FTC_CMapCache_Lookup(font->ftcCMapCache, 0, 0, text[i]);

            //FTC_Node ftcNode;
            //FTC_SBitCache_Lookup(ftcSBitCache, &ftcImageType, glyphIndex, &ftcSBit, &ftcNode);
            //FTC_Node_Unref(ftcNode, ftcManager);

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

            /* increment pen position */
            x += (ftcSBit->xadvance) + sx_offset;
            y += (ftcSBit->yadvance) + sy_offset;
        }
	}


    void FontRenderer::render(
        const Font* font,
        const uint8_t fontSize,
        const char* text,
        int16_t x,
        int16_t y,
        const uint32_t color,
        const uint8_t sx_offset,
        const uint8_t sy_offset,
        std::function<void(const char s, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h)> addGlyphCallback
    ) {
        FTC_ImageTypeRec_ ftcImageType;
        ftcImageType.face_id = 0;
        ftcImageType.width = fontSize;
        ftcImageType.height = fontSize;
        ftcImageType.flags = FT_LOAD_DEFAULT | FT_LOAD_RENDER;

        for (size_t i = 0, sz = strlen(text); i < sz; ++i) {
            const auto glyphIndex = FTC_CMapCache_Lookup(font->ftcCMapCache, 0, 0, text[i]);

            //FTC_Node ftcNode;
            //FTC_SBitCache_Lookup(ftcSBitCache, &ftcImageType, glyphIndex, &ftcSBit, &ftcNode);
            //FTC_Node_Unref(ftcNode, ftcManager);

            FTC_SBitCache_Lookup(font->ftcSBitCache, &ftcImageType, glyphIndex, &ftcSBit, nullptr);

            fill_image(
                image,
                imgWidth,
                imgHeight,
                ftcSBit->buffer,
                ftcSBit->width,
                ftcSBit->height,
                x + ftcSBit->left,
                y + fontSize - ftcSBit->top,
                color);

            if (addGlyphCallback) {
                addGlyphCallback(text[i], x + ftcSBit->left, y + fontSize - ftcSBit->top, ftcSBit->width, ftcSBit->height);
            }

            /* increment pen position */
            x += (ftcSBit->xadvance) + sx_offset;
            y += (ftcSBit->yadvance) + sy_offset;
        }
    }
}