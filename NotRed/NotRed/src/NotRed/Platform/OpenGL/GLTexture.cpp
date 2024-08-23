#include "nrpch.h"
#include "GLTexture.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Renderer/Renderer.h"

#include <filesystem>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace NR
{
    static GLenum ToOpenGLTextureFormat(TextureFormat format)
    {
        switch (format)
        {
        case NR::TextureFormat::RGB:     return GL_RGB;
        case NR::TextureFormat::RGBA:    return GL_RGBA;
        case NR::TextureFormat::Float16: return GL_RGBA16F;
        default:
        {
            NR_CORE_ASSERT(false, "Unknown texture format!");
            return 0;
        }
        }
    }

    // Texture2D-----------------------------------------------------------------------

    GLTexture2D::GLTexture2D(TextureFormat format, uint32_t width, uint32_t height, TextureWrap wrap)
        : mFormat(format), mWidth(width), mHeight(height), mWrap(wrap)
    {
        Ref<GLTexture2D> instance = this;
        Renderer::Submit([instance]() mutable {
            glGenTextures(1, &instance->mID);
            glBindTexture(GL_TEXTURE_2D, instance->mID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            GLenum wrapper = instance->mWrap == TextureWrap::Clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapper);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapper);
            glTextureParameterf(instance->mID, GL_TEXTURE_MAX_ANISOTROPY, RendererAPI::GetCapabilities().MaxAnisotropy);

            glTexImage2D(GL_TEXTURE_2D, 0, ToOpenGLTextureFormat(instance->mFormat), instance->mWidth, instance->mHeight, 0, ToOpenGLTextureFormat(instance->mFormat), GL_UNSIGNED_BYTE, nullptr);

            glBindTexture(GL_TEXTURE_2D, 0);
            });

        mImageData.Allocate(width * height * Texture::GetBPP(mFormat));
    }

    GLTexture2D::GLTexture2D(const std::string& path, bool standardRGB)
        : mFilePath(path)
    {
        int width, height, channels;
        if (stbi_is_hdr(path.c_str()))
        {
            NR_CORE_INFO("Loading HDR texture {0}, srgb = {1}", path, standardRGB);
            mImageData.Data = (byte*)stbi_loadf(path.c_str(), &width, &height, &channels, 0);
            NR_CORE_ASSERT(mImageData.Data, "Could not read image!");
            mIsHDR = true;
            mFormat = TextureFormat::Float16;
        }
        else
        {
            NR_CORE_INFO("Loading texture {0}, srgb = {1}", path, standardRGB);
            mImageData.Data = stbi_load(path.c_str(), &width, &height, &channels, standardRGB ? STBI_rgb : STBI_rgb_alpha);
            NR_CORE_ASSERT(mImageData.Data, "Could not read image!");
            mFormat = TextureFormat::RGBA;
        }

        mLoaded = mImageData.Data != nullptr;

        mWidth = width;
        mHeight = height;

        Ref<GLTexture2D> instance = this;
        Renderer::Submit([instance, standardRGB]() mutable {
            if (standardRGB)
            {
                glCreateTextures(GL_TEXTURE_2D, 1, &instance->mID);
                int levels = Texture::CalculateMipMapCount(instance->mWidth, instance->mHeight);
                glTextureStorage2D(instance->mID, levels, GL_SRGB8, instance->mWidth, instance->mHeight);
                glTextureParameteri(instance->mID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
                glTextureParameteri(instance->mID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTextureSubImage2D(instance->mID, 0, 0, 0, instance->mWidth, instance->mHeight, GL_RGB, GL_UNSIGNED_BYTE, instance->mImageData.Data);
                glGenerateTextureMipmap(instance->mID);
            }
            else
            {
                glGenTextures(1, &instance->mID);
                glBindTexture(GL_TEXTURE_2D, instance->mID);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

                GLenum internalFormat = ToOpenGLTextureFormat(instance->mFormat);
                GLenum format = standardRGB ? GL_SRGB8 : (instance->mIsHDR ? GL_RGB : ToOpenGLTextureFormat(instance->mFormat)); // HDR = GL_RGB for now
                GLenum type = internalFormat == GL_RGBA16F ? GL_FLOAT : GL_UNSIGNED_BYTE;
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, instance->mWidth, instance->mHeight, 0, format, type, instance->mImageData.Data);
                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);
            }
            stbi_image_free(instance->mImageData.Data);
            });
    }

    GLTexture2D::~GLTexture2D()
    {
        GLuint rendererID = mID;
        Renderer::Submit([rendererID]() {
            glDeleteTextures(1, &rendererID);
            });
    }

    void GLTexture2D::Bind(uint32_t slot) const
    {
        Ref<const GLTexture2D> instance = this;
        Renderer::Submit([instance, slot]() {
            glBindTextureUnit(slot, instance->mID);
            });
    }

    void GLTexture2D::Lock()
    {
        mLocked = true;
    }

    void GLTexture2D::Unlock()
    {
        mLocked = false;
        Ref<GLTexture2D> instance = this;
        Renderer::Submit([instance]() {
            glTextureSubImage2D(instance->mID, 0, 0, 0, instance->mWidth, instance->mHeight, ToOpenGLTextureFormat(instance->mFormat), GL_UNSIGNED_BYTE, instance->mImageData.Data);
            });
    }

    void GLTexture2D::Resize(uint32_t width, uint32_t height)
    {
        NR_CORE_ASSERT(mLocked, "Texture must be locked!");

        mImageData.Allocate(width * height * Texture::GetBPP(mFormat));

#if NR_DEBUG
        mImageData.ZeroInitialize();
#endif
    }

    Buffer GLTexture2D::GetWriteableBuffer()
    {
        NR_CORE_ASSERT(mLocked, "Texture must be locked!");
        return mImageData;
    }

    uint32_t GLTexture2D::GetMipLevelCount() const
    {
        return Texture::CalculateMipMapCount(mWidth, mHeight);
    }


    // TextureCube-----------------------------------------------------------------------

    GLTextureCube::GLTextureCube(TextureFormat format, uint32_t width, uint32_t height)
    {
        mWidth = width;
        mHeight = height;
        mFormat = format;

        uint32_t levels = Texture::CalculateMipMapCount(width, height);

        Ref<GLTextureCube> instance = this;
        Renderer::Submit([instance, levels]() mutable {
            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &instance->mID);
            glTextureStorage2D(instance->mID, levels, ToOpenGLTextureFormat(instance->mFormat), instance->mWidth, instance->mHeight);
            glTextureParameteri(instance->mID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTextureParameteri(instance->mID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            });
    }

    GLTextureCube::GLTextureCube(const std::string& path)
        : mFilePath(path)
    {
        static std::filesystem::path modelsDirectory = std::filesystem::current_path().parent_path() /
            "NotEditor" / path;
        int width, height, channels;
        stbi_set_flip_vertically_on_load(false);
        mImageData = stbi_load(modelsDirectory.string().c_str(), &width, &height, &channels, STBI_rgb);

        mWidth = width;
        mHeight = height;
        mFormat = TextureFormat::RGB;

        uint32_t faceWidth = mWidth / 4;
        uint32_t faceHeight = mHeight / 3;
        NR_CORE_ASSERT(faceWidth == faceHeight, "Non-square faces!");

        std::array<uint8_t*, 6> faces;
        for (size_t i = 0; i < faces.size(); ++i)
        {
            faces[i] = new uint8_t[faceWidth * faceHeight * 3]; // 3 BPP
        }

        int faceIndex = 0;

        for (size_t i = 0; i < 4; ++i)
        {
            for (size_t y = 0; y < faceHeight; ++y)
            {
                size_t yOffset = y + faceHeight;
                for (size_t x = 0; x < faceWidth; ++x)
                {
                    size_t xOffset = x + i * faceWidth;
                    faces[faceIndex][(x + y * faceWidth) * 3 + 0] = mImageData[(xOffset + yOffset * mWidth) * 3 + 0];
                    faces[faceIndex][(x + y * faceWidth) * 3 + 1] = mImageData[(xOffset + yOffset * mWidth) * 3 + 1];
                    faces[faceIndex][(x + y * faceWidth) * 3 + 2] = mImageData[(xOffset + yOffset * mWidth) * 3 + 2];
                }
            }
            ++faceIndex;
        }

        for (size_t i = 0; i < 3; ++i)
        {
            // Skip the middle one
            if (i == 1)
            {
                continue;
            }

            for (size_t y = 0; y < faceHeight; ++y)
            {
                size_t yOffset = y + i * faceHeight;
                for (size_t x = 0; x < faceWidth; ++x)
                {
                    size_t xOffset = x + faceWidth;
                    faces[faceIndex][(x + y * faceWidth) * 3 + 0] = mImageData[(xOffset + yOffset * mWidth) * 3 + 0];
                    faces[faceIndex][(x + y * faceWidth) * 3 + 1] = mImageData[(xOffset + yOffset * mWidth) * 3 + 1];
                    faces[faceIndex][(x + y * faceWidth) * 3 + 2] = mImageData[(xOffset + yOffset * mWidth) * 3 + 2];
                }
            }
            faceIndex++;
        }

        Ref<GLTextureCube> instance = this;
        Renderer::Submit([instance, faceWidth, faceHeight, faces]() mutable {
            glGenTextures(1, &instance->mID);
            glBindTexture(GL_TEXTURE_CUBE_MAP, instance->mID);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTextureParameterf(instance->mID, GL_TEXTURE_MAX_ANISOTROPY, RendererAPI::GetCapabilities().MaxAnisotropy);

            auto format = ToOpenGLTextureFormat(instance->mFormat);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[2]);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[0]);

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[4]);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[5]);

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[1]);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[3]);

            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

            glBindTexture(GL_TEXTURE_2D, 0);

            for (size_t i = 0; i < faces.size(); ++i)
            {
                delete[] faces[i];
            }

            stbi_image_free(instance->mImageData);
            });
    }

    GLTextureCube::~GLTextureCube()
    {
        GLuint rendererID = mID;
        Renderer::Submit([rendererID]() {
            glDeleteTextures(1, &rendererID);
            });
    }

    void GLTextureCube::Bind(uint32_t slot) const
    {
        Ref<const GLTextureCube> instance = this;
        Renderer::Submit([instance, slot]() {
            glBindTextureUnit(slot, instance->mID);
            });
    }

    uint32_t GLTextureCube::GetMipLevelCount() const
    {
        return Texture::CalculateMipMapCount(mWidth, mHeight);
    }
}