using NR;

public class AirLayerHandler : VoxelLayerHandler
{
    protected override bool TryHandling(Chunk chunkData, float x, float y, float z, float surfaceHeightNoise, Vector2 mapSeedOffset)
    {
        Next = new SurfaceLayerHandler();

        if (y > surfaceHeightNoise)
        {
            Vector3 pos = new Vector3(x, y, z);
            ChunkManager.SetVoxel(chunkData, pos, VoxelType.Air);
            return true;
        }

        return false;
    }
}