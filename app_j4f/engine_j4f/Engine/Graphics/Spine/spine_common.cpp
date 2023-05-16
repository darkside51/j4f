#include "spine_common.h"
#include <spine/Extension.h>

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

    void J4SpineTextureLoader::load(spine::AtlasPage& page, const spine::String& path) {

    }

    void J4SpineTextureLoader::unload(void* texture) {
        if (texture) {

        }
    }

}