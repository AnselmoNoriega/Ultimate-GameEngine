using NR;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using static World;

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

    public WorldData worldData;

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
        GenerateWorld(Vector3.Zero);
    }

    private void GenerateWorld(Vector3 position)
    {
        WorldGenerationData worldGenerationData = GetPositionsThatPlayerSees(position);

        foreach (Vector3 pos in worldGenerationData.chunkPositionsToRemove)
        {
            WorldDataHelper.RemoveChunk(this, pos);
        }
        foreach (Vector3 pos in worldGenerationData.chunkDataToRemove)
        {
            WorldDataHelper.RemoveChunkData(this, pos);
        }

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
            ChunkRenderer chunkRenderer = chunkObject.As<ChunkRenderer>();

            worldData.chunkDictionary.Add(data.WorldPosition, chunkRenderer);
            chunkRenderer.InitializeChunk(data);
            chunkRenderer.UpdateChunk(meshData);
        }

        OnWorldCreated?.Invoke();
    }

    internal bool SetVoxel(RaycastHit hit, VoxelType VoxelType)
    {
        ChunkRenderer chunk = FindEntityByID(hit.EntityID).As<ChunkRenderer>();
        if (chunk == null)
        {
            return false;
        }

        Vector3 pos = GetVoxelPos(hit);
        WorldDataHelper.SetVoxel(chunk.ChunkData.WorldRef, pos, VoxelType);
        chunk.SetIsModified(true);

        if (ChunkManager.IsOnEdge(chunk.ChunkData, pos))
        {
            List<Chunk> neighbourDataList = ChunkManager.GetEdgeNeighbourChunk(chunk.ChunkData, pos);
            foreach (Chunk neighbourData in neighbourDataList)
            {
                //neighbourData.modifiedByThePlayer = true;
                ChunkRenderer chunkToUpdate = WorldDataHelper.GetChunk(neighbourData.WorldRef, neighbourData.WorldPosition);
                if (chunkToUpdate != null)
                    chunkToUpdate.UpdateChunk();
            }
        }

        chunk.UpdateChunk();
        return true;
    }

    private Vector3 GetVoxelPos(RaycastHit hit)
    {
        Vector3 pos = new Vector3(
             GetVoxelPositionIn(hit.Position.x, hit.Normal.x),
             GetVoxelPositionIn(hit.Position.y, hit.Normal.y),
             GetVoxelPositionIn(hit.Position.z, hit.Normal.z)
             );

        return new Vector3((int)pos.x, (int)pos.y, (int)pos.z);
    }

    private float GetVoxelPositionIn(float pos, float normal)
    {
        if (Mathf.Abs(pos % 1) == 0.5f)
        {
            pos -= (normal / 2);
        }
        return (float)pos;
    }

    internal void RemoveChunk(ChunkRenderer chunk)
    {
        //TODO
        //chunk.gameObject.SetActive(false);
        Destroy(chunk);
    }

    private WorldGenerationData GetPositionsThatPlayerSees(Vector3 playerPosition)
    {
        List<Vector3> allChunkPositionsNeeded = WorldDataHelper.GetChunkPositionsAroundPlayer(this, playerPosition);
        List<Vector3> allChunkDataPositionsNeeded = WorldDataHelper.GetDataPositionsAroundPlayer(this, playerPosition);
        List<Vector3> chunkPositionsToCreate = WorldDataHelper.SelectPositonsToCreate(worldData, allChunkPositionsNeeded, playerPosition);
        List<Vector3> chunkDataPositionsToCreate = WorldDataHelper.SelectDataPositonsToCreate(worldData, allChunkDataPositionsNeeded, playerPosition);

        List<Vector3> chunkPositionsToRemove = WorldDataHelper.GetUnnededChunks(worldData, allChunkPositionsNeeded);
        List<Vector3> chunkDataToRemove = WorldDataHelper.GetUnnededData(worldData, allChunkDataPositionsNeeded);

        WorldGenerationData data = new WorldGenerationData
        {
            chunkPositionsToCreate = chunkPositionsToCreate,
            chunkDataPositionsToCreate = chunkDataPositionsToCreate,
            chunkPositionsToRemove = chunkPositionsToRemove,
            chunkDataToRemove = chunkDataToRemove,
            chunkPositionsToUpdate = new List<Vector3>()
        };
        return data;
    }

    internal void LoadAdditionalChunksRequest(Entity player)
    {
        GenerateWorld(new Vector3((int)player.Translation.x, (int)player.Translation.y, (int)player.Translation.z));
        OnNewChunksGenerated?.Invoke();
    }

    internal VoxelType GetVoxelFromChunkCoordinates(Chunk chunkData, float x, float y, float z)
    {
        Vector3 pos = ChunkManager.ChunkPositionFromVoxelCoords(this, x, y, z);
        Chunk containerChunk = null;
        worldData.chunkDataDictionary.TryGetValue(pos, out containerChunk);
        if (containerChunk == null)
        {
            return VoxelType.Nothing;
        }

        Vector3 VoxelInCHunkCoordinates = ChunkManager.GetVoxelInChunkCoordinates(containerChunk, new Vector3(x, y, z));

        return ChunkManager.GetVoxelFromChunkCoordinates(containerChunk, VoxelInCHunkCoordinates);
    }

    public struct WorldGenerationData
    {
        public List<Vector3> chunkPositionsToCreate;
        public List<Vector3> chunkDataPositionsToCreate;
        public List<Vector3> chunkPositionsToRemove;
        public List<Vector3> chunkDataToRemove;
        public List<Vector3> chunkPositionsToUpdate;
    }

    public class WorldData
    {
        public Dictionary<Vector3, Chunk> chunkDataDictionary;
        public Dictionary<Vector3, ChunkRenderer> chunkDictionary;
        public int chunkSize;
        public int chunkHeight;
    }
}