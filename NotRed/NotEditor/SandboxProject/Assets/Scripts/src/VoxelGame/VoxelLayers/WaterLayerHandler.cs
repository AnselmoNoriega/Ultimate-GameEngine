using System.Collections;
using System.Collections.Generic;
using NR;

public class WaterLayerHandler : VoxelLayerHandler
{
    public int waterLevel = 50;

    public WaterLayerHandler(int waterLevel)
    {
        this.waterLevel = waterLevel;
    }

    protected override bool TryHandling(Chunk chunkData, float x, float y, float z, float surfaceHeightNoise, Vector2 mapSeedOffset)
    {
        Next = new AirLayerHandler();

        if (y > surfaceHeightNoise && y <= waterLevel)
        {
            Vector3 pos = new Vector3(x, y, z);
            ChunkManager.SetVoxel(chunkData, pos, VoxelType.Water);

            if (y == surfaceHeightNoise + 1)
            {
                pos.y = surfaceHeightNoise;
                ChunkManager.SetVoxel(chunkData, pos, VoxelType.Sand);
            }

            return true;
        }

        return false;
    }
}