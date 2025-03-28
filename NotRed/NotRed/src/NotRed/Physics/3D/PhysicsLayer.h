#pragma once

namespace NR
{
	struct PhysicsLayer
	{
		uint32_t ID;
		std::string Name;
		uint32_t BitValue;
		int32_t CollidesWith = 0;

		bool IsValid() const
		{
			return !Name.empty() && BitValue > 0;
		}
	};

	class PhysicsLayerManager
	{
	public:
		static uint32_t AddLayer(const std::string& name, bool setCollisions = true);
		static void RemoveLayer(uint32_t layerId);

		static void UpdateLayerName(uint32_t layerId, const std::string& newName);

		static void SetLayerCollision(uint32_t layerId, uint32_t otherLayer, bool shouldCollide);
		static std::vector<PhysicsLayer> GetLayerCollisions(uint32_t layerId);

		static const std::vector<PhysicsLayer>& GetLayers() { return sLayers; }
		static const std::vector<std::string>& GetLayerNames() { return sLayerNames; }

		static PhysicsLayer& GetLayer(uint32_t layerId);
		static PhysicsLayer& GetLayer(const std::string& layerName);
		static uint32_t GetLayerCount() { return (uint32_t)sLayers.size(); }

		static bool ShouldCollide(uint32_t layer1, uint32_t layer2);
		static bool IsLayerValid(uint32_t layerId);

		static void ClearLayers();

	private:
		static uint32_t GetNextLayerID();

	private:
		static std::vector<PhysicsLayer> sLayers;
		static std::vector<std::string> sLayerNames;
		static PhysicsLayer sNullLayer;
	};
}