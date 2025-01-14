using NR;
using System;
using System.Collections.Generic;

public class VoxelData
{
    public float textureSizeX, textureSizeY;
    // Should be a list
    public List<TextureData> textureDataList;
}

[Serializable]
public class TextureData
{
    public VoxelType Type;
    public Vector2 Up, Down, Side;
    public bool IsSolid = true;
}