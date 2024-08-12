#include "nrpch.h"
#include "GLTexture.h"

#include <glad/glad.h>

#include "stb_image.h"

namespace NR
{
    namespace Utils
    {
        static GLenum ImageFormatToGLInternalFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGB8:  return GL_RGB8;
            case ImageFormat::RGBA8: return GL_RGBA8;
            default:
            {
                NR_CORE_ASSERT(false, "Invalid Format");
                return 0;
            }
            }
        }

        static GLenum ImageFormatToGLDataFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGB8:  return GL_RGB;
            case ImageFormat::RGBA8: return GL_RGBA;
            default:
            {
                NR_CORE_ASSERT(false, "Invalid Format");
                return 0;
            }
            }
        }
    }

    GLTexture2D::GLTexture2D(const TextureSpecification& specification)
        :mSpecification(specification), mWidth(mSpecification.Width), mHeight(mSpecification.Height)
    {
        NR_PROFILE_FUNCTION();

        GLenum glFormat = Utils::ImageFormatToGLInternalFormat(mSpecification.Format);
        mDataFormat = Utils::ImageFormatToGLDataFormat(mSpecification.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &mID);
        glTextureStorage2D(mID, 1, glFormat, mWidth, mHeight);

        glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureParameteri(mID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(mID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    GLTexture2D::GLTexture2D(const std::string& path)
        :mPath(path)
    {
        NR_PROFILE_FUNCTION();

        int width, height, channnels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* data = nullptr;
        {
            NR_PROFILE_SCOPE("stbi_load - GLTexture2D::GLTexture2D(const std::string&)");
            data = stbi_load(path.c_str(), &width, &height, &channnels, 0);
        }

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
        NR_PROFILE_FUNCTION();

        glDeleteTextures(1, &mID);
    }

    void GLTexture2D::SetData(void* data, uint32_t size)
    {
        NR_PROFILE_FUNCTION();

        uint32_t bpp = mDataFormat == GL_RGBA ? 4 : 3;
        NR_CORE_ASSERT(size == mWidth * mHeight * bpp, "Data must be entire texture!");
        glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, mDataFormat, GL_UNSIGNED_BYTE, data);
    }

    void GLTexture2D::Bind(uint32_t slot) const
    {
        NR_PROFILE_FUNCTION();

        glBindTextureUnit(slot, mID);
    }

    //////////////////////////////////////////////////////////////
    // TextureCube
    /////////////////////////////////////////////////////////////

	GLTextureCube::GLTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
		: mWidth(width), mHeight(height), mFormat(format), mProperties(properties)
	{
	}

    GLTextureCube::GLTextureCube(const std::string& path, TextureProperties properties)
		: m_FilePath(path), m_Properties(properties)
	{
		NR_CORE_ASSERT(false, "Not supported yet!");
	}

    GLTextureCube::~GLTextureCube()
	{
		GLuint rendererID = m_RendererID;
		Renderer::Submit([rendererID]() {
			glDeleteTextures(1, &rendererID);
			});
	}

	void GLTextureCube::Bind(uint32_t slot) const
	{
		Ref<const OpenGLTextureCube> instance = this;
		Renderer::Submit([instance, slot]() {
			glBindTextureUnit(slot, instance->m_RendererID);
			});
	}

	uint32_t GLTextureCube::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Width, m_Height);
	}
}