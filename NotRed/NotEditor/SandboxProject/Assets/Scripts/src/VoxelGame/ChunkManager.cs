using NR;
using System;
using System.Collections.Generic;

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

    public static VoxelType GetVoxelFromChunkCoordinates(Chunk chunkData, Vector3 chunkCoordinates)
    {
        return GetVoxelFromChunkCoordinates(chunkData, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
    }

    public static VoxelType GetVoxelFromChunkCoordinates(Chunk chunkData, float x, float y, float z)
    {
        if (InRange(chunkData, x) && InRangeHeight(chunkData, y) && InRange(chunkData, z))
        {
            float index = GetIndexFromPosition(chunkData, x, y, z);
            return chunkData.Voxels[(int)index];
        }

        return chunkData.WorldRef.GetVoxelFromChunkCoordinates(chunkData, chunkData.WorldPosition.x + x, chunkData.WorldPosition.y + y, chunkData.WorldPosition.z + z);
    }

    public static void SetVoxel(Chunk chunkData, Vector3 localPosition, VoxelType block)
    {
        if (InRange(chunkData, localPosition.x) && InRangeHeight(chunkData, localPosition.y) && InRange(chunkData, localPosition.z))
        {
            float index = GetIndexFromPosition(chunkData, localPosition.x, localPosition.y, localPosition.z);
            chunkData.Voxels[(int)index] = block;
        }
        else
        {
            WorldDataHelper.SetVoxel(chunkData.WorldRef, localPosition, block);
        }
    }

    private static float GetIndexFromPosition(Chunk Chunk, float x, float y, float z)
    {
        return x + (Chunk.Size * y) + (Chunk.Size * Chunk.Height * z);
    }

    public static Vector3 GetVoxelInChunkCoordinates(Chunk chunkData, Vector3 pos)
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

        LoopThroughTheVoxels(chunkData, (x, y, z) => meshData = VoxelHelper.GetMeshData(chunkData, x, y, z, meshData, chunkData.Voxels[(int)GetIndexFromPosition(chunkData, x, y, z)]));

        return meshData;
    }

    internal static Vector3 ChunkPositionFromVoxelCoords(World world, float x, float y, float z)
    {
        Vector3 pos = new Vector3
        {
            x = (float)Mathf.FloorToInt(x / (float)world.chunkSize) * world.chunkSize,
            y = (float)Mathf.FloorToInt(y / (float)world.chunkHeight) * world.chunkHeight,
            z = (float)Mathf.FloorToInt(z / (float)world.chunkSize) * world.chunkSize
        };
        return pos;
    }

    internal static List<Chunk> GetEdgeNeighbourChunk(Chunk chunkData, Vector3 worldPosition)
    {
        Vector3 chunkPosition = GetVoxelInChunkCoordinates(chunkData, worldPosition);
        List<Chunk> neighboursToUpdate = new List<Chunk>();
        if (chunkPosition.x == 0)
        {
            neighboursToUpdate.Add(WorldDataHelper.GetChunkData(chunkData.WorldRef, worldPosition - Vector3.Right));
        }
        if (chunkPosition.x == chunkData.Size - 1)
        {
            neighboursToUpdate.Add(WorldDataHelper.GetChunkData(chunkData.WorldRef, worldPosition + Vector3.Right));
        }
        if (chunkPosition.y == 0)
        {
            neighboursToUpdate.Add(WorldDataHelper.GetChunkData(chunkData.WorldRef, worldPosition - Vector3.Up));
        }
        if (chunkPosition.y == chunkData.Height - 1)
        {
            neighboursToUpdate.Add(WorldDataHelper.GetChunkData(chunkData.WorldRef, worldPosition + Vector3.Up));
        }
        if (chunkPosition.z == 0)
        {
            neighboursToUpdate.Add(WorldDataHelper.GetChunkData(chunkData.WorldRef, worldPosition - Vector3.Forward));
        }
        if (chunkPosition.z == chunkData.Size - 1)
        {
            neighboursToUpdate.Add(WorldDataHelper.GetChunkData(chunkData.WorldRef, worldPosition + Vector3.Forward));
        }
        return neighboursToUpdate;
    }
    internal static bool IsOnEdge(Chunk chunkData, Vector3 worldPosition)
    {
        Vector3 chunkPosition = GetVoxelInChunkCoordinates(chunkData, worldPosition);

        if (chunkPosition.x == 0 || chunkPosition.x == chunkData.Size - 1 ||
            chunkPosition.y == 0 || chunkPosition.y == chunkData.Height - 1 ||
            chunkPosition.z == 0 || chunkPosition.z == chunkData.Size - 1)
        {
            return true;
        }

        return false;
    }
}
