#include "nrpch.h"
#include "Renderer2D.h"

#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <codecvt>

// TEMP
#include "NotRed/Platform/Vulkan/VKRenderCommandBuffer.h"

#include "NotRed/Renderer/UI/MSDFData.h"

namespace NR
{
	Renderer2D::Renderer2D()
	{
		Init();
	}

	Renderer2D::~Renderer2D()
	{
		Shutdown();
	}

	void Renderer2D::Init()
	{
		mRenderCommandBuffer = RenderCommandBuffer::Create(0, "Renderer2D");

		FrameBufferSpecification frameBufferSpec;
		frameBufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
		frameBufferSpec.Samples = 1;
		frameBufferSpec.ClearOnLoad = false;
		frameBufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };
		frameBufferSpec.DebugName = "Renderer2D FrameBuffer";

		Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(frameBufferSpec);

		RenderPassSpecification renderPassSpec;
		renderPassSpec.TargetFrameBuffer = frameBuffer;
		renderPassSpec.DebugName = "Renderer2D";
		Ref<RenderPass> renderPass = RenderPass::Create(renderPassSpec);

		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Quad";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D");
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float4, "aColor" },
				{ ShaderDataType::Float2, "aTexCoord" },
				{ ShaderDataType::Float, "aTexIndex" },
				{ ShaderDataType::Float, "aTilingFactor" }
			};
			mQuadPipeline = Pipeline::Create(pipelineSpecification);

			mQuadVertexBuffer = VertexBuffer::Create(MaxVertices * sizeof(QuadVertex));
			mQuadVertexBufferBase = new QuadVertex[MaxVertices];

			uint32_t* quadIndices = new uint32_t[MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;

				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			mQuadIndexBuffer = IndexBuffer::Create(quadIndices, MaxIndices);
			delete[] quadIndices;
		}

		mWhiteTexture = Renderer::GetWhiteTexture();

		// Set all texture slots to 0
		mTextureSlots[0] = mWhiteTexture;

		mQuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		mQuadVertexPositions[1] = { -0.5f,  0.5f, 0.0f, 1.0f };
		mQuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		mQuadVertexPositions[3] = { 0.5f, -0.5f, 0.0f, 1.0f };

		// Lines
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Line";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Line");
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.Topology = PrimitiveTopology::Lines;
			pipelineSpecification.LineWidth = 2.0f;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float4, "aColor" }
			};
			mLinePipeline = Pipeline::Create(pipelineSpecification);
			pipelineSpecification.DepthTest = false;
			mLineOnTopPipeline = Pipeline::Create(pipelineSpecification);

			mLineVertexBuffer = VertexBuffer::Create(MaxLineVertices * sizeof(LineVertex));
			mLineVertexBufferBase = new LineVertex[MaxLineVertices];

			uint32_t* lineIndices = new uint32_t[MaxLineIndices];
			for (uint32_t i = 0; i < MaxLineIndices; ++i)
				lineIndices[i] = i;

			mLineIndexBuffer = IndexBuffer::Create(lineIndices, MaxLineIndices);
			delete[] lineIndices;
		}

		// Text
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Text";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Text");
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float4, "aColor" },
				{ ShaderDataType::Float2, "aTexCoord" },
				{ ShaderDataType::Float, "aTexIndex" }
			};

			mTextPipeline = Pipeline::Create(pipelineSpecification);
			mTextMaterial = Material::Create(pipelineSpecification.Shader);
			mTextVertexBuffer = VertexBuffer::Create(MaxVertices * sizeof(TextVertex));
			mTextVertexBufferBase = new TextVertex[MaxVertices];

			uint32_t* textQuadIndices = new uint32_t[MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < MaxIndices; i += 6)
			{
				textQuadIndices[i + 0] = offset + 0;
				textQuadIndices[i + 1] = offset + 1;
				textQuadIndices[i + 2] = offset + 2;

				textQuadIndices[i + 3] = offset + 2;
				textQuadIndices[i + 4] = offset + 3;
				textQuadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			mTextIndexBuffer = IndexBuffer::Create(textQuadIndices, MaxIndices);
			delete[] textQuadIndices;
		}

		// Circles
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Circle";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Circle");
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aWorldPosition" },
				{ ShaderDataType::Float,  "aThickness" },
				{ ShaderDataType::Float2, "aLocalPosition" },
				{ ShaderDataType::Float4, "aColor" }
			};
			mCirclePipeline = Pipeline::Create(pipelineSpecification);
			mCircleMaterial = Material::Create(pipelineSpecification.Shader);

			mCircleVertexBuffer = VertexBuffer::Create(MaxVertices * sizeof(QuadVertex));
			mCircleVertexBufferBase = new CircleVertex[MaxVertices];
		}

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		mUniformBufferSet = UniformBufferSet::Create(framesInFlight);
		mUniformBufferSet->Create(sizeof(UBCamera), 0);

		mQuadMaterial = Material::Create(mQuadPipeline->GetSpecification().Shader, "QuadMaterial");
		mLineMaterial = Material::Create(mLinePipeline->GetSpecification().Shader, "LineMaterial");

	}

	void Renderer2D::Shutdown()
	{
	}

	void Renderer2D::BeginScene(const glm::mat4& viewProj, const glm::mat4& view, bool depthTest)
	{
		mCameraViewProj = viewProj;
		mCameraView = view;
		mDepthTest = depthTest;

		Renderer::Submit([uniformBufferSet = mUniformBufferSet, viewProj]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				uniformBufferSet->Get(0, 0, bufferIndex)->RT_SetData(&viewProj, sizeof(UBCamera));
			});

		mQuadIndexCount = 0;
		mQuadVertexBufferPtr = mQuadVertexBufferBase;

		mTextIndexCount = 0;
		mTextVertexBufferPtr = mTextVertexBufferBase;

		mLineIndexCount = 0;
		mLineVertexBufferPtr = mLineVertexBufferBase;

		mCircleIndexCount = 0;
		mCircleVertexBufferPtr = mCircleVertexBufferBase;

		mTextureSlotIndex = 1;
		mFontTextureSlotIndex = 0;

		for (uint32_t i = 1; i < mTextureSlots.size(); ++i)
		{
			mTextureSlots[i] = nullptr;
		}

		for (uint32_t i = 0; i < mFontTextureSlots.size(); ++i)
		{
			mFontTextureSlots[i] = nullptr;
		}
	}

	void Renderer2D::EndScene()
	{
		mRenderCommandBuffer->Begin();
		Renderer::BeginRenderPass(mRenderCommandBuffer, mQuadPipeline->GetSpecification().RenderPass);

		// Lines
		uint32_t dataSize = (uint32_t)((uint8_t*)mQuadVertexBufferPtr - (uint8_t*)mQuadVertexBufferBase);
		if (dataSize)
		{
			mQuadVertexBuffer->SetData(mQuadVertexBufferBase, dataSize);

			for (uint32_t i = 0; i < mTextureSlots.size(); ++i)
			{
				if (mTextureSlots[i])
				{
					mQuadMaterial->Set("uTextures", mTextureSlots[i], i);
				}
				else
				{
					mQuadMaterial->Set("uTextures", mWhiteTexture, i);
				}
			}

			Renderer::RenderGeometry(mRenderCommandBuffer, mQuadPipeline, mUniformBufferSet, nullptr, mQuadMaterial, mQuadVertexBuffer, mQuadIndexBuffer, glm::mat4(1.0f), mQuadIndexCount);

			++mStats.DrawCalls;
		}

		// Circles
		dataSize = (uint32_t)((uint8_t*)mCircleVertexBufferPtr - (uint8_t*)mCircleVertexBufferBase);
		if (dataSize)
		{
			mCircleVertexBuffer->SetData(mCircleVertexBufferBase, dataSize);
			Renderer::RenderGeometry(mRenderCommandBuffer, mCirclePipeline, mUniformBufferSet, nullptr, mCircleMaterial, mCircleVertexBuffer, mQuadIndexBuffer, glm::mat4(1.0f), mCircleIndexCount);
			++mStats.DrawCalls;
		}

		// Render text
		dataSize = (uint32_t)((uint8_t*)mTextVertexBufferPtr - (uint8_t*)mTextVertexBufferBase);
		if (dataSize)
		{
			mTextVertexBuffer->SetData(mTextVertexBufferBase, dataSize);

			for (uint32_t i = 0; i < mFontTextureSlots.size(); ++i)
			{
				if (mFontTextureSlots[i])
				{
					mTextMaterial->Set("uFontAtlases", mFontTextureSlots[i], i);
				}
				else
				{
					mTextMaterial->Set("uFontAtlases", mWhiteTexture, i);
				}
			}

			Renderer::RenderGeometry(mRenderCommandBuffer, mTextPipeline, mUniformBufferSet, nullptr, mTextMaterial, mTextVertexBuffer, mTextIndexBuffer, glm::mat4(1.0f), mTextIndexCount);

			++mStats.DrawCalls;
		}

		dataSize = (uint32_t)((uint8_t*)mLineVertexBufferPtr - (uint8_t*)mLineVertexBufferBase);
		if (dataSize)
		{
			mLineVertexBuffer->SetData(mLineVertexBufferBase, dataSize);

			Renderer::Submit([lineWidth = mLineWidth, renderCommandBuffer = mRenderCommandBuffer]()
				{
					uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
					VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
					vkCmdSetLineWidth(commandBuffer, lineWidth);
				});
			Renderer::RenderGeometry(mRenderCommandBuffer, mLinePipeline, mUniformBufferSet, nullptr, mLineMaterial, mLineVertexBuffer, mLineIndexBuffer, glm::mat4(1.0f), mLineIndexCount);

			++mStats.DrawCalls;
		}

		Renderer::EndRenderPass(mRenderCommandBuffer);
		mRenderCommandBuffer->End();
		mRenderCommandBuffer->Submit();
	}

	void Renderer2D::Flush()
	{
	}

	Ref<RenderPass> Renderer2D::GetTargetRenderPass()
	{
		return mQuadPipeline->GetSpecification().RenderPass;
	}

	void Renderer2D::SetTargetRenderPass(Ref<RenderPass> renderPass)
	{
		if (renderPass != mQuadPipeline->GetSpecification().RenderPass)
		{
			{
				PipelineSpecification pipelineSpecification = mQuadPipeline->GetSpecification();
				pipelineSpecification.RenderPass = renderPass;
				mQuadPipeline = Pipeline::Create(pipelineSpecification);
			}

			{
				PipelineSpecification pipelineSpecification = mLinePipeline->GetSpecification();
				pipelineSpecification.RenderPass = renderPass;
				mLinePipeline = Pipeline::Create(pipelineSpecification);
			}

			{
				PipelineSpecification pipelineSpecification = mTextPipeline->GetSpecification();
				pipelineSpecification.RenderPass = renderPass;
				mTextPipeline = Pipeline::Create(pipelineSpecification);
			}
		}
	}

	void Renderer2D::FlushAndReset()
	{
		//EndScene();

		mQuadIndexCount = 0;
		mQuadVertexBufferPtr = mQuadVertexBufferBase;

		mTextureSlotIndex = 1;
	}

	void Renderer2D::FlushAndResetLines()
	{

	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (mQuadIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		for (size_t i = 0; i < quadVertexCount; ++i)
		{
			mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[i];
			mQuadVertexBufferPtr->Color = color;
			mQuadVertexBufferPtr->TexCoord = textureCoords[i];
			mQuadVertexBufferPtr->TexIndex = textureIndex;
			mQuadVertexBufferPtr->TilingFactor = tilingFactor;
			mQuadVertexBufferPtr++;
		}

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (mQuadIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < mTextureSlotIndex; ++i)
		{
			if (*mTextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (mTextureSlotIndex >= MaxTextureSlots)
			{
				FlushAndReset();
			}

			textureIndex = (float)mTextureSlotIndex;
			mTextureSlots[mTextureSlotIndex] = texture;
			mTextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; ++i)
		{
			mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[i];
			mQuadVertexBufferPtr->Color = color;
			mQuadVertexBufferPtr->TexCoord = textureCoords[i];
			mQuadVertexBufferPtr->TexIndex = textureIndex;
			mQuadVertexBufferPtr->TilingFactor = tilingFactor;
			mQuadVertexBufferPtr++;
		}

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (mQuadIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[0];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[1];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[2];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[3];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		if (mQuadIndexCount >= MaxIndices)
			FlushAndReset();

		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < mTextureSlotIndex; ++i)
		{
			if (*mTextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)mTextureSlotIndex;
			mTextureSlots[mTextureSlotIndex] = texture;
			mTextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[0];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[1];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[2];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[3];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (mQuadIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::vec3 camRightWS = { mCameraView[0][0], mCameraView[1][0], mCameraView[2][0] };
		glm::vec3 camUpWS = { mCameraView[0][1], mCameraView[1][1], mCameraView[2][1] };

		mQuadVertexBufferPtr->Position = position + camRightWS * (mQuadVertexPositions[0].x) * size.x + camUpWS * mQuadVertexPositions[0].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = position + camRightWS * mQuadVertexPositions[1].x * size.x + camUpWS * mQuadVertexPositions[1].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = position + camRightWS * mQuadVertexPositions[2].x * size.x + camUpWS * mQuadVertexPositions[2].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = position + camRightWS * mQuadVertexPositions[3].x * size.x + camUpWS * mQuadVertexPositions[3].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		if (mQuadIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < mTextureSlotIndex; ++i)
		{
			if (*mTextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)mTextureSlotIndex;
			mTextureSlots[mTextureSlotIndex] = texture;
			mTextureSlotIndex++;
		}

		glm::vec3 camRightWS = { mCameraView[0][0], mCameraView[1][0], mCameraView[2][0] };
		glm::vec3 camUpWS = { mCameraView[0][1], mCameraView[1][1], mCameraView[2][1] };

		mQuadVertexBufferPtr->Position = position + camRightWS * (mQuadVertexPositions[0].x) * size.x + camUpWS * mQuadVertexPositions[0].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = position + camRightWS * mQuadVertexPositions[1].x * size.x + camUpWS * mQuadVertexPositions[1].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = position + camRightWS * mQuadVertexPositions[2].x * size.x + camUpWS * mQuadVertexPositions[2].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = position + camRightWS * mQuadVertexPositions[3].x * size.x + camUpWS * mQuadVertexPositions[3].y * size.y;
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		if (mQuadIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[0];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[1];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[2];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[3];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		if (mQuadIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < mTextureSlotIndex; ++i)
		{
			if (*mTextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)mTextureSlotIndex;
			mTextureSlots[mTextureSlotIndex] = texture;
			mTextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[0];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[1];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[2];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadVertexBufferPtr->Position = transform * mQuadVertexPositions[3];
		mQuadVertexBufferPtr->Color = color;
		mQuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		mQuadVertexBufferPtr->TexIndex = textureIndex;
		mQuadVertexBufferPtr->TilingFactor = tilingFactor;
		mQuadVertexBufferPtr++;

		mQuadIndexCount += 6;

		mStats.QuadCount++;
	}

	void Renderer2D::DrawRotatedRect(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedRect({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedRect(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		if (mLineIndexCount >= MaxLineIndices)
		{
			FlushAndResetLines();
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		glm::vec3 positions[4] =
		{
			transform * mQuadVertexPositions[0],
			transform * mQuadVertexPositions[1],
			transform * mQuadVertexPositions[2],
			transform * mQuadVertexPositions[3]
		};

		for (int i = 0; i < 4; ++i)
		{
			auto& v0 = positions[i];
			auto& v1 = positions[(i + 1) % 4];

			mLineVertexBufferPtr->Position = v0;
			mLineVertexBufferPtr->Color = color;
			mLineVertexBufferPtr++;

			mLineVertexBufferPtr->Position = v1;
			mLineVertexBufferPtr->Color = color;
			mLineVertexBufferPtr++;

			mLineIndexCount += 2;
			mStats.LineCount++;
		}
	}

	void Renderer2D::FillCircle(const glm::vec2& position, float radius, const glm::vec4& color, float thickness)
	{
		FillCircle({ position.x, position.y, 0.0f }, radius, color, thickness);
	}

	void Renderer2D::FillCircle(const glm::vec3& position, float radius, const glm::vec4& color, float thickness)
	{
		if (mCircleIndexCount >= MaxIndices)
		{
			FlushAndReset();
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { radius * 2.0f, radius * 2.0f, 1.0f });

		for (int i = 0; i < 4; ++i)
		{
			mCircleVertexBufferPtr->WorldPosition = transform * mQuadVertexPositions[i];
			mCircleVertexBufferPtr->Thickness = thickness;
			mCircleVertexBufferPtr->LocalPosition = mQuadVertexPositions[i] * 2.0f;
			mCircleVertexBufferPtr->Color = color;
			mCircleVertexBufferPtr++;

			mCircleIndexCount += 6;
			mStats.QuadCount++;
		}

	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		if (mLineIndexCount >= MaxLineIndices)
			FlushAndResetLines();

		mLineVertexBufferPtr->Position = p0;
		mLineVertexBufferPtr->Color = color;
		mLineVertexBufferPtr++;

		mLineVertexBufferPtr->Position = p1;
		mLineVertexBufferPtr->Color = color;
		mLineVertexBufferPtr++;

		mLineIndexCount += 2;

		mStats.LineCount++;
	}

	void Renderer2D::DrawCircle(const glm::vec3& position, const glm::vec3& rotation, float radius, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation.x, { 1.0f, 0.0f, 0.0f })
			* glm::rotate(glm::mat4(1.0f), rotation.y, { 0.0f, 1.0f, 0.0f })
			* glm::rotate(glm::mat4(1.0f), rotation.z, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(radius));

		int segments = 32;
		for (int i = 0; i < segments; ++i)
		{
			float angle = 2.0f * glm::pi<float>() * (float)i / segments;
			glm::vec4 startPosition = { glm::cos(angle), glm::sin(angle), 0.0f, 1.0f };
			angle = 2.0f * glm::pi<float>() * (float)((i + 1) % segments) / segments;
			glm::vec4 endPosition = { glm::cos(angle), glm::sin(angle), 0.0f, 1.0f };

			glm::vec3 p0 = transform * startPosition;
			glm::vec3 p1 = transform * endPosition;
			DrawLine(p0, p1, color);
		}

	}

	void Renderer2D::DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		const auto& meshAssetSubmeshes = mesh->GetMeshSource()->GetSubmeshes();
		auto& submeshes = mesh->GetSubmeshes();
		for (uint32_t submeshIndex : submeshes)
		{
			const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
			auto& aabb = submesh.BoundingBox;
			auto aabbTransform = transform * submesh.Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void Renderer2D::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
		};

		for (uint32_t i = 0; i < 4; ++i)
		{
			DrawLine(corners[i], corners[(i + 1) % 4], color);
		}

		for (uint32_t i = 0; i < 4; ++i)
		{
			DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);
		}

		for (uint32_t i = 0; i < 4; ++i)
		{
			DrawLine(corners[i], corners[i + 4], color);
		}
	}

	static bool NextLine(int index, const std::vector<int>& lines)
	{
		for (int line : lines)
		{
			if (line == index)
			{
				return true;
			}
		}
		return false;
	}

	void Renderer2D::DrawString(const std::string& string, const glm::vec3& position, float maxWidth, const glm::vec4& color)
	{
		// Use default font
		DrawString(string, Font::GetDefaultFont(), position, maxWidth, color);
	}

	void Renderer2D::DrawString(const std::string& string, const Ref<Font>& font, const glm::vec3& position, float maxWidth, const glm::vec4& color)
	{
		DrawString(string, font, glm::translate(glm::mat4(1.0f), position), maxWidth, color);
	}

	static std::u32string To_UTF32(const std::string& s)
	{
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
		return conv.from_bytes(s);
	}

	void Renderer2D::DrawString(const std::string& string, const Ref<Font>& font, const glm::mat4& transform, float maxWidth, const glm::vec4& color, float lineHeightOffset, float kerningOffset)
	{
		if (string.empty())
		{
			return;
		}

		float textureIndex = 0.0f;

		std::u32string utf32string = To_UTF32(string);

		Ref<Texture2D> fontAtlas = font->GetFontAtlas();
		NR_CORE_ASSERT(fontAtlas);

		for (uint32_t i = 0; i < mFontTextureSlotIndex; ++i)
		{
			if (*mFontTextureSlots[i].Raw() == *fontAtlas.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)mFontTextureSlotIndex;
			mFontTextureSlots[mFontTextureSlotIndex] = fontAtlas;
			mFontTextureSlotIndex++;
		}

		auto& fontGeometry = font->GetMSDFData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		std::vector<int> nextLines;
		{
			double x = 0.0;
			double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
			double y = -fsScale * metrics.ascenderY;
			int lastSpace = -1;
			for (int i = 0; i < utf32string.size(); ++i)
			{
				char32_t character = utf32string[i];
				if (character == '\n')
				{
					x = 0;
					y -= fsScale * metrics.lineHeight + lineHeightOffset;
					continue;
				}

				auto glyph = fontGeometry.getGlyph(character);
				if (!glyph)
				{
					glyph = fontGeometry.getGlyph('?');
				}
				if (!glyph)
				{
					continue;
				}

				if (character != ' ')
				{
					// Calc geo
					double pl, pb, pr, pt;
					glyph->getQuadPlaneBounds(pl, pb, pr, pt);
					glm::vec2 quadMin((float)pl, (float)pb);
					glm::vec2 quadMax((float)pr, (float)pt);

					quadMin *= fsScale;
					quadMax *= fsScale;
					quadMin += glm::vec2(x, y);
					quadMax += glm::vec2(x, y);

					if (quadMax.x > maxWidth && lastSpace != -1)
					{
						i = lastSpace;
						nextLines.emplace_back(lastSpace);
						lastSpace = -1;
						x = 0;
						y -= fsScale * metrics.lineHeight + lineHeightOffset;
					}
				}
				else
				{
					lastSpace = i;
				}

				double advance = glyph->getAdvance();
				fontGeometry.getAdvance(advance, character, utf32string[i + 1]);
				x += fsScale * advance + kerningOffset;
			}
		}

		{
			double x = 0.0;
			double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
			double y = 0.0;// -fsScale * metrics.ascenderY;
			for (int i = 0; i < utf32string.size(); ++i)
			{
				char32_t character = utf32string[i];
				if (character == '\n' || NextLine(i, nextLines))
				{
					x = 0;
					y -= fsScale * metrics.lineHeight + lineHeightOffset;
					continue;
				}

				auto glyph = fontGeometry.getGlyph(character);
				if (!glyph)
				{
					glyph = fontGeometry.getGlyph('?');
				}
				if (!glyph)
				{
					continue;
				}

				double l, b, r, t;
				glyph->getQuadAtlasBounds(l, b, r, t);

				double pl, pb, pr, pt;
				glyph->getQuadPlaneBounds(pl, pb, pr, pt);

				pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
				pl += x, pb += y, pr += x, pt += y;

				double texelWidth = 1. / fontAtlas->GetWidth();
				double texelHeight = 1. / fontAtlas->GetHeight();
				l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

				// ImGui::Begin("Font");
				// ImGui::Text("Size: %d, %d", mExampleFontSheet->GetWidth(), mExampleFontSheet->GetHeight());
				// UI::Image(mExampleFontSheet, ImVec2(mExampleFontSheet->GetWidth(), mExampleFontSheet->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
				// ImGui::End();

				mTextVertexBufferPtr->Position = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
				mTextVertexBufferPtr->Color = color;
				mTextVertexBufferPtr->TexCoord = { l, b };
				mTextVertexBufferPtr->TexIndex = textureIndex;
				mTextVertexBufferPtr++;

				mTextVertexBufferPtr->Position = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
				mTextVertexBufferPtr->Color = color;
				mTextVertexBufferPtr->TexCoord = { l, t };
				mTextVertexBufferPtr->TexIndex = textureIndex;
				mTextVertexBufferPtr++;

				mTextVertexBufferPtr->Position = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
				mTextVertexBufferPtr->Color = color;
				mTextVertexBufferPtr->TexCoord = { r, t };
				mTextVertexBufferPtr->TexIndex = textureIndex;
				mTextVertexBufferPtr++;

				mTextVertexBufferPtr->Position = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
				mTextVertexBufferPtr->Color = color;
				mTextVertexBufferPtr->TexCoord = { r, b };
				mTextVertexBufferPtr->TexIndex = textureIndex;
				mTextVertexBufferPtr++;

				mTextIndexCount += 6;

				double advance = glyph->getAdvance();
				fontGeometry.getAdvance(advance, character, utf32string[i + 1]);
				x += fsScale * advance + kerningOffset;

				mStats.QuadCount++;
			}
		}

	}

	void Renderer2D::SetLineWidth(float lineWidth)
	{
		mLineWidth = lineWidth;
	}

	void Renderer2D::ResetStats()
	{
		memset(&mStats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return mStats;
	}

}