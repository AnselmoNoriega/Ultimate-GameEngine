#include "nrpch.h"
#include "Font.h"

#include <filesystem>

#undef INFINITE
#include "msdf-atlas-gen.h"
#include "FontGeometry.h"
#include "GlyphGeometry.h"

#include "MSDFData.h"

namespace NR
{
    template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
    static Ref<Texture2D> CreateAndCacheAtlas
    (
        const std::string& fontName,
        float fontSize,
        const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
        const msdf_atlas::FontGeometry& fontGeometry,
        uint32_t width,
        uint32_t height
    )
    {
        msdf_atlas::GeneratorAttributes attributes;
        attributes.config.overlapSupport = true;
        attributes.scanlinePass = true;

        msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
        generator.setAttributes(attributes);
        generator.setThreadCount(8);
        generator.generate(glyphs.data(), (int)glyphs.size());

        msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

        TextureSpecification specification;
        specification.Width = bitmap.width;
        specification.Height = bitmap.height;
        specification.Format = ImageFormat::RGB8;
        specification.GenerateMips = false;

        Ref<Texture2D> texture = Texture2D::Create(specification);
        texture->SetData((void*)bitmap.pixels, bitmap.width * bitmap.height * 3);
        return texture;
    }

    Font::Font(const std::filesystem::path& filepath)
        : mData(new MSDFData())
    {
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
        NR_CORE_ASSERT(ft, "Unable to Initialize Freetype!");

        std::string fileString = filepath.string();

        msdfgen::FontHandle* font = msdfgen::loadFont(ft, fileString.c_str());
        if (!font)
        {
            NR_CORE_ERROR("Failed to load font: {}", fileString);
            return;
        }

        struct CharsetRange
        {
            uint32_t Begin, End;
        };

        // From imgui_draw.cpp
        static const CharsetRange charsetRanges[] = { { 0x0020, 0x00FF } };

        msdf_atlas::Charset charset;
        for (CharsetRange range : charsetRanges)
        {
            for (uint32_t c = range.Begin; c <= range.End; ++c)
            {
                charset.add(c);
            }
        }

        double fontScale = 1.0;
        mData->FontGeometry = msdf_atlas::FontGeometry(&mData->Glyphs);
        int glyphsLoaded = mData->FontGeometry.loadCharset(font, fontScale, charset);
        NR_CORE_INFO("Loaded {} glyphs from font (out of {})", glyphsLoaded, charset.size());

        float emSize = 40.0;

        msdf_atlas::TightAtlasPacker atlasPacker;
        atlasPacker.setPixelRange(2.0);
        atlasPacker.setMiterLimit(1.0);
        atlasPacker.setPadding(0);
        atlasPacker.setScale(emSize);
        int remaining = atlasPacker.pack(mData->Glyphs.data(), (int)mData->Glyphs.size());
        NR_CORE_ASSERT(remaining == 0, "AtlasPacker could not pack all data!");

        int width, height;
        atlasPacker.getDimensions(width, height);
        emSize = (float)atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8

        uint64_t coloringSeed = 0;
        bool expensiveColoring = false;
        if (expensiveColoring)
        {
            msdf_atlas::Workload(
                [&glyphs = mData->Glyphs, &coloringSeed](int i, int threadNo) -> bool 
                {
                unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
                glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
                return true;
                }, 
                mData->Glyphs.size()
            ).finish(THREAD_COUNT);
        }
        else 
        {
            unsigned long long glyphSeed = coloringSeed;
            for (msdf_atlas::GlyphGeometry& glyph : mData->Glyphs)
            {
                glyphSeed *= LCG_MULTIPLIER;
                glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
            }
        }

        mAtlas = CreateAndCacheAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>("FirstFont", emSize, mData->Glyphs, mData->FontGeometry, width, height);

        msdfgen::destroyFont(font);
        msdfgen::deinitializeFreetype(ft);
    }

    Ref<Font> Font::GetDefault()
    {
        static Ref<Font> DefaultFont;
        if (!DefaultFont)
        {
            DefaultFont = CreateRef<Font>("Assets/Fonts/Main/Baskervville-Regular.ttf");
        }

        return DefaultFont;
    }

    Font::~Font()
    {
        delete mData;
    }
}