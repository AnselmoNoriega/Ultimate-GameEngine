using NR;

public abstract class VoxelLayerHandler
{
    protected VoxelLayerHandler Next;

    public bool Handle(Chunk chunkData, float x, float y, float z, int surfaceHeightNoise, Vector2 mapSeedOffset)
    {
        if (TryHandling(chunkData, x, y, z, surfaceHeightNoise, mapSeedOffset))
        {
            return true;
        }

        if (Next != null)
        {
            return Next.Handle(chunkData, x, y, z, surfaceHeightNoise, mapSeedOffset);
        }

        return false;
    }

    protected abstract bool TryHandling(Chunk chunkData, float x, float y, float z, float surfaceHeightNoise, Vector2 mapSeedOffset);
}