using NR;
using System;

public static class ChunkManager
{
    public static void LoopThroughTheVoxels(Chunk chunkData, Action<float, float, float> actionToPerform)
    {
        for (int index = 0; index < chunkData.Voxels.Length; index++)
        {
            var position = GetPostitionFromIndex(chunkData, index);
            actionToPerform(position.x, position.y, position.z);
        }
    }

    private static Vector3 GetPostitionFromIndex(Chunk chunkData, int index)
    {
        int x = index % chunkData.Size;
        int y = (index / chunkData.Size) % chunkData.Height;
        int z = index / (chunkData.Size * chunkData.Height);
        return new Vector3(x, y, z);
    }

    //in chunk coordinate system
    private static bool InRange(Chunk chunkData, float axisCoordinate)
    {
        if (axisCoordinate < 0 || axisCoordinate >= chunkData.Size)
        {
            return false;
        }

        return true;
    }

    //in chunk coordinate system
    private static bool InRangeHeight(Chunk chunkData, float ycoordinate)
    {
        if (ycoordinate < 0 || ycoordinate >= chunkData.Height)
        {
            return false;
        }

        return true;
    }

    public static VoxelType GetBlockFromChunkCoordinates(Chunk chunkData, Vector3 chunkCoordinates)
    {
        return GetBlockFromChunkCoordinates(chunkData, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
    }

    public static VoxelType GetBlockFromChunkCoordinates(Chunk chunkData, float x, float y, float z)
    {
        if (InRange(chunkData, x) && InRangeHeight(chunkData, y) && InRange(chunkData, z))
        {
            float index = GetIndexFromPosition(chunkData, x, y, z);
            return chunkData.Voxels[(int)index];
        }

        return chunkData.WorldRef.GetBlockFromChunkCoordinates(chunkData, chunkData.WorldPosition.x + x, chunkData.WorldPosition.y + y, chunkData.WorldPosition.z + z);
    }

    public static void SetBlock(Chunk chunkData, Vector3 localPosition, VoxelType block)
    {
        if (InRange(chunkData, localPosition.x) && InRangeHeight(chunkData, localPosition.y) && InRange(chunkData, localPosition.z))
        {
            float index = GetIndexFromPosition(chunkData, localPosition.x, localPosition.y, localPosition.z);
            chunkData.Voxels[(int)index] = block;
        }
        else
        {
            Log.Info("Need to ask World for appropiate chunk");
            throw new Exception("Need to ask World for appropiate chunk");
        }
    }

    private static float GetIndexFromPosition(Chunk Chunk, float x, float y, float z)
    {
        return x + (Chunk.Size * y) + (Chunk.Size * Chunk.Height * z);
    }

    public static Vector3 GetBlockInChunkCoordinates(Chunk chunkData, Vector3 pos)
    {
        return new Vector3
        {
            x = pos.x - chunkData.WorldPosition.x,
            y = pos.y - chunkData.WorldPosition.y,
            z = pos.z - chunkData.WorldPosition.z
        };
    }

    public static MeshData GetChunkMeshData(Chunk chunkData)
    {
        MeshData meshData = new MeshData(true);

        LoopThroughTheVoxels(chunkData, (x, y, z) => meshData = BlockHelper.GetMeshData(chunkData, x, y, z, meshData, chunkData.Voxels[(int)GetIndexFromPosition(chunkData, x, y, z)]));

        return meshData;
    }

    internal static Vector3 ChunkPositionFromBlockCoords(World world, float x, float y, float z)
    {
        Vector3 pos = new Vector3
        {
            x = (float)Mathf.FloorToInt(x / (float)world.chunkSize) * world.chunkSize,
            y = (float)Mathf.FloorToInt(y / (float)world.chunkHeight) * world.chunkHeight,
            z = (float)Mathf.FloorToInt(z / (float)world.chunkSize) * world.chunkSize
        };
        return pos;
    }
}
