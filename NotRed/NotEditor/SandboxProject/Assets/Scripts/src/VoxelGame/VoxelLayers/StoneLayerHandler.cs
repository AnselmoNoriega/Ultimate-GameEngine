using NR;

public class StoneLayerHandler : VoxelLayerHandler
{
    public float stoneThreshold = 0.56f;

    private NoiseSettings stoneNoiseSettings;

    public StoneLayerHandler()
    {
        stoneNoiseSettings = new NoiseSettings();
        stoneNoiseSettings.Zoom = 0.16f;
        stoneNoiseSettings.Octaves = 2;
        stoneNoiseSettings.Offset = new Vector2(10.0f, 50.0f);
        stoneNoiseSettings.WorldOffset = new Vector2();
        stoneNoiseSettings.Persistance = 2.3f;
        stoneNoiseSettings.RedistributionModifier = 0.9f;
        stoneNoiseSettings.Exponent = 1.0f;
    }

    protected override bool TryHandling(Chunk chunkData, float x, float y, float z, float surfaceHeightNoise, Vector2 mapSeedOffset)
    {
        if (chunkData.WorldPosition.y > surfaceHeightNoise)
        {
            return false;
        }

        stoneNoiseSettings.WorldOffset = mapSeedOffset;
        float stoneNoise = Noise.OctavePerlin(chunkData.WorldPosition.x + x, chunkData.WorldPosition.z + z, stoneNoiseSettings);
        float endPosition = surfaceHeightNoise;
        
        if (chunkData.WorldPosition.y < 0)
        {
            endPosition = chunkData.WorldPosition.y + chunkData.Height;
        }
        if (stoneNoise > stoneThreshold)
        {
            for (int i = (int)chunkData.WorldPosition.y; i <= endPosition; i++)
            {
                Vector3 pos = new Vector3(x, i, z);
                ChunkManager.SetVoxel(chunkData, pos, VoxelType.Stone);
            }
            return true;
        }
        return false;
    }
} 