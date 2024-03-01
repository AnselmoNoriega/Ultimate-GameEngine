#pragma once

#include "NotRed/Renderer/Texture.h"

namespace NR
{
    class GLTexture2D : public Texture2D
    {
    public:
        GLTexture2D(const std::string& path);
        ~GLTexture2D() override;

        uint32_t GetWidth() const override { return mWidth; }
        uint32_t GetHeight() const override { return mHeight; }

        void Bind(uint32_t slot = 0) const override;

    private:
        uint32_t mWidth, mHeight;
        uint32_t mID;
    };
}