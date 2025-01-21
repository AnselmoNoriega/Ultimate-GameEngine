using NR;
using System;
using System.Collections.Generic;

public class World : Entity
{
    public int mapSizeInChunks = 6;
    public int chunkSize = 16, chunkHeight = 100;
    public int waterThreshold = 50;
    public float noiseScale = 0.03f;
    public Prefab chunkPrefab;

    Dictionary<Vector3, Chunk> chunkDataDictionary = new Dictionary<Vector3, Chunk>();
    Dictionary<Vector3, ChunkRenderer> chunkDictionary = new Dictionary<Vector3, ChunkRenderer>();

    public void Init()
    {
        GenerateWorld();
    }

    public void GenerateWorld()
    {
        chunkDataDictionary.Clear();
        //foreach (ChunkRenderer chunk in chunkDictionary.Values)
        //{
        //    Destroy(chunk.gameObject);
        //}

        chunkDictionary.Clear();
        for (int x = 0; x < mapSizeInChunks; ++x)
        {
            for (int z = 0; z < mapSizeInChunks; ++z)
            {
                Chunk data = new Chunk(chunkSize, chunkHeight, this, new Vector3(x * chunkSize, 0, z * chunkSize));
                GenerateVoxels(data);
                chunkDataDictionary.Add(data.WorldPosition, data);
            }
        }

        foreach (Chunk data in chunkDataDictionary.Values)
        {
            MeshData meshData = ChunkManager.GetChunkMeshData(data);
            Entity chunkObject = Instantiate(chunkPrefab, data.WorldPosition);
            ChunkRenderer chunkRenderer = new ChunkRenderer();

            chunkDictionary.Add(data.WorldPosition, chunkRenderer);
            chunkRenderer.InitializeChunk(data, chunkObject.GetComponent<MeshComponent>());
            chunkRenderer.UpdateChunk(meshData);
        }
    }

    private void GenerateVoxels(Chunk data)
    {
        for (int x = 0; x < data.Size; ++x)
        {
            for (int z = 0; z < data.Size; ++z)
            {
                float noiseValue = Noise.PerlinNoise((data.WorldPosition.x + x) * noiseScale, (data.WorldPosition.z + z) * noiseScale);
                int groundPosition = 0;// Mathf.RoundToInt(noiseValue * chunkHeight);
                for (int y = 0; y < chunkHeight; ++y)
                {
                    VoxelType voxelType = VoxelType.Dirt;
                    if (y > groundPosition)
                    {
                        if (y < waterThreshold)
                        {
                            voxelType = VoxelType.Water;
                        }
                        else
                        {
                            voxelType = VoxelType.Air;
                        }
                    }
                    else if (y == groundPosition)
                    {
                        voxelType = VoxelType.Grass;
                    }
                    ChunkManager.SetBlock(data, new Vector3(x, y, z), voxelType);
                }
            }
        }
    }

    internal VoxelType GetBlockFromChunkCoordinates(Chunk chunkData, float x, float y, float z)
    {
        Vector3 pos = ChunkManager.ChunkPositionFromBlockCoords(this, x, y, z);
        Chunk containerChunk = null;
        chunkDataDictionary.TryGetValue(pos, out containerChunk);
        if (containerChunk == null)
        {
            return VoxelType.Nothing;
        }

        Vector3 blockInCHunkCoordinates = ChunkManager.GetBlockInChunkCoordinates(containerChunk, new Vector3(x, y, z));

        return ChunkManager.GetBlockFromChunkCoordinates(containerChunk, blockInCHunkCoordinates);
    }
}