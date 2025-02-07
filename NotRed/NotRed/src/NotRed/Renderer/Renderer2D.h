#pragma once

#include <glm/glm.hpp>

#include "NotRed/Math/AABB.h"

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/RenderPass.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"
#include "NotRed/Renderer/UniformBufferSet.h"

#include "NotRed/Renderer/UI/Font.h"

namespace NR
{
	class Renderer2D : public RefCounted
	{
	public:
		Renderer2D();
		virtual ~Renderer2D();

		void Init();
		void Shutdown();

		void BeginScene(const glm::mat4& viewProj, const glm::mat4& view, bool depthTest = true);
		void EndScene();

		Ref<RenderPass> GetTargetRenderPass();
		void SetTargetRenderPass(Ref<RenderPass> renderPass);

		// Primitives
		void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
		void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		void DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		void DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		void DrawRotatedRect(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		void DrawRotatedRect(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);

		// Thickness is between 0 and 1
		void DrawCircle(const glm::vec3& p0, const glm::vec3& rotation, float radius, const glm::vec4& color);
		void FillCircle(const glm::vec2& p0, float radius, const glm::vec4& color, float thickness = 0.05f);
		void FillCircle(const glm::vec3& p0, float radius, const glm::vec4& color, float thickness = 0.05f);

		void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color = glm::vec4(1.0f));

		void DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
		void DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

		void DrawString(const std::string& string, const glm::vec3& position, float maxWidth, const glm::vec4& color = glm::vec4(1.0f));
		void DrawString(const std::string& string, const Ref<Font>& font, const glm::vec3& position, float maxWidth, const glm::vec4& color = glm::vec4(1.0f));
		void DrawString(const std::string& string, const Ref<Font>& font, const glm::mat4& transform, float maxWidth, const glm::vec4& color = glm::vec4(1.0f), float lineHeightOffset = 0.0f, float kerningOffset = 0.0f);

		void SetLineWidth(float lineWidth);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t LineCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4 + LineCount * 2; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6 + LineCount * 2; }
		};

		void ResetStats();
		Statistics GetStats();

	private:
		void Flush();

		void FlushAndReset();
		void FlushAndResetLines();

	private:
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec4 Color;
			glm::vec2 TexCoord;
			float TexIndex;
			float TilingFactor;
		};

		struct TextVertex
		{
			glm::vec3 Position;
			glm::vec4 Color;
			glm::vec2 TexCoord;
			float TexIndex;
		};

		struct LineVertex
		{
			glm::vec3 Position;
			glm::vec4 Color;
		};

		struct CircleVertex
		{
			glm::vec3 WorldPosition;
			float Thickness;
			glm::vec2 LocalPosition;
			glm::vec4 Color;
		};

		static const uint32_t MaxQuads = 200000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxLineIndices = MaxLines * 6;

		Ref<RenderCommandBuffer> mRenderCommandBuffer;

		Ref<Texture2D> mWhiteTexture;

		Ref<Pipeline> mQuadPipeline;
		Ref<VertexBuffer> mQuadVertexBuffer;
		Ref<IndexBuffer> mQuadIndexBuffer;
		Ref<Material> mQuadMaterial;

		uint32_t mQuadIndexCount = 0;
		QuadVertex* mQuadVertexBufferBase = nullptr;
		QuadVertex* mQuadVertexBufferPtr = nullptr;

		Ref<Pipeline> mCirclePipeline;
		Ref<VertexBuffer> mCircleVertexBuffer;
		uint32_t mCircleIndexCount = 0;
		CircleVertex* mCircleVertexBufferBase = nullptr;
		CircleVertex* mCircleVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> mTextureSlots;
		uint32_t mTextureSlotIndex = 1; // 0 = white texture

		glm::vec4 mQuadVertexPositions[4];

		// Lines
		Ref<Pipeline> mLinePipeline;
		Ref<Pipeline> mLineOnTopPipeline;
		Ref<VertexBuffer> mLineVertexBuffer;
		Ref<IndexBuffer> mLineIndexBuffer;
		Ref<Material> mLineMaterial;

		uint32_t mLineIndexCount = 0;
		LineVertex* mLineVertexBufferBase = nullptr;
		LineVertex* mLineVertexBufferPtr = nullptr;

		// Text
		Ref<Pipeline> mTextPipeline;
		Ref<VertexBuffer> mTextVertexBuffer;
		Ref<IndexBuffer> mTextIndexBuffer;
		Ref<Material> mTextMaterial;
		std::array<Ref<Texture2D>, MaxTextureSlots> mFontTextureSlots;
		uint32_t mFontTextureSlotIndex = 0;

		uint32_t mTextIndexCount = 0;
		TextVertex* mTextVertexBufferBase = nullptr;
		TextVertex* mTextVertexBufferPtr = nullptr;

		glm::mat4 mCameraViewProj;
		glm::mat4 mCameraView;
		bool mDepthTest = true;

		float mLineWidth = 1.0f;

		Statistics mStats;

		Ref<UniformBufferSet> mUniformBufferSet;

		struct UBCamera
		{
			glm::mat4 ViewProjection;
		};
	};

}