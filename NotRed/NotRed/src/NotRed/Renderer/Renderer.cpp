#include "nrpch.h"
#include "Renderer.h"

#include "Platform/OpenGL/GLShader.h"
#include "Texture.h"
#include "Mesh.h"

namespace NR
{
	struct VertexData
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec2 TexCoord = { 0.0f, 0.0f };
		glm::vec4 Color = { 0.0f, 0.0f, 0.0f, 0.0f };
		float TexIndex = 0.0f;

		int EntityID = -1;
	};

	struct Renderer2DStorage
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> ObjShader;
		Ref<Texture2D> EmptyTexture;

		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		Ref<VertexArray> TextVertexArray;
		Ref<VertexBuffer> TextVertexBuffer;
		Ref<Shader> TextShader;

		uint32_t IndexCount = 0;
		VertexData* VertexBufferBase = nullptr;
		VertexData* VertexBufferPtr = nullptr;

		float LineWidth = 2.0f;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		glm::vec4 VertexPositions[4] = { { -0.5, -0.5, 0.0, 1.0f },
										 {  0.5, -0.5, 0.0, 1.0f },
										 {  0.5,  0.5, 0.0, 1.0f },
										 { -0.5,  0.5, 0.0, 1.0f } };

		glm::vec2 TextureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f },
									   { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		Renderer::Statistics Stats;


		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
		//Ref<UniformBuffer> CameraUniformBuffer;
	};

	void Renderer::SetMeshLayout(Ref<VertexArray> vertexArray, Ref<VertexBuffer> vertexBuffer, uint32_t verticesCount)
	{
		vertexArray = VertexArray::Create();

		vertexBuffer = VertexBuffer::Create(verticesCount * sizeof(VertexData));
		vertexBuffer->SetLayout(
			{
				{ShaderDataType::Float3, "aPosition"},
				{ShaderDataType::Float2, "aTexCoord"},
				{ShaderDataType::Float4, "aColor"   },
				{ShaderDataType::Float,  "aTexIndex"},
				{ShaderDataType::Int,    "aEntityID"}
			});
		vertexArray->AddVertexBuffer(vertexBuffer);
	}

	void Renderer::PackageVertices(Ref<Mesh> model, glm::vec3 position, glm::vec2 texCoord)
	{
		NR_PROFILE_FUNCTION();

		VertexData vertex;

		//if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
		//{
		//    NextBatch();
		//}

		vertex.Position = position;
		vertex.TexCoord = texCoord;
		vertex.TexIndex = 0.0f;
		//sData.VertexBufferPtr->EntityID = entityID;
		const std::byte* rawData = reinterpret_cast<const std::byte*>(&vertex);

		model->AddVertex(rawData, sizeof(vertex));
	}
}