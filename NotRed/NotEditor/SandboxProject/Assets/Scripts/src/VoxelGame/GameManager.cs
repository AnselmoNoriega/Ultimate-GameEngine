using NR;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;

public class GameManager : Entity
{
    public Prefab playerPrefab;
    private Entity player;
    public Vector3 currentPlayerChunkPosition;
    private Vector3 currentChunkCenter = Vector3.Zero;

    public Entity world;

    public float detectionTime = 1.0f;

    public void SpawnPlayer()
    {
        if (player != null)
        {
            return;
        }

        Vector3 raycastStartposition = new Vector3(world.As<World>().chunkSize / 2, 100, world.As<World>().chunkSize / 2);
        RaycastHit hit;

        if (Physics.Raycast(raycastStartposition, Vector3.Down, 120.0f, out hit))
        {
            player = Instantiate(playerPrefab, hit.Position + Vector3.Up);
            StartCheckingTheMap();
        }
        else
        {
            player = Instantiate(playerPrefab,Vector3.Zero);
            StartCheckingTheMap();
        }
    }

    public void StartCheckingTheMap()
    {
        SetCurrentChunkCoordinates();
        StopAllCoroutines();
        StartCoroutine(CheckIfShouldLoadNextPosition());
    }

    IEnumerator CheckIfShouldLoadNextPosition()
    {
        yield return new WaitForSeconds(detectionTime);
        if (
            Mathf.Abs(currentChunkCenter.x - player.Translation.x) > world.As<World>().chunkSize ||
            Mathf.Abs(currentChunkCenter.z - player.Translation.z) > world.As<World>().chunkSize ||
            (Mathf.Abs(currentPlayerChunkPosition.y - player.Translation.y) > world.As<World>().chunkHeight)
            )
        {
            world.As<World>().LoadAdditionalChunksRequest(player);
        }
        else
        {
            StartCoroutine(CheckIfShouldLoadNextPosition());
        }
    }

    private void SetCurrentChunkCoordinates()
    {
        currentPlayerChunkPosition = WorldDataHelper.ChunkPositionFromBlockCoords(world.As<World>(), player.Translation);
        currentChunkCenter.x = currentPlayerChunkPosition.x + world.As<World>().chunkSize / 2;
        currentChunkCenter.z = currentPlayerChunkPosition.z + world.As<World>().chunkSize / 2;
    }
}