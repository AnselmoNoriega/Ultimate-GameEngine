#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/IndexBuffer.h"

namespace NR
{
	class Mesh
	{
	public:
		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec3 Tangent;
			glm::vec3 Binormal;
			glm::vec2 Texcoord;
		};

		static_assert(sizeof(Vertex) == 14 * sizeof(float));
		static const int NumAttributes = 5;

		struct Index
		{
			uint32_t V1, V2, V3;
		};
		static_assert(sizeof(Index) == 3 * sizeof(uint32_t));

		Mesh(const std::string& filename);
		~Mesh();

		void Render();

		inline const std::string& GetFilePath() const { return mFilePath; }
	private:
		std::vector<Vertex> mVertices;
		std::vector<Index> mIndices;

		std::unique_ptr<VertexBuffer> mVertexBuffer;
		std::unique_ptr<IndexBuffer> mIndexBuffer;

		std::string mFilePath;
	};
}