using NR;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

public static class WorldDataHelper
{
    public static Vector3 ChunkPositionFromVoxelCoords(World world, Vector3 position)
    {
        return new Vector3
        {
            x = Mathf.FloorToInt(position.x / (float)world.chunkSize) * world.chunkSize,
            y = Mathf.FloorToInt(position.y / (float)world.chunkHeight) * world.chunkHeight,
            z = Mathf.FloorToInt(position.z / (float)world.chunkSize) * world.chunkSize
        };
    }

    internal static List<Vector3> GetChunkPositionsAroundPlayer(World world, Vector3 playerPosition)
    {
        float startX = playerPosition.x - (world.chunkDrawingRange) * world.chunkSize;
        float startZ = playerPosition.z - (world.chunkDrawingRange) * world.chunkSize;
        float endX = playerPosition.x + (world.chunkDrawingRange) * world.chunkSize;
        float endZ = playerPosition.z + (world.chunkDrawingRange) * world.chunkSize;
        List<Vector3> chunkPositionsToCreate = new List<Vector3>();

        for (int x = (int)startX; x <= endX; x += world.chunkSize)
        {
            for (int z = (int)startZ; z <= endZ; z += world.chunkSize)
            {
                Vector3 chunkPos = ChunkPositionFromVoxelCoords(world, new Vector3(x, 0, z));
                chunkPositionsToCreate.Add(chunkPos);
                //if (x >= playerPosition.x - world.chunkSize && 
                //    x <= playerPosition.x + world.chunkSize && 
                //    z >= playerPosition.z - world.chunkSize && 
                //    z <= playerPosition.z + world.chunkSize)
                //{
                //    for (int y = -world.chunkHeight; y >= playerPosition.y - world.chunkHeight * 2; y -= world.chunkHeight)
                //    {
                //        chunkPos = ChunkPositionFromVoxelCoords(world, new Vector3(x, y, z));
                //        chunkPositionsToCreate.Add(chunkPos);
                //    }
                //}
            }
        }
        return chunkPositionsToCreate;
    }

    internal static void RemoveChunkData(World world, Vector3 pos)
    {
        world.worldData.chunkDataDictionary.Remove(pos);
    }

    internal static void RemoveChunk(World world, Vector3 pos)
    {
        ChunkRenderer chunk = null;
        if (world.worldData.chunkDictionary.TryGetValue(pos, out chunk))
        {
            world.RemoveChunk(chunk);
            world.worldData.chunkDictionary.Remove(pos);
        }
    }

    internal static List<Vector3> GetDataPositionsAroundPlayer(World world, Vector3 playerPosition)
    {
        float startX = playerPosition.x - (world.chunkDrawingRange + 1) * world.chunkSize;
        float startZ = playerPosition.z - (world.chunkDrawingRange + 1) * world.chunkSize;
        float endX = playerPosition.x + (world.chunkDrawingRange + 1) * world.chunkSize;
        float endZ = playerPosition.z + (world.chunkDrawingRange + 1) * world.chunkSize;
        List<Vector3> chunkDataPositionsToCreate = new List<Vector3>();
        for (int x = (int)startX; x <= endX; x += world.chunkSize)
        {
            for (int z = (int)startZ; z <= endZ; z += world.chunkSize)
            {
                Vector3 chunkPos = ChunkPositionFromVoxelCoords(world, new Vector3(x, 0, z));
                chunkDataPositionsToCreate.Add(chunkPos);
                //if (x >= playerPosition.x - world.chunkSize && 
                //    x <= playerPosition.x + world.chunkSize && 
                //    z >= playerPosition.z - world.chunkSize && 
                //    z <= playerPosition.z + world.chunkSize)
                //{
                //    for (int y = -world.chunkHeight; y >= playerPosition.y - world.chunkHeight * 2; y -= world.chunkHeight)
                //    {
                //        chunkPos = ChunkPositionFromVoxelCoords(world, new Vector3(x, y, z));
                //        chunkDataPositionsToCreate.Add(chunkPos);
                //    }
                //}
            }
        }
        return chunkDataPositionsToCreate;
    }

    internal static ChunkRenderer GetChunk(World worldReference, Vector3 worldPosition)
    {
        if (worldReference.worldData.chunkDictionary.ContainsKey(worldPosition))
        {
            return worldReference.worldData.chunkDictionary[worldPosition];
        }

        return null;
    }

    internal static void SetVoxel(World worldReference, Vector3 pos, VoxelType VoxelType)
    {
        Chunk chunkData = GetChunkData(worldReference, pos);
        if (chunkData != null)
        {
            Vector3 localPosition = ChunkManager.GetVoxelInChunkCoordinates(chunkData, pos);
            ChunkManager.SetVoxel(chunkData, localPosition, VoxelType);
        }
    }

    public static Chunk GetChunkData(World worldReference, Vector3 pos)
    {
        Vector3 chunkPosition = ChunkPositionFromVoxelCoords(worldReference, pos);
        Chunk containerChunk = null;
        worldReference.worldData.chunkDataDictionary.TryGetValue(chunkPosition, out containerChunk);
        return containerChunk;
    }

    internal static List<Vector3> GetUnnededData(World.WorldData worldData, List<Vector3> allChunkDataPositionsNeeded)
    {
        return worldData.chunkDataDictionary.Keys
            .Where(pos => allChunkDataPositionsNeeded.Contains(pos) == false && worldData.chunkDataDictionary[pos].IsModied == false)
            .ToList();
    }

    internal static List<Vector3> GetUnnededChunks(World.WorldData worldData, List<Vector3> allChunkPositionsNeeded)
    {
        List<Vector3> positionToRemove = new List<Vector3>();
        foreach (var pos in worldData.chunkDictionary.Keys
            .Where(pos => allChunkPositionsNeeded.Contains(pos) == false))
        {
            Log.Info($"Delete: {pos}");
            if (worldData.chunkDictionary.ContainsKey(pos))
            {
                positionToRemove.Add(pos);
            }
        }
        return positionToRemove;
    }

    internal static List<Vector3> SelectPositonsToCreate(World.WorldData worldData, List<Vector3> allChunkPositionsNeeded, Vector3 playerPosition)
    {
        return allChunkPositionsNeeded
            .Where(pos => worldData.chunkDictionary.ContainsKey(pos) == false)
            .OrderBy(pos => Vector3.Distance(playerPosition, pos))
            .ToList();
    }

    internal static List<Vector3> SelectDataPositonsToCreate(World.WorldData worldData, List<Vector3> allChunkDataPositionsNeeded, Vector3 playerPosition)
    {
        return allChunkDataPositionsNeeded
            .Where(pos => worldData.chunkDataDictionary.ContainsKey(pos) == false)
            .OrderBy(pos => Vector3.Distance(playerPosition, pos))
            .ToList();
    }
}