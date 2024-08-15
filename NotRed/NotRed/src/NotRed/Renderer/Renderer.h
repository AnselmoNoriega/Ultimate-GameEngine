#pragma once

#include "RenderCommandQueue.h"
#include "RendererAPI.h"

namespace NR
{
	class Renderer
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		static void Clear();
		static void Clear(float r, float g, float b, float a = 1.0f);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, bool depthTestActive = true);

		static void ClearMagenta();

		static void Init();

		static void* Submit(RenderCommandFn fn, uint32_t size)
		{
			return sInstance->mCommandQueue.Allocate(fn, size);
		}

		void WaitAndRender();

		inline static Renderer& Get() { return *sInstance; }

	private:
		static Renderer* sInstance;

		RenderCommandQueue mCommandQueue;
	};
}

#define NR_RENDER_PASTE2(a, b) a ## b
#define NR_RENDER_PASTE(a, b) NR_RENDER_PASTE2(a, b)
#define NR_RENDER_UNIQUE(x) NR_RENDER_PASTE(x, __LINE__)

#define NR_RENDER(code) \
    struct NR_RENDER_UNIQUE(NRRenderCommand) \
    {\
        static void Execute(void*)\
        {\
            code\
        }\
    };\
	{\
		auto mem = ::NR::Renderer::Submit(NR_RENDER_UNIQUE(NRRenderCommand)::Execute, sizeof(NR_RENDER_UNIQUE(NRRenderCommand)));\
		new (mem) NR_RENDER_UNIQUE(NRRenderCommand)();\
	}\

#define NR_RENDER_1(arg0, code) \
	do {\
    struct NR_RENDER_UNIQUE(NRRenderCommand) \
    {\
		NR_RENDER_UNIQUE(NRRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0) \
		: arg0(arg0) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg0;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
    };\
	{\
		auto mem = ::NR::Renderer::Submit(NR_RENDER_UNIQUE(NRRenderCommand)::Execute, sizeof(NR_RENDER_UNIQUE(NRRenderCommand)));\
		new (mem) NR_RENDER_UNIQUE(NRRenderCommand)(arg0);\
	} } while(0)

#define NR_RENDER_2(arg0, arg1, code) \
    struct NR_RENDER_UNIQUE(NRRenderCommand) \
    {\
		NR_RENDER_UNIQUE(NRRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1) \
		: arg0(arg0), arg1(arg1) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg1;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
    };\
	{\
		auto mem = ::NR::Renderer::Submit(NR_RENDER_UNIQUE(NRRenderCommand)::Execute, sizeof(NR_RENDER_UNIQUE(NRRenderCommand)));\
		new (mem) NR_RENDER_UNIQUE(NRRenderCommand)(arg0, arg1);\
	}\

#define NR_RENDER_3(arg0, arg1, arg2, code) \
    struct NR_RENDER_UNIQUE(NRRenderCommand) \
    {\
		NR_RENDER_UNIQUE(NRRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2) \
		: arg0(arg0), arg1(arg1), arg2(arg2) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg1;\
			auto& arg2 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg2;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2;\
    };\
	{\
		auto mem = ::NR::Renderer::Submit(NR_RENDER_UNIQUE(NRRenderCommand)::Execute, sizeof(NR_RENDER_UNIQUE(NRRenderCommand)));\
		new (mem) NR_RENDER_UNIQUE(NRRenderCommand)(arg0, arg1, arg2);\
	}\

#define NR_RENDER_4(arg0, arg1, arg2, arg3, code) \
    struct NR_RENDER_UNIQUE(NRRenderCommand) \
    {\
		NR_RENDER_UNIQUE(NRRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg3)>::type>::type arg3)\
		: arg0(arg0), arg1(arg1), arg2(arg2), arg3(arg3) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg1;\
			auto& arg2 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg2;\
			auto& arg3 = ((NR_RENDER_UNIQUE(NRRenderCommand)*)argBuffer)->arg3;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg3)>::type>::type arg3;\
    };\
	{\
		auto mem = Renderer::Submit(NR_RENDER_UNIQUE(NRRenderCommand)::Execute, sizeof(NR_RENDER_UNIQUE(NRRenderCommand)));\
		new (mem) NR_RENDER_UNIQUE(NRRenderCommand)(arg0, arg1, arg2, arg3);\
	}

#define NR_RENDER_S(code) auto self = this;\
	NR_RENDER_1(self, code)

#define NR_RENDER_S1(arg0, code) auto self = this;\
	NR_RENDER_2(self, arg0, code)

#define NR_RENDER_S2(arg0, arg1, code) auto self = this;\
	NR_RENDER_3(self, arg0, arg1, code)

#define NR_RENDER_S3(arg0, arg1, arg2, code) auto self = this;\
	NR_RENDER_4(self, arg0, arg1, arg2, code)