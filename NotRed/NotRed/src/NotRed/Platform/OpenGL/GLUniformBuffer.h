#pragma once

#include "NotRed/Renderer/UniformBuffer.h"

namespace NR
{
	class GLUniformBuffer : public UniformBuffer
	{
	public:
		GLUniformBuffer(uint32_t size, uint32_t binding);
		~GLUniformBuffer() override;

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		uint32_t GetBinding() const override { return mBinding; }

	private:
		uint32_t mID = 0;
		uint32_t mSize = 0;
		uint32_t mBinding = 0;
		uint8_t* mLocalStorage = nullptr;
	};
}