#pragma once

#include <string>

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/Image.h"

namespace NR
{
    struct TextureSpecification
    {
        uint32_t Width = 1;
        uint32_t Height = 1;
        ImageFormat Format = ImageFormat::RGBA;
        bool GenerateMips = true;
    };

    class Texture
    {
    public:
        virtual ~Texture() = default;

        virtual const TextureSpecification& GetSpecification() const = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetData(void* data, uint32_t size) = 0;

        virtual const std::string& GetPath() const = 0;

        virtual void Bind(uint32_t slot = 0) const = 0;
        virtual uint32_t GetRendererID() = 0;

        virtual bool operator== (const Texture& other) const = 0;

        virtual TextureType GetType() const = 0;
    };

    class Texture2D : public Texture
    {
    public:
        static Ref<Texture2D> Create(const TextureSpecification& specification);
        static Ref<Texture2D> Create(const std::string& path);

        TextureType GetType() const override { return TextureType::Texture2D; }
    };

    class TextureCube : public Texture
    {
    public:
        static Ref<TextureCube> Create(ImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr, TextureProperties properties = TextureProperties());
        static Ref<TextureCube> Create(const std::string& path, TextureProperties properties = TextureProperties());

        virtual const std::string& GetPath() const = 0;

        TextureType GetType() const override { return TextureType::TextureCube; }

        //TODO
        //static AssetType GetStaticType() { return AssetType::EnvMap; }
        //virtual AssetType GetAssetType() const override { return GetStaticType(); }
    };
}