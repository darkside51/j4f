#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include <vulkan/vulkan.h>

namespace engine {
    class VertexAttributes {
    public:
        struct AttributeDescription {
            //template <typename T, typename std::enable_if<std::is_fundamental_v<T>, int>::type = 0>
            template <typename T> requires(std::is_fundamental_v<T>)
            static std::pair<AttributeDescription, uint32_t> make(uint32_t componentsCount, uint32_t binding = 0u, bool normalized = true, bool reverse = false) {
                AttributeDescription d;
                if constexpr (std::is_same_v<T, float>) {
                    d.type = static_cast<Type>(componentsCount - 1);
                    d.sizeInBytes = componentsCount * sizeof(float);
                } else if constexpr (std::is_signed_v<T>) {
                    d.type = normalized ? static_cast<Type>(8u + componentsCount) : static_cast<Type>(16u + componentsCount);
                    d.sizeInBytes = componentsCount * sizeof(uint8_t);
                } else {
                    d.type = normalized ? static_cast<Type>(4u + componentsCount) : static_cast<Type>(12u + componentsCount);
                    d.sizeInBytes = componentsCount * sizeof(uint8_t);
                }

                d.reverse = reverse;
                return {std::move(d), binding};
            }

            AttributeDescription() = default;
            AttributeDescription(AttributeDescription&&) = default;
            AttributeDescription& operator=(AttributeDescription&&) = default;

            enum class Type : uint8_t {
                FLOAT_1x32 = 0u,
                FLOAT_2x32,
                FLOAT_3x32,
                FLOAT_4x32,
                UNORM_1x8 = 4u,
                UNORM_2x8,
                UNORM_3x8,
                UNORM_4x8,
                SNORM_1x8 = 8u,
                SNORM_2x8,
                SNORM_3x8,
                SNORM_4x8,
                UINT_1x8 = 12u,
                UINT_2x8,
                UINT_3x8,
                UINT_4x8,
                SINT_1x8 = 16u,
                SINT_2x8,
                SINT_3x8,
                SINT_4x8
            } type;

            bool reverse = false;
            uint8_t sizeInBytes = 0u;
        };

        const std::vector<std::vector<AttributeDescription>> & attributes() const noexcept { return _attributes; }
        inline size_t count() const noexcept { return _count; }

        template<typename T, typename... Args>
        void set(Args&&... args) {
            auto&& [d, binding] = AttributeDescription::make<T>(std::forward<Args>(args)...);
            if (_attributes.size() <= binding) {
                _attributes.resize(binding + 1u);
            }
            _attributes[binding].push_back(std::move(d));
        }

    private:
        size_t _count = 0u;
        std::vector<std::vector<AttributeDescription>> _attributes; // separate by bindings
    };

    struct VulkanAttributesProvider {
        static VkFormat parseFormat(VertexAttributes::AttributeDescription const & description) {
            using Type = VertexAttributes::AttributeDescription::Type;

            switch (description.type) {
                case Type::FLOAT_1x32:
                    return VK_FORMAT_R32_SFLOAT;
                case Type::FLOAT_2x32:
                    return VK_FORMAT_R32G32_SFLOAT;
                case Type::FLOAT_3x32:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case Type::FLOAT_4x32:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;

                case Type::UNORM_1x8:
                    return VK_FORMAT_R8_UNORM;
                case Type::UNORM_2x8:
                    return VK_FORMAT_R8G8_UNORM;
                case Type::UNORM_3x8:
                    return description.reverse ? VK_FORMAT_B8G8R8_UNORM : VK_FORMAT_R8G8B8_UNORM;
                case Type::UNORM_4x8:
                    return description.reverse ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;

                case Type::SNORM_1x8:
                    return VK_FORMAT_R8_SNORM;
                case Type::SNORM_2x8:
                    return VK_FORMAT_R8G8_SNORM;
                case Type::SNORM_3x8:
                    return description.reverse ? VK_FORMAT_B8G8R8_SNORM : VK_FORMAT_R8G8B8_SNORM;
                case Type::SNORM_4x8:
                    return description.reverse ? VK_FORMAT_B8G8R8A8_SNORM : VK_FORMAT_R8G8B8A8_SNORM;

                case Type::UINT_1x8:
                    return VK_FORMAT_R8_UINT;
                case Type::UINT_2x8:
                    return VK_FORMAT_R8G8_UINT;
                case Type::UINT_3x8:
                    return description.reverse ? VK_FORMAT_B8G8R8_UINT : VK_FORMAT_R8G8B8_UINT;
                case Type::UINT_4x8:
                    return description.reverse ? VK_FORMAT_B8G8R8A8_UINT : VK_FORMAT_R8G8B8A8_UINT;

                case Type::SINT_1x8:
                    return VK_FORMAT_R8_SINT;
                case Type::SINT_2x8:
                    return VK_FORMAT_R8G8_SINT;
                case Type::SINT_3x8:
                    return description.reverse ? VK_FORMAT_B8G8R8_SINT : VK_FORMAT_R8G8B8_SINT;
                case Type::SINT_4x8:
                    return description.reverse ? VK_FORMAT_B8G8R8A8_SINT : VK_FORMAT_R8G8B8A8_SINT;
            }

            return VK_FORMAT_UNDEFINED;
        }

        static std::vector<VkVertexInputAttributeDescription> convert(VertexAttributes const & attributesCollection) {
            std::vector<VkVertexInputAttributeDescription> vkAttributes;
            auto const &attributesVec = attributesCollection.attributes();
            vkAttributes.reserve(attributesCollection.count());
            uint32_t attribute_location = 0u;
            uint32_t attribute_binding = 0u;
            for (auto const &attributes: attributesVec) {
                uint32_t attribute_offset = 0u;
                for (auto const &attribute: attributes) {
                    vkAttributes.push_back(VkVertexInputAttributeDescription{
                            attribute_location++,		        // location
                            attribute_binding,					// binding
                            parseFormat(attribute),    // format
                            attribute_offset			            // offset
                    });
                    attribute_offset += attribute.sizeInBytes;
                }
                ++attribute_binding;
            }

            return vkAttributes;
        }
    };
}
