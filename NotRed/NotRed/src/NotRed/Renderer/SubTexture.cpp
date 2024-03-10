#include "nrpch.h"
#include "SubTexture.h"

namespace NR
{
    SubTexture::SubTexture(const Ref<Texture2D>& texture, const glm::vec2& min, const glm::vec2& max)
        : mTexture(texture)
    {
        mTexCoords[0] = { min.x, min.y };
        mTexCoords[1] = { max.x, min.y };
        mTexCoords[2] = { max.x, max.y };
        mTexCoords[3] = { min.x, max.y };
    }

    Ref<SubTexture> SubTexture::CreateFromCoords(
        const Ref<Texture2D>& texture,
        const glm::vec2& coords,
        const glm::vec2& cellSize,
        const glm::vec2& spriteSize)
    {
        glm::vec2 min = { 
            (coords.x * cellSize.x) / texture->GetWidth(),
            (coords.y * cellSize.y) / texture->GetHeight() 
        };
        glm::vec2 max = {
            ((coords.x + spriteSize.x) * cellSize.x) / texture->GetWidth(), 
            ((coords.y + spriteSize.y) * cellSize.y) / texture->GetHeight()
        };

        return CreateRef<SubTexture>(texture, min, max);
    }
}