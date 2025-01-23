using NR;
using System;
using System.Collections;
using System.Collections.Generic;

public class TerrainGenerator
{
    public BiomeGenerator biomeGenerator;

    public TerrainGenerator(BiomeGenerator biomeGenerator)
    {
        this.biomeGenerator = biomeGenerator;
    }

    public Chunk GenerateChunk(Chunk data, Vector2 mapSeedOffset)
    {
        for (int x = 0; x < data.Size; x++)
        {
            for (int z = 0; z < data.Size; z++)
            {
                data = biomeGenerator.ProcessChunkColumn(data, x, z, mapSeedOffset);
            }
        }
        return data;
    }
}