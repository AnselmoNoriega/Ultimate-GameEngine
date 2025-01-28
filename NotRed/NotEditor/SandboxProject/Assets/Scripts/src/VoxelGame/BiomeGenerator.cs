using NR;
using System;
using System.Collections;
using System.Collections.Generic;

public class BiomeGenerator
{
    public int waterThreshold;
    public float noiseScale;

    public DomainWarping domainWarping;
    public bool useDomainWarping = true;

    public NoiseSettings biomeNoiseSettings;

    public VoxelLayerHandler startLayerHandler;

    public List<VoxelLayerHandler> additionalLayerHandlers = new List<VoxelLayerHandler>();

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

        startLayerHandler = new WaterLayerHandler(waterThreshold);
        domainWarping = new DomainWarping();

        additionalLayerHandlers.Add(new StoneLayerHandler());

        this.waterThreshold = waterThreshold;
        this.noiseScale = noiseScale;
    }

    public Chunk ProcessChunkColumn(Chunk data, int x, int z, Vector2 mapSeedOffset)
    {
        biomeNoiseSettings.WorldOffset = mapSeedOffset;
        int groundPosition = GetSurfaceHeightNoise(data.WorldPosition.x + x, data.WorldPosition.z + z, data.Height);

        for (int y = 0; y < data.Height; ++y)
        {
            startLayerHandler.Handle(data, x, y, z, groundPosition, mapSeedOffset);
        }

        foreach (var layer in additionalLayerHandlers)
        {
            layer.Handle(data, x, data.WorldPosition.y, z, groundPosition, mapSeedOffset);
        }
        return data;
    }

    private int GetSurfaceHeightNoise(float x, float z, int chunkHeight)
    {
        float terrainHeight;
        if (useDomainWarping == false)
        {
            terrainHeight = Noise.OctavePerlin(x, z, biomeNoiseSettings);
        }
        else
        {
            terrainHeight = domainWarping.GenerateDomainNoise(x, z, biomeNoiseSettings);
        }

        terrainHeight = Noise.Redistribution(terrainHeight, biomeNoiseSettings);

        int surfaceHeight = Noise.RemapValueToInt(terrainHeight, 0, chunkHeight);

        return surfaceHeight;
    }
}