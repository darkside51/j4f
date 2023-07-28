#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

#include <vulkan/vulkan.h>

namespace engine {
    enum class AttributeLayout : uint8_t {
        Forward = 0u,
        Backward = 1u
    };

    class VertexAttributes {
    public:
        struct AttributeDescription {
            //template <typename T, typename std::enable_if<std::is_fundamental_v<T>, int>::type = 0>
            template <typename T, uint32_t binding = 0u> requires(std::is_fundamental_v<T>)
            static AttributeDescription make(uint32_t componentsCount, bool normalized = false, AttributeLayout layout = AttributeLayout::Forward) {
                AttributeDescription d;
                if constexpr (std::is_same_v<T, float>) {
                    d.type = static_cast<Type>(componentsCount - 1);
                    d.sizeInBytes = componentsCount * sizeof(float);
                } else if constexpr (std::is_signed_v<T>) {
                    d.type = normalized ? static_cast<Type>(8u + componentsCount - 1u) : static_cast<Type>(16u + componentsCount - 1u);
                    d.sizeInBytes = componentsCount * sizeof(uint8_t);
                } else {
                    d.type = normalized ? static_cast<Type>(4u + componentsCount - 1u) : static_cast<Type>(12u + componentsCount - 1u);
                    d.sizeInBytes = componentsCount * sizeof(uint8_t);
                }

                d.backward = (layout == AttributeLayout::Backward);
                return d;
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
            } type = Type::FLOAT_1x32;

            bool backward = false;
            uint8_t sizeInBytes = 0u;
        };

        VertexAttributes() = default;

        VertexAttributes(std::initializer_list<uint32_t> && l) : _attributes(l.size()) {
            size_t i = 0u;
            for (auto v : l) {
                _count += v;
                _attributes[i++].reserve(v);
            }
        }

        template<size_t S>
        explicit VertexAttributes(std::array<uint32_t, S> && array) : _attributes(S) {
            for (size_t i = 0; i < S; ++i) {
                _count += array[i];
                _attributes[i].reserve(array[i]);
            }
        }

        const std::vector<std::vector<AttributeDescription>> & attributes() const noexcept { return _attributes; }
        inline uint32_t count() const noexcept { return _count; }

        template<typename T, uint32_t binding, typename... Args>
        void set(Args&&... args) {
            if (_attributes.size() <= binding) {
                _attributes.resize(binding + 1u);
            }
            _attributes[binding].push_back(AttributeDescription::make<T, binding>(std::forward<Args>(args)...));
            ++_count;
        }

    private:
        uint32_t _count = 0u;
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
                    return description.backward ? VK_FORMAT_B8G8R8_UNORM : VK_FORMAT_R8G8B8_UNORM;
                case Type::UNORM_4x8:
                    return description.backward ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;

                case Type::SNORM_1x8:
                    return VK_FORMAT_R8_SNORM;
                case Type::SNORM_2x8:
                    return VK_FORMAT_R8G8_SNORM;
                case Type::SNORM_3x8:
                    return description.backward ? VK_FORMAT_B8G8R8_SNORM : VK_FORMAT_R8G8B8_SNORM;
                case Type::SNORM_4x8:
                    return description.backward ? VK_FORMAT_B8G8R8A8_SNORM : VK_FORMAT_R8G8B8A8_SNORM;

                case Type::UINT_1x8:
                    return VK_FORMAT_R8_UINT;
                case Type::UINT_2x8:
                    return VK_FORMAT_R8G8_UINT;
                case Type::UINT_3x8:
                    return description.backward ? VK_FORMAT_B8G8R8_UINT : VK_FORMAT_R8G8B8_UINT;
                case Type::UINT_4x8:
                    return description.backward ? VK_FORMAT_B8G8R8A8_UINT : VK_FORMAT_R8G8B8A8_UINT;

                case Type::SINT_1x8:
                    return VK_FORMAT_R8_SINT;
                case Type::SINT_2x8:
                    return VK_FORMAT_R8G8_SINT;
                case Type::SINT_3x8:
                    return description.backward ? VK_FORMAT_B8G8R8_SINT : VK_FORMAT_R8G8B8_SINT;
                case Type::SINT_4x8:
                    return description.backward ? VK_FORMAT_B8G8R8A8_SINT : VK_FORMAT_R8G8B8A8_SINT;
            }

            return VK_FORMAT_UNDEFINED;
        }

        static std::vector<VkVertexInputAttributeDescription> convert(VertexAttributes const & attributesCollection) {
            std::vector<VkVertexInputAttributeDescription> vkAttributes;
            vkAttributes.reserve(attributesCollection.count());
            auto const& attributesVec = attributesCollection.attributes();
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
