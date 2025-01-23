using NR;
using System;
using System.Collections;
using System.Collections.Generic;

public class BiomeGenerator
{
    public int waterThreshold = 50;
    public float noiseScale = 0.03f;

    public BiomeGenerator(int waterThreshold, float noiseScale)
    {
        this.waterThreshold = waterThreshold;
        this.noiseScale = noiseScale;
    }

    public Chunk ProcessChunkColumn(Chunk data, int x, int z, Vector2 mapSeedOffset)
    {
        float noiseValue = Noise.PerlinNoise((mapSeedOffset.x + data.WorldPosition.x + x) * noiseScale, (mapSeedOffset.y + data.WorldPosition.z + z) * noiseScale);
        int groundPosition = Mathf.RoundToInt(noiseValue * data.Height);

        for (int y = 0; y < data.Height; ++y)
        {
            VoxelType voxelType = VoxelType.Dirt;
            if (y > groundPosition)
            {
                if (y < waterThreshold)
                {
                    voxelType = VoxelType.Water;
                }
                else
                {
                    voxelType = VoxelType.Air;
                }
            }
            else if (y == groundPosition && y < waterThreshold)
            {
                voxelType = VoxelType.Sand;
            }
            else if (y == groundPosition)
            {
                voxelType = VoxelType.Grass;
            }

            ChunkManager.SetBlock(data, new Vector3(x, y, z), voxelType);
        }
        return data;
    }
}