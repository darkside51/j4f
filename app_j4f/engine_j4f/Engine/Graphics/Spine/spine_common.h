#pragma once

#include <spine/spine.h>
#include "../Texture/TextureHandler.h"

#include <vector>

namespace engine {
//    class J4SpineAtlasAttachmentLoader: public spine::AtlasAttachmentLoader {
//    public:
//        explicit J4SpineAtlasAttachmentLoader(spine::Atlas* atlas);
//        ~J4SpineAtlasAttachmentLoader() override;
//        void configureAttachment(spine::Attachment* attachment) override;
//    };

    class SpineTextureLoader: public spine::TextureLoader {
    public:
        SpineTextureLoader();
        ~SpineTextureLoader() override;

        void load(spine::AtlasPage& page, const spine::String& path) override;
        void unload(void* texture) override;
    private:
        std::vector<TexturePtr> _textures;
    };

    class SpineExtension: public spine::DefaultSpineExtension {
    public:
        SpineExtension();
        ~SpineExtension() override;
    protected:
        char *_readFile(const spine::String &path, int *length) override;
    };
}