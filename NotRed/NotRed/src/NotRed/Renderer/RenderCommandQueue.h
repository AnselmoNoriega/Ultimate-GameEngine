#pragma once

#include "nrpch.h"

namespace NR
{
	class RenderCommandQueue
	{
	public:
		using RenderCommand = std::function<uint32_t(void*)>;
		typedef uint32_t(*RenderCommandFn)(void*);

		RenderCommandQueue();
		~RenderCommandQueue();

		void Submit(const RenderCommand& command);
		void SubmitCommand(RenderCommandFn fn, void* params, uint32_t size);
		void Execute();

	private:
		unsigned char* mCommandBuffer;
		unsigned char* mCommandBufferPtr;
		uint32_t mRenderCommandCount = 0;
	};
}