#pragma once

#include "NotRed/Core/Ref.h"

#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/RenderPass.h"
#include "NotRed/Renderer/UniformBuffer.h"

namespace NR
{
	enum class PrimitiveTopology
	{
		None,
		Points,
		Lines,
		Triangles,
		LineStrip,
		TriangleStrip,
		TriangleFan
	};

	struct PipelineSpecification
	{
		Ref<NR::Shader> Shader;
		VertexBufferLayout Layout;
		Ref<RenderPass> RenderPass;

		PrimitiveTopology Topology = PrimitiveTopology::Triangles;

		bool BackfaceCulling = true;
		bool DepthTest = true;
		bool DepthWrite = true;
		bool Wireframe = false;

		float LineWidth = 1.0f;

		std::string DebugName;
	};

	class Pipeline : public RefCounted
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;
		virtual void SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set = 0) = 0;

		virtual void Bind() = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& spec);
	};
}