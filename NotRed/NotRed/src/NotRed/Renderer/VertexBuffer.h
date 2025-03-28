#pragma once

#include "RendererTypes.h"
#include "NotRed/Core/Log.h"

namespace NR
{
    enum class ShaderDataType
    {
        None,
        Bool,
        Int, Int2, Int3, Int4,
        Float, Float2, Float3, Float4,
        Mat3, Mat4,
    };

    static uint32_t ShaderDataTypeSize(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Bool:     return 1;
        case ShaderDataType::Int:      return 4;
        case ShaderDataType::Int2:     return 4 * 2;
        case ShaderDataType::Int3:     return 4 * 3;
        case ShaderDataType::Int4:     return 4 * 4;
        case ShaderDataType::Float:    return 4;
        case ShaderDataType::Float2:   return 4 * 2;
        case ShaderDataType::Float3:   return 4 * 3;
        case ShaderDataType::Float4:   return 4 * 4;
        case ShaderDataType::Mat3:     return 4 * 3 * 3;
        case ShaderDataType::Mat4:     return 4 * 4 * 4;
        default:
        {
            NR_CORE_ASSERT(false, "Unknown ShaderDataType!");
            return 0;
        }
        }
    }

    struct VertexBufferElement
    {
        ShaderDataType Type;
        std::string Name;
        uint32_t Size;
        uint32_t Offset;
        bool Normalized;

        VertexBufferElement() = default;

        VertexBufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
            : Type(type), Name(name), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
        {
        }

        uint32_t GetComponentCount() const
        {
            switch (Type)
            {
            case ShaderDataType::Bool:    return 1;
            case ShaderDataType::Int:     return 1;
            case ShaderDataType::Int2:    return 2;
            case ShaderDataType::Int3:    return 3;
            case ShaderDataType::Int4:    return 4;
            case ShaderDataType::Float:   return 1;
            case ShaderDataType::Float2:  return 2;
            case ShaderDataType::Float3:  return 3;
            case ShaderDataType::Float4:  return 4;
            case ShaderDataType::Mat3:    return 3 * 3;
            case ShaderDataType::Mat4:    return 4 * 4;
            default:
            {
                NR_CORE_ASSERT(false, "Unknown ShaderDataType!");
                return 0;
            }
            }
        }
    };

    class VertexBufferLayout
    {
    public:
        VertexBufferLayout() {}

        VertexBufferLayout(const std::initializer_list<VertexBufferElement>& elements)
            : mElements(elements)
        {
            CalculateOffsetsAndStride();
        }

        uint32_t GetStride() const { return mStride; }
        const std::vector<VertexBufferElement>& GetElements() const { return mElements; }
        uint32_t GetElementCount() const { return (uint32_t)mElements.size(); }

        [[nodiscard]] std::vector<VertexBufferElement>::iterator begin() { return mElements.begin(); }
        [[nodiscard]] std::vector<VertexBufferElement>::iterator end() { return mElements.end(); }
        [[nodiscard]] std::vector<VertexBufferElement>::const_iterator begin() const { return mElements.begin(); }
        [[nodiscard]] std::vector<VertexBufferElement>::const_iterator end() const { return mElements.end(); }

    private:
        void CalculateOffsetsAndStride()
        {
            uint32_t offset = 0;
            mStride = 0;
            for (auto& element : mElements)
            {
                element.Offset = offset;
                offset += element.Size;
                mStride += element.Size;
            }
        }

    private:
        std::vector<VertexBufferElement> mElements;
        uint32_t mStride = 0;
    };

    enum class VertexBufferUsage
    {
        None, 
        Static, 
        Dynamic
    };

    class VertexBuffer : public RefCounted
    {
    public:
        static Ref<VertexBuffer> Create(void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
        static Ref<VertexBuffer> Create(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);

        virtual ~VertexBuffer() {}

        virtual void SetData(void* data, uint32_t size, uint32_t offset = 0) = 0;
        virtual void RT_SetData(void* buffer, uint32_t size, uint32_t offset = 0) = 0;
        virtual void Bind() const = 0;

        virtual uint32_t GetSize() const = 0;
        virtual RendererID GetRendererID() const = 0;
    };
}

