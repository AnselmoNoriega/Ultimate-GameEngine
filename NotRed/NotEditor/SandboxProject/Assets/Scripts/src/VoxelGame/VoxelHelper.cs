using NR;
using System.Collections;
using System.Collections.Generic;

public static class BlockHelper
{
    private static Direction[] directions =
    {
        Direction.backwards,
        Direction.down,
        Direction.foreward,
        Direction.left,
        Direction.right,
        Direction.up
    };

    public static MeshData GetMeshData(Chunk chunk, float x, float y, float z, MeshData meshData, VoxelType blockType)
    {
        if (blockType == VoxelType.Air || blockType == VoxelType.Nothing)
        {
            return meshData;
        }

        foreach (Direction direction in directions)
        {
            var neighbourBlockCoordinates = new Vector3(x, y, z) + direction.GetVector();
            var neighbourVoxelType = ChunkManager.GetBlockFromChunkCoordinates(chunk, neighbourBlockCoordinates);
            if (neighbourVoxelType != VoxelType.Nothing && VoxelDataManager.blockTextureDataDictionary[neighbourVoxelType].IsSolid == false)
            {
                if (blockType == VoxelType.Water)
                {
                    if (neighbourVoxelType == VoxelType.Air)
                    {
                        meshData.WaterMesh = GetFaceDataIn(direction, chunk, x, y, z, meshData.WaterMesh, blockType);
                    }
                }
                else
                {
                    meshData = GetFaceDataIn(direction, chunk, x, y, z, meshData, blockType);
                }
            }
        }
        return meshData;
    }

    public static MeshData GetFaceDataIn(Direction direction, Chunk chunk, float x, float y, float z, MeshData meshData, VoxelType blockType)
    {
        GetFaceVertices(direction, x, y, z, meshData, blockType);
        meshData.AddQuadTriangles();
        meshData.TextureCoords.AddRange(FaceTextureCoords(direction, blockType));
        return meshData;
    }

    public static void GetFaceVertices(Direction direction, float x, float y, float z, MeshData meshData, VoxelType blockType)
    {
        //order of vertices matters for the normals and how we render the mesh
        switch (direction)
        {
            case Direction.backwards:
                {
                    meshData.AddVertex(new Vector3(x - 0.5f, y - 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y + 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y + 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y - 0.5f, z - 0.5f));
                    break;
                }
            case Direction.foreward:
                {
                    meshData.AddVertex(new Vector3(x + 0.5f, y - 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y + 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y + 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y - 0.5f, z + 0.5f));
                    break;
                }
            case Direction.left:
                {
                    meshData.AddVertex(new Vector3(x - 0.5f, y - 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y + 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y + 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y - 0.5f, z - 0.5f));
                    break;
                }
            case Direction.right:
                {
                    meshData.AddVertex(new Vector3(x + 0.5f, y - 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y + 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y + 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y - 0.5f, z + 0.5f));
                    break;
                }
            case Direction.down:
                {
                    meshData.AddVertex(new Vector3(x - 0.5f, y - 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y - 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y - 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y - 0.5f, z + 0.5f));
                    break;
                }
            case Direction.up:
                {
                    meshData.AddVertex(new Vector3(x - 0.5f, y + 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y + 0.5f, z + 0.5f));
                    meshData.AddVertex(new Vector3(x + 0.5f, y + 0.5f, z - 0.5f));
                    meshData.AddVertex(new Vector3(x - 0.5f, y + 0.5f, z - 0.5f));
                    break;
                }
            default:
                break;
        }
    }

    public static Vector2[] FaceTextureCoords(Direction direction, VoxelType blockType)
    {
        Vector2[] textureCoords = new Vector2[4];
        var tilePos = TexturePosition(direction, blockType);

        textureCoords[0] = new Vector2(
            VoxelDataManager.tileSizeX * tilePos.x + VoxelDataManager.tileSizeX - VoxelDataManager.textureOffset,
            VoxelDataManager.tileSizeY * tilePos.y + VoxelDataManager.textureOffset);

        textureCoords[1] = new Vector2(
            VoxelDataManager.tileSizeX * tilePos.x + VoxelDataManager.tileSizeX - VoxelDataManager.textureOffset,
            VoxelDataManager.tileSizeY * tilePos.y + VoxelDataManager.tileSizeY - VoxelDataManager.textureOffset);

        textureCoords[2] = new Vector2(
            VoxelDataManager.tileSizeX * tilePos.x + VoxelDataManager.textureOffset,
            VoxelDataManager.tileSizeY * tilePos.y + VoxelDataManager.tileSizeY - VoxelDataManager.textureOffset);

        textureCoords[3] = new Vector2(
            VoxelDataManager.tileSizeX * tilePos.x + VoxelDataManager.textureOffset,
            VoxelDataManager.tileSizeY * tilePos.y + VoxelDataManager.textureOffset);

        return textureCoords;
    }

    public static Vector2 TexturePosition(Direction direction, VoxelType blockType)
    {
        var blockTextureData = VoxelDataManager.blockTextureDataDictionary[blockType];
        switch (direction)
        {
            case Direction.up:
                return blockTextureData.Up;
            case Direction.down:
                return blockTextureData.Down;
            default:
                return blockTextureData.Side;
        }
    }
}