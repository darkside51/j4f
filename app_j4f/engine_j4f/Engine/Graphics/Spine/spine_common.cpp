#include "spine_common.h"
#include <spine/Extension.h>

#include "../../Core/AssetManager.h"
#include "../../Core/Engine.h"
#include "../../File/FileManager.h"
#include "../Graphics.h"
#include "../Texture/TexturePtrLoader.h"

namespace engine {

//    void deleteAttachmentVertices (void* vertices) {
        //delete static_cast<AttachmentVertices*>(vertices);
//    }

//    static constexpr unsigned short quadTriangles[6] = {0, 1, 2, 2, 3, 0};
//
//    static void setAttachmentVertices(spine::RegionAttachment* attachment) {}

    J4SpineAtlasAttachmentLoader::J4SpineAtlasAttachmentLoader(spine::Atlas* atlas) : AtlasAttachmentLoader(atlas) { }
    J4SpineAtlasAttachmentLoader::~J4SpineAtlasAttachmentLoader() = default;

    void J4SpineAtlasAttachmentLoader::configureAttachment(spine::Attachment* attachment) {
//        const spine::RTTI &attacmentRTTI = attachment->getRTTI();
//
//        if (attacmentRTTI.isExactly(spine::RegionAttachment::rtti)) {
//            setAttachmentVertices(static_cast<spine::RegionAttachment*>(attachment));
//        } else if (attacmentRTTI.isExactly(spine::MeshAttachment::rtti)) {
//            setAttachmentVertices(static_cast<spine::MeshAttachment*>(attachment));
//        }
    }
    
    J4SpineTextureLoader::J4SpineTextureLoader() = default;
    J4SpineTextureLoader::~J4SpineTextureLoader() = default;

    VkFilter convertFilter(spine::TextureFilter f) {
        switch(f) {
            case spine::TextureFilter::TextureFilter_Linear:
                return VK_FILTER_LINEAR;
            case spine::TextureFilter::TextureFilter_Nearest:
                return VK_FILTER_NEAREST;
            default : break;
        }
        return VK_FILTER_NEAREST;
    }

    VkSamplerAddressMode convertAddressMode(spine::TextureWrap w) {
        switch(w) {
            case spine::TextureWrap::TextureWrap_ClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case spine::TextureWrap::TextureWrap_Repeat:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case spine::TextureWrap::TextureWrap_MirroredRepeat:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }
    }

    void J4SpineTextureLoader::load(spine::AtlasPage& page, const spine::String& path) {
        auto* assm = Engine::getInstance().getModule<AssetManager>();

        TexturePtrLoadingParams loadingParams;
        loadingParams.files = { path.buffer() };
        loadingParams.flags->async = 1;
        loadingParams.flags->use_cache = 1;

        TexturePtr texture = assm->loadAsset<TexturePtr>(loadingParams, [&page](TexturePtr const & asset, const AssetLoadingResult result) {
            auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
            const_cast<TextureHandler::value_type*>(asset->get())->setSampler(
                    renderer->getSampler(
                            convertFilter(page.minFilter),
                            convertFilter(page.magFilter),
                            VK_SAMPLER_MIPMAP_MODE_NEAREST,
                            convertAddressMode(page.uWrap),
                            convertAddressMode(page.vWrap),
                            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                            VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
                    ));
        });

        page.texture = texture->get();
        page.width = texture->get()->width();
        page.height = texture->get()->height();
        _textures.push_back(texture);
    }

    void J4SpineTextureLoader::unload(void* texture) {
        _textures.erase(std::remove_if(_textures.begin(), _textures.end(), [texture](const auto& t){
            return t->get() == texture;
        }), _textures.end());
    }

    ///////
    char* J4SpineExtension::_readFile(const spine::String &path, int *length) {
        const auto* fileManager = Engine::getInstance().getModule<FileManager>();
        size_t l = 0;
        char * result = fileManager->readFile(path.buffer(), l);
        *length = static_cast<int>(l);
        return result;
    }
}