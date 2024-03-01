#include "nrpch.h"
#include "GLTexture.h"

#include <glad/glad.h>

#include "stb_image.h"

namespace NR
{
    GLTexture2D::GLTexture2D(const std::string& path)
    {
        int width, height, channnels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channnels, 0);

        NR_CORE_ASSERT(data, "Failed to load image!");
        mWidth = width;
        mHeight = height;

        glCreateTextures(GL_TEXTURE_2D, 1, &mID);
        glTextureStorage2D(mID, 1, GL_RGB8, mWidth, mHeight);

        glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    GLTexture2D::~GLTexture2D()
    {
        glDeleteTextures(1, &mID);
    }

    void GLTexture2D::Bind(uint32_t slot) const
    {
        glBindTextureUnit(slot, mID);
    }
}