using NR;

public class UndergroundLayerHandler : VoxelLayerHandler
{
    public VoxelType undergroundBlockType = VoxelType.Dirt;

    protected override bool TryHandling(Chunk chunkData, float x, float y, float z, float surfaceHeightNoise, Vector2 mapSeedOffset)
    {
        if (y < surfaceHeightNoise)
        {
            Vector3 pos = new Vector3(x, y, z);
            ChunkManager.SetVoxel(chunkData, pos, undergroundBlockType);
            return true;
        }

        return false;
    }
}