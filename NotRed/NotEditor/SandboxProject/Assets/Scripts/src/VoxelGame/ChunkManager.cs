using System;
using NR;

public static class ChunkManager
{
    public static void LoopThroughTheVoxels(Chunk Chunk, Action<float, float, float> actionToPerform)
    {
        for (int index = 0; index < Chunk.Voxels.Length; index++)
        {
            var position = GetPostitionFromIndex(Chunk, index);
            actionToPerform(position.x, position.y, position.z);
        }
    }

    private static Vector3 GetPostitionFromIndex(Chunk Chunk, int index)
    {
        int x = index % Chunk.Size;
        int y = (index / Chunk.Size) % Chunk.Height;
        int z = index / (Chunk.Size * Chunk.Height);
        return new Vector3(x, y, z);
    }

    //in chunk coordinate system
    private static bool InRange(Chunk Chunk, float axisCoordinate)
    {
        if (axisCoordinate < 0 || axisCoordinate >= Chunk.Size)
        {
            return false;
        }

        return true;
    }

    //in chunk coordinate system
    private static bool InRangeHeight(Chunk Chunk, float ycoordinate)
    {
        if (ycoordinate < 0 || ycoordinate >= Chunk.Height)
        {
            return false;
        }

        return true;
    }

    public static VoxelType GetBlockFromChunkCoordinates(Chunk Chunk, Vector3 chunkCoordinates)
    {
        return GetBlockFromChunkCoordinates(Chunk, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
    }

    public static VoxelType GetBlockFromChunkCoordinates(Chunk Chunk, float x, float y, float z)
    {
        if (InRange(Chunk, x) && InRangeHeight(Chunk, y) && InRange(Chunk, z))
        {
            float index = GetIndexFromPosition(Chunk, x, y, z);
            return Chunk.Voxels[(int)index];
        }
        throw new Exception("Need to ask World for appropiate chunk");
    }

    public static void SetBlock(Chunk Chunk, Vector3 localPosition, VoxelType block)
    {
        if (InRange(Chunk, localPosition.x) && InRangeHeight(Chunk, localPosition.y) && InRange(Chunk, localPosition.z))
        {
            float index = GetIndexFromPosition(Chunk, localPosition.x, localPosition.y, localPosition.z);
            Chunk.Voxels[(int)index] = block;
        }
        else
        {
            throw new Exception("Need to ask World for appropiate chunk");
        }
    }

    private static float GetIndexFromPosition(Chunk Chunk, float x, float y, float z)
    {
        return x + Chunk.Size * y + Chunk.Size * Chunk.Height * z;
    }

    public static Vector3 GetBlockInChunkCoordinates(Chunk Chunk, Vector3 pos)
    {
        return new Vector3
        {
            x = pos.x - Chunk.WorldPosition.x,
            y = pos.y - Chunk.WorldPosition.y,
            z = pos.z - Chunk.WorldPosition.z
        };
    }

    public static MeshData GetChunkMeshData(Chunk Chunk)
    {
        MeshData meshData = new MeshData(true);
        return meshData;
    }
}
