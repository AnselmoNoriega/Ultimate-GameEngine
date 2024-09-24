#include "nrpch.h"
#include "GLTexture.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Renderer/Renderer.h"

#include <filesystem>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "GLImage.h"

namespace NR
{
	// Texture2D-----------------------------------------------------------------------

	GLTexture2D::GLTexture2D(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
		: mWidth(width), mHeight(height), mProperties(properties)
	{
		ImageSpecification spec;
		spec.Format = format;
		spec.Width = width;
		spec.Height = height;
		mImage = Image2D::Create(spec);

		Renderer::Submit([=]()
			{
				mImage->Invalidate();
			});
	}

	GLTexture2D::GLTexture2D(const std::string& path, TextureProperties properties)
		: mFilePath(path), mProperties(properties)
	{
		int width, height, channels;
		if (stbi_is_hdr(path.c_str()))
		{
			NR_CORE_INFO("Loading HDR texture {0}, srgb = {1}", path, properties.StandardRGB);

			float* imageData = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			NR_CORE_ASSERT(imageData);
			Buffer buffer(imageData, Utils::GetImageMemorySize(ImageFormat::RGBA32F, width, height));

			ImageSpecification spec;
			spec.Format = ImageFormat::RGBA32F;
			spec.Width = width;
			spec.Height = height;
			mImage = Image2D::Create(spec, buffer);
		}
		else
		{
			NR_CORE_INFO("Loading texture {0}, srgb = {1}", path, properties.StandardRGB);

			stbi_uc* imageData = stbi_load(path.c_str(), &width, &height, &channels, properties.StandardRGB ? STBI_rgb : STBI_rgb_alpha);
			NR_CORE_ASSERT(imageData);
			ImageFormat format = properties.StandardRGB ? ImageFormat::RGB : ImageFormat::RGBA;
			Buffer buffer(imageData, Utils::GetImageMemorySize(format, width, height));

			ImageSpecification spec;
			spec.Format = format;
			spec.Width = width;
			spec.Height = height;
			mImage = Image2D::Create(spec, buffer);
		}

		mImage.As<GLImage2D>()->CreateSampler(mProperties);

		mWidth = width;
		mHeight = height;
		mLoaded = true;

		Ref<Image2D>& image = mImage;
		Renderer::Submit([image]() mutable
			{
				image->Invalidate();

				Buffer& buffer = image->GetBuffer();
				stbi_image_free(buffer.Data);
				buffer = Buffer();
			});
	}

	GLTexture2D::~GLTexture2D()
	{
		Ref<Image2D> image = mImage;
		Renderer::Submit([image]() mutable {
			image->Release();
			});
	}

	void GLTexture2D::Bind(uint32_t slot) const
	{
		Ref<GLImage2D> image = mImage.As<GLImage2D>();
		Renderer::Submit([slot, image]() {
			glBindTextureUnit(slot, image->GetRendererID());
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
		Ref<GLImage2D> image = mImage.As<GLImage2D>();
		Renderer::Submit([instance, image]() mutable {

			glTextureSubImage2D(
				image->GetRendererID(),
				0, 0, 0,
				instance->mWidth, instance->mHeight,
				Utils::OpenGLImageFormat(image->GetSpecification().Format),
				GL_UNSIGNED_BYTE,
				instance->mImage->GetBuffer().Data
			);

			});
	}

	Buffer GLTexture2D::GetWriteableBuffer()
	{
		NR_CORE_ASSERT(mLocked, "Texture must be locked!");
		return mImage->GetBuffer();
	}

	uint32_t GLTexture2D::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(mWidth, mHeight);
	}

	// TextureCube-----------------------------------------------------------------------

	GLTextureCube::GLTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
		: mWidth(width), mHeight(height), mFormat(format), mProperties(properties)
	{
		if (data)
		{
			uint32_t size = width * height * 4 * 6; // six layers
			mLocalStorage = Buffer::Copy(data, size);
		}

		uint32_t levels = Utils::CalculateMipCount(width, height);

		Ref<GLTextureCube> instance = this;
		Renderer::Submit([instance, levels]() mutable {
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &instance->mID);
			glTextureStorage2D(instance->mID, levels, Utils::OpenGLImageInternalFormat(instance->mFormat), instance->mWidth, instance->mHeight);
			if (instance->mLocalStorage.Data)
			{
				glTextureSubImage3D(
					instance->mID, 0, 0, 0, 0,
					instance->mWidth, instance->mHeight, 6,
					Utils::OpenGLImageFormat(instance->mFormat),
					Utils::OpenGLFormatDataType(instance->mFormat), instance->mLocalStorage.Data);
			}

			glTextureParameteri(instance->mID, GL_TEXTURE_MIN_FILTER, Utils::OpenGLSamplerFilter(instance->mProperties.SamplerFilter, instance->mProperties.GenerateMips));
			glTextureParameteri(instance->mID, GL_TEXTURE_MAG_FILTER, Utils::OpenGLSamplerFilter(instance->mProperties.SamplerFilter, false));
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, Utils::OpenGLSamplerWrap(instance->mProperties.SamplerWrap));
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, Utils::OpenGLSamplerWrap(instance->mProperties.SamplerWrap));
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, Utils::OpenGLSamplerWrap(instance->mProperties.SamplerWrap));
			});
	}
	GLTextureCube::GLTextureCube(const std::string& path, TextureProperties properties)
		: mFilePath(path), mProperties(properties)
	{
		NR_CORE_ASSERT(false, "Not yet set!");
#if 0
		static std::filesystem::path modelsDirectory = std::filesystem::current_path().parent_path() /
			"NotEditor" / path;
		int width, height, channels;
		stbi_set_flip_vertically_on_load(false);
		mLocalStorage.Data = stbi_load(modelsDirectory.string().c_str(), &width, &height, &channels, STBI_rgb);

		mWidth = width;
		mHeight = height;
		mFormat = ImageFormat::RGB;

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
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
			glTextureParameterf(instance->mID, GL_TEXTURE_MAX_ANISOTROPY, Renderer::GetCapabilities().MaxAnisotropy);

			auto format = Utils::OpenGLImageFormat(instance->mFormat);
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
#endif
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
		return Utils::CalculateMipCount(mWidth, mHeight);
	}
	}