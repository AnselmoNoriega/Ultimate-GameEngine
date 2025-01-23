using NR;
using System.Collections.Generic;

public class World : Entity
{
    public int mapSizeInChunks = 6;
    public int chunkSize = 16, chunkHeight = 100;

    // For Biome
    public int waterThreshold = 50;
    public float noiseScale = 0.03f;

    public Prefab chunkPrefab;

    public TerrainGenerator terrainGenerator;
    public Vector2 mapSeed;

    Dictionary<Vector3, Chunk> chunkDataDictionary = new Dictionary<Vector3, Chunk>();
    Dictionary<Vector3, ChunkRenderer> chunkDictionary = new Dictionary<Vector3, ChunkRenderer>();

    public void Init()
    {
        terrainGenerator = new TerrainGenerator(new BiomeGenerator(waterThreshold, noiseScale));
        GenerateWorld();
    }

    public void GenerateWorld()
    {
        chunkDataDictionary.Clear();
        chunkDictionary.Clear();
        for (int x = 0; x < mapSizeInChunks; ++x)
        {
            for (int z = 0; z < mapSizeInChunks; ++z)
            {
                Chunk data = new Chunk(chunkSize, chunkHeight, this, new Vector3(x * chunkSize, 0, z * chunkSize));
                //GenerateVoxels(data);
                Chunk newData = terrainGenerator.GenerateChunk(data, mapSeed);
                chunkDataDictionary.Add(newData.WorldPosition, newData);
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