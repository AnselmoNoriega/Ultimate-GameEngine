#pragma once

#include "NotRed/Renderer/Texture.h"

namespace NR
{
    class GLTexture2D : public Texture2D
    {
    public:
        GLTexture2D(const TextureSpecification& specification);
        GLTexture2D(const std::string& path);
        ~GLTexture2D() override;

        const TextureSpecification& GetSpecification() const override { return mSpecification; }

        uint32_t GetWidth() const override { return mWidth; }
        uint32_t GetHeight() const override { return mHeight; }

        void SetData(void* data, uint32_t size) override;

        const std::string& GetPath() const override { return mPath; }

        void Bind(uint32_t slot = 0) const override;
        uint32_t GetRendererID() override { return mID; };

        bool operator== (const Texture& other) const override
        {
            return mID == ((GLTexture2D&)other).mID;
        }

    private:
        TextureSpecification mSpecification;

        uint32_t mID;
        std::string mPath;
        uint32_t mDataFormat;
        uint32_t mWidth, mHeight;
    };

    class GLTextureCube : public TextureCube
    {
    public:
        GLTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data);
        GLTextureCube(const std::string& path);
        ~GLTextureCube() override;

        virtual void Bind(uint32_t slot = 0) const;

        ImageFormat GetFormat() const { return mFormat; }
        uint32_t GetWidth() const { return mWidth; }
        uint32_t GetHeight() const { return mHeight; }

        virtual const std::string& GetPath() const override { return mFilePath; }

        uint32_t GetRendererID() const { return mRendererID; }

        virtual uint64_t GetHash() const { return (uint64_t)mRendererID; }
    private:
        uint32_t mRendererID;
        ImageFormat mFormat;
        uint32_t mWidth, mHeight;

        TextureProperties mProperties;

        Buffer<void> mLocalStorage;

        std::string mFilePath;
    };
}