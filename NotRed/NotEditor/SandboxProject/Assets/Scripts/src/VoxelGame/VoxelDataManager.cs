using System.Collections.Generic;

public class VoxelDataManager
{
    public static float textureOffset = 0.001f;
    public static float tileSizeX, tileSizeY;
    public static Dictionary<VoxelType, TextureData> blockTextureDataDictionary = new Dictionary<VoxelType, TextureData>();
    public VoxelData textureData;

    private void Init()
    {
        foreach (var item in textureData.textureDataList)
        {
            if (blockTextureDataDictionary.ContainsKey(item.Type) == false)
            {
                blockTextureDataDictionary.Add(item.Type, item);
            };
        }

        tileSizeX = textureData.textureSizeX;
        tileSizeY = textureData.textureSizeY;
    }
}