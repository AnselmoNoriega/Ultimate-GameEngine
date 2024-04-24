#pragma once

#include <stdint.h>
#include <cstring>

namespace NR 
{
	template<typename T>
	struct Buffer
	{
		T* Data = nullptr;
		uint64_t Size = 0;

		Buffer() = default;
		Buffer(const Buffer&) = default;

		Buffer(uint64_t size)
		{
			Allocate(size);
		}

		void Allocate(uint64_t size)
		{
			Release();

			Data = new T[size];
			Size = size;
		}

		void Release()
		{
			delete[] Data;
			Data = nullptr;
			Size = 0;
		}

		operator bool() const { return (bool)Data; }

		Buffer<T> Copy()
		{
			Buffer<T> result(Size);
			memcpy((void*)result.Data, (const void*)Data, Size * sizeof(T));
			return result;
		}
	};

	template<typename T>
	struct ScopedBuffer
	{
		ScopedBuffer(const Buffer<T> buffer)
			: mBuffer(buffer)
		{
		}

		ScopedBuffer(uint64_t size)
			: mBuffer(size)
		{
		}

		~ScopedBuffer()
		{
			mBuffer.Release();
		}

		T* Data() const { return mBuffer.Data; }
		uint64_t Size() const { return mBuffer.Size; }

		explicit operator bool() const { return mBuffer; }

	private:
		Buffer<T> mBuffer;
	};
}