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
		Renderer::Submit([this](){
			glGenTextures(1, &mID);
			glBindTexture(GL_TEXTURE_2D, mID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GLenum wrapper = mWrap == TextureWrap::Clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapper);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapper);
			glTextureParameterf(mID, GL_TEXTURE_MAX_ANISOTROPY, RendererAPI::GetCapabilities().MaxAnisotropy);

			glTexImage2D(GL_TEXTURE_2D, 0, ToOpenGLTextureFormat(mFormat), mWidth, mHeight, 0, ToOpenGLTextureFormat(mFormat), GL_UNSIGNED_BYTE, nullptr);

			glBindTexture(GL_TEXTURE_2D, 0);
			});

		mImageData.Allocate(width* height* Texture::GetBPP(mFormat));
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

		Renderer::Submit([=](){
			if (standardRGB)
			{
				glCreateTextures(GL_TEXTURE_2D, 1, &mID);
				int levels = Texture::CalculateMipMapCount(mWidth, mHeight);
				glTextureStorage2D(mID, levels, GL_SRGB8, mWidth, mHeight);
				glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
				glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, mImageData.Data);
				glGenerateTextureMipmap(mID);
			}
			else
			{
				glGenTextures(1, &mID);
				glBindTexture(GL_TEXTURE_2D, mID);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				GLenum internalFormat = ToOpenGLTextureFormat(mFormat);
				GLenum format = standardRGB ? GL_SRGB8 : (mIsHDR ? GL_RGB : ToOpenGLTextureFormat(mFormat)); // HDR = GL_RGB for now
				GLenum type = internalFormat == GL_RGBA16F ? GL_FLOAT : GL_UNSIGNED_BYTE;
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, mWidth, mHeight, 0, format, type, mImageData.Data); 
				glGenerateMipmap(GL_TEXTURE_2D);

				glBindTexture(GL_TEXTURE_2D, 0);
			}
			stbi_image_free(mImageData.Data);
			});
	}

	GLTexture2D::~GLTexture2D()
	{
		Renderer::Submit([this]() {
			glDeleteTextures(1, &mID);
			});
	}

	void GLTexture2D::Bind(uint32_t slot) const
	{
		Renderer::Submit([this, slot]() {
			glBindTextureUnit(slot, mID);
			});
	}

	void GLTexture2D::Lock()
	{
		mLocked = true;
	}

	void GLTexture2D::Unlock()
	{
		mLocked = false;
		Renderer::Submit([this]() {
			glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, ToOpenGLTextureFormat(mFormat), GL_UNSIGNED_BYTE, mImageData.Data);
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

		Renderer::Submit([=]() {
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &mID);
			glTextureStorage2D(mID, levels, ToOpenGLTextureFormat(mFormat), width, height);
			glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

		std::array<unsigned char*, 6> faces;
		for (size_t i = 0; i < faces.size(); ++i)
		{
			faces[i] = new unsigned char[faceWidth * faceHeight * 3]; // 3 BPP
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

		Renderer::Submit([=]() {
			glGenTextures(1, &mID);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mID);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTextureParameterf(mID, GL_TEXTURE_MAX_ANISOTROPY, RendererAPI::GetCapabilities().MaxAnisotropy);

			auto format = ToOpenGLTextureFormat(mFormat);
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

			stbi_image_free(mImageData);
			});
	}

	GLTextureCube::~GLTextureCube()
	{
		auto self = this;
		Renderer::Submit([this]() {
			glDeleteTextures(1, &mID);
			});
	}

	void GLTextureCube::Bind(uint32_t slot) const
	{
		Renderer::Submit([this, slot]() {
			glBindTextureUnit(slot, mID);
			});
	}

	uint32_t GLTextureCube::GetMipLevelCount() const
	{
		return Texture::CalculateMipMapCount(mWidth, mHeight);
	}
}