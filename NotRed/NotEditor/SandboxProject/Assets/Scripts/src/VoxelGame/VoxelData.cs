using NR;
using System;
using System.Collections.Generic;

public class VoxelData
{
    public float textureSizeX, textureSizeY;
    public List<TextureData> textureDataList = new List<TextureData>();

    public VoxelData()
    {
        textureDataList.Add(new TextureData(
            VoxelType.Air,
            new Vector2(8, 0),
            new Vector2(8, 0),
            new Vector2(8, 0),
            false
            ));

        textureDataList.Add(new TextureData(
            VoxelType.Grass,
            new Vector2(6, 8),
            new Vector2(7, 4),
            new Vector2(7, 5)
            ));

        textureDataList.Add(new TextureData(
            VoxelType.Dirt,
            new Vector2(7, 4),
            new Vector2(7, 4),
            new Vector2(7, 4)
            ));

        textureDataList.Add(new TextureData(
            VoxelType.Stone,
            new Vector2(2, 2),
            new Vector2(2, 2),
            new Vector2(2, 2)
            ));

        textureDataList.Add(new TextureData(
            VoxelType.Wood,
            new Vector2(0, 0),
            new Vector2(0, 0),
            new Vector2(1, 9)
            ));

        textureDataList.Add(new TextureData(
            VoxelType.LeafTransparent,
            new Vector2(4, 1),
            new Vector2(4, 1),
            new Vector2(4, 1),
            false
            ));

        textureDataList.Add(new TextureData(
            VoxelType.LeafSolid,
            new Vector2(4, 1),
            new Vector2(4, 1),
            new Vector2(4, 1)
            ));

        textureDataList.Add(new TextureData(
            VoxelType.Water,
            new Vector2(0, 3),
            new Vector2(0, 3),
            new Vector2(0, 3),
            false
            ));

        textureDataList.Add(new TextureData(
            VoxelType.Sans,
            new Vector2(3, 3),
            new Vector2(3, 3),
            new Vector2(3, 3)
            ));
    }
}

[Serializable]
public class TextureData
{
    public VoxelType Type;
    public Vector2 Up, Down, Side;
    public bool IsSolid = true;

    public TextureData(
        VoxelType type,
        Vector2 up, Vector2 down, Vector2 side,
        bool isSolid = true
        )
    {
        Type = type;
        Up = up;
        Down = down;
        Side = side;
        IsSolid = isSolid;
    }
}