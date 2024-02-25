#pragma once

namespace NR
{
    enum class RendererAPI
    {
        None, OpenGL
    };

    class Renderer
    {
    public:
        inline static RendererAPI GetAPI() { return sRendererAPI; }
    private:
        static RendererAPI sRendererAPI;
    };
}