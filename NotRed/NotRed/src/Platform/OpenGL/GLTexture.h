#pragma once

#include "NotRed/Renderer/Texture.h"

namespace NR
{
    class GLTexture2D : public Texture2D
    {
    public:
        GLTexture2D(uint32_t width, uint32_t height);
        GLTexture2D(const std::string& path);
        ~GLTexture2D() override;

        uint32_t GetWidth() const override { return mWidth; }
        uint32_t GetHeight() const override { return mHeight; }

        void SetData(void* data, uint32_t size) override;

        void Bind(uint32_t slot = 0) const override;

        bool operator== (const Texture& other) const override 
        { 
            return mID == ((GLTexture2D&)other).mID; 
        }

    private:
        uint32_t mWidth, mHeight;
        uint32_t mID;
        uint32_t mDataFormat;
    };
}