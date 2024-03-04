#include "nrpch.h"
#include "GLTexture.h"

#include <glad/glad.h>

#include "stb_image.h"

namespace NR
{
    GLTexture2D::GLTexture2D(uint32_t width, uint32_t height)
        :mWidth(width), mHeight(height)
    {
        GLenum glFormat = GL_RGBA8;
        mDataFormat = GL_RGBA;

        glCreateTextures(GL_TEXTURE_2D, 1, &mID);
        glTextureStorage2D(mID, 1, glFormat, mWidth, mHeight);

        glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureParameteri(mID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(mID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    GLTexture2D::GLTexture2D(const std::string& path)
    {
        int width, height, channnels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channnels, 0);

        NR_CORE_ASSERT(data, "Failed to load image!");
        mWidth = width;
        mHeight = height;

        GLenum glFormat = 0;
        mDataFormat = 0;
        switch (channnels)
        {
        case 4:
        {
            glFormat = GL_RGBA8;
            mDataFormat = GL_RGBA;
            break;
        }
        case 3:
        {
            glFormat = GL_RGB8;
            mDataFormat = GL_RGB;
            break;
        }
        }

        NR_CORE_ASSERT(glFormat & mDataFormat, "Not supported format!");

        glCreateTextures(GL_TEXTURE_2D, 1, &mID);
        glTextureStorage2D(mID, 1, glFormat, mWidth, mHeight);

        glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureParameteri(mID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(mID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, mDataFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    GLTexture2D::~GLTexture2D()
    {
        glDeleteTextures(1, &mID);
    }

    void GLTexture2D::SetData(void* data, uint32_t size)
    {
        uint32_t bpp = mDataFormat == GL_RGBA ? 4 : 3;
        NR_CORE_ASSERT(size == mWidth * mHeight * bpp, "Data must be entire texture!");
        glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, mDataFormat, GL_UNSIGNED_BYTE, data);
    }

    void GLTexture2D::Bind(uint32_t slot) const
    {
        glBindTextureUnit(slot, mID);
    }
}