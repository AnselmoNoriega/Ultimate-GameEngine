using NR;
using System;
using System.Collections.Generic;

public class World : Entity
{
    // World Info
    public int mapSizeInChunks = 6;
    public int chunkSize = 16, chunkHeight = 100;
    public Vector2 mapSeed;
    public int chunkDrawingRange = 8;

    // For Biome
    public int waterThreshold = 50;
    public float noiseScale = 0.03f;

    public Prefab chunkPrefab;

    // Helper
    public TerrainGenerator terrainGenerator;
    public Entity gameManagerEntity;

    // Extras
    public event Action OnWorldCreated, OnNewChunksGenerated;

    public WorldData worldData { get; private set; }

    public void Init()
    {
        worldData = new WorldData
        {
            chunkHeight = this.chunkHeight,
            chunkSize = this.chunkSize,
            chunkDataDictionary = new Dictionary<Vector3, Chunk>(),
            chunkDictionary = new Dictionary<Vector3, ChunkRenderer>()
        };

        terrainGenerator = new TerrainGenerator(new BiomeGenerator(waterThreshold, noiseScale));

        OnWorldCreated += gameManagerEntity.As<GameManager>().SpawnPlayer;
        OnNewChunksGenerated += gameManagerEntity.As<GameManager>().StartCheckingTheMap;

        GenerateWorld();
    }

    public void GenerateWorld()
    {
        WorldGenerationData worldGenerationData = GetPosisionsThatPlayerSees(Vector3.Zero);

        foreach (var pos in worldGenerationData.chunkDataPositionsToCreate)
        {
            Chunk data = new Chunk(chunkSize, chunkHeight, this, pos);
            Chunk newData = terrainGenerator.GenerateChunk(data, mapSeed);
            worldData.chunkDataDictionary.Add(pos, newData);
        }

        foreach (var pos in worldGenerationData.chunkPositionsToCreate)
        {
            Chunk data = worldData.chunkDataDictionary[pos];
            MeshData meshData = ChunkManager.GetChunkMeshData(data);
            Entity chunkObject = Instantiate(chunkPrefab, data.WorldPosition);
            ChunkRenderer chunkRenderer = new ChunkRenderer();

            worldData.chunkDictionary.Add(data.WorldPosition, chunkRenderer);
            chunkRenderer.InitializeChunk(data, chunkObject.GetComponent<MeshComponent>());
            chunkRenderer.UpdateChunk(meshData);
        }

        OnWorldCreated?.Invoke();
    }

    private WorldGenerationData GetPosisionsThatPlayerSees(Vector3 playerPosition)
    {
        List<Vector3> allChunkPositionsNeeded = WorldDataHelper.GetChunkPositionsAroundPlayer(this, playerPosition);
        List<Vector3> allChunkDataPositionsNeeded = WorldDataHelper.GetDataPositionsAroundPlayer(this, playerPosition);
        List<Vector3> chunkPositionsToCreate = WorldDataHelper.SelectPositonsToCreate(worldData, allChunkPositionsNeeded, playerPosition);
        List<Vector3> chunkDataPositionsToCreate = WorldDataHelper.SelectDataPositonsToCreate(worldData, allChunkDataPositionsNeeded, playerPosition);

        WorldGenerationData data = new WorldGenerationData
        {
            chunkPositionsToCreate = chunkPositionsToCreate,
            chunkDataPositionsToCreate = chunkDataPositionsToCreate,
            chunkPositionsToRemove = new List<Vector3>(),
            chunkDataToRemove = new List<Vector3>(),
            chunkPositionsToUpdate = new List<Vector3>()
        };
        return data;
    }

    internal void LoadAdditionalChunksRequest(Entity player)
    {
        Log.Info("Load more chunks");
        OnNewChunksGenerated?.Invoke();
    }

    private void GenerateVoxels(Chunk data)
    {

    }

    internal VoxelType GetBlockFromChunkCoordinates(Chunk chunkData, float x, float y, float z)
    {
        Vector3 pos = ChunkManager.ChunkPositionFromBlockCoords(this, x, y, z);
        Chunk containerChunk = null;
        worldData.chunkDataDictionary.TryGetValue(pos, out containerChunk);
        if (containerChunk == null)
        {
            return VoxelType.Nothing;
        }

        Vector3 blockInCHunkCoordinates = ChunkManager.GetBlockInChunkCoordinates(containerChunk, new Vector3(x, y, z));

        return ChunkManager.GetBlockFromChunkCoordinates(containerChunk, blockInCHunkCoordinates);
    }

    public struct WorldGenerationData
    {
        public List<Vector3> chunkPositionsToCreate;
        public List<Vector3> chunkDataPositionsToCreate;
        public List<Vector3> chunkPositionsToRemove;
        public List<Vector3> chunkDataToRemove;
        public List<Vector3> chunkPositionsToUpdate;
    }

    public struct WorldData
    {
        public Dictionary<Vector3, Chunk> chunkDataDictionary;
        public Dictionary<Vector3, ChunkRenderer> chunkDictionary;
        public int chunkSize;
        public int chunkHeight;
    }
}