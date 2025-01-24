using NR;
using System;
using System.Collections;
using System.Collections.Generic;

public class BiomeGenerator
{
    public int waterThreshold;
    public float noiseScale;

    public NoiseSettings biomeNoiseSettings;

    public BiomeGenerator(int waterThreshold, float noiseScale)
    {
        biomeNoiseSettings = new NoiseSettings();
        biomeNoiseSettings.Zoom = 0.16f;
        biomeNoiseSettings.Octaves = 6;
        biomeNoiseSettings.Offset = new Vector2();
        biomeNoiseSettings.WorldOffset = new Vector2();
        biomeNoiseSettings.Persistance = 2.3f;
        biomeNoiseSettings.RedistributionModifier = 0.9f;
        biomeNoiseSettings.Exponent = 1.0f;

        this.waterThreshold = waterThreshold;
        this.noiseScale = noiseScale;
    }

    public Chunk ProcessChunkColumn(Chunk data, int x, int z, Vector2 mapSeedOffset)
    {
        biomeNoiseSettings.WorldOffset = mapSeedOffset;
        int groundPosition = GetSurfaceHeightNoise(data.WorldPosition.x + x, data.WorldPosition.z + z, data.Height);

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

    private int GetSurfaceHeightNoise(float x, float z, int chunkHeight)
    {
        float terrainHeight = Noise.OctavePerlin(x, z, biomeNoiseSettings);
        terrainHeight = Noise.Redistribution(terrainHeight, biomeNoiseSettings);

        int surfaceHeight = Noise.RemapValueToInt(terrainHeight, 0, chunkHeight);

        return surfaceHeight;
    }
}