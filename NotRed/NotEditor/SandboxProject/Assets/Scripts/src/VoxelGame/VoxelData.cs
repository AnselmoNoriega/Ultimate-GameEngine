using System;
using System.Collections;
using System.Collections.Generic;
using NR;

public class VoxelData : ScriptableObject
{
    public float textureSizeX, textureSizeY;
    public List<TextureData> textureDataList;
}

[Serializable]
public class TextureData
{
    public VoxelType Type;
    public Vector2 Up, Down, Side;
    public bool IsSolid = true;
    public bool GeneratesCollider = true;
}