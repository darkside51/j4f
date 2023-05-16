#pragma once

#include <spine/spine.h>

namespace engine {
    class J4SpineAtlas;

    class J4SpineAtlasAttachmentLoader: public spine::AtlasAttachmentLoader {
    public:
        explicit J4SpineAtlasAttachmentLoader(spine::Atlas* atlas);
        ~J4SpineAtlasAttachmentLoader() override;
        void configureAttachment(spine::Attachment* attachment) override;
    };

    class J4SpineTextureLoader: public spine::TextureLoader {
    public:
//        struct LoadParams {
//            J4SpineAtlas *atlas = nullptr;
//        };

        J4SpineTextureLoader();
        ~J4SpineTextureLoader() override;

        void load(spine::AtlasPage& page, const spine::String& path) override;
        void unload(void* texture) override;
//        void setLoadParams(LoadParams params);

    private:
//        LoadParams _params;
    };

    class J4SpineExtension: public spine::DefaultSpineExtension {
    public:
        J4SpineExtension();
        ~J4SpineExtension() override;
    protected:
        char *_readFile(const spine::String &path, int *length) override;
    };
}