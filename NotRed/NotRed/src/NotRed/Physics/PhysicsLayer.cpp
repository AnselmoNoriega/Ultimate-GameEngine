#include "nrpch.h"
#include "PhysicsLayer.h"

namespace NR
{
	template<typename T, typename ConditionFunction>
	static bool RemoveIfExists(std::vector<T>& vector, ConditionFunction condition)
	{
		auto it = vector.begin();
		while (it != vector.end())
		{
			if (condition(*it))
			{
				vector.erase(it);
				return true;
			}
			else
			{
				++it;
			}
		}

		return false;
	}

	uint32_t PhysicsLayerManager::AddLayer(const std::string& name, bool setCollisions)
	{
		uint32_t layerId = GetNextLayerID();
		PhysicsLayer layer = { layerId, name, (1 << layerId) };
		sLayers.push_back(layer);

		if (setCollisions)
		{
			for (const auto& layer2 : sLayers)
			{
				SetLayerCollision(layer.ID, layer2.ID, true);
			}
		}

		return layer.ID;
	}

	void PhysicsLayerManager::RemoveLayer(uint32_t layerId)
	{
		if (!IsLayerValid(layerId))
		{
			return;
		}

		PhysicsLayer& layerInfo = GetLayer(layerId);

		for (auto& otherLayer : sLayers)
		{
			if (otherLayer.ID == layerId)
			{
				continue;
			}

			if (otherLayer.CollidesWith & layerInfo.BitValue)
			{
				otherLayer.CollidesWith &= ~layerInfo.BitValue;
			}
		}

		RemoveIfExists<PhysicsLayer>(sLayers, [&](const PhysicsLayer& layer) { return layer.ID == layerId; });
	}

	void PhysicsLayerManager::SetLayerCollision(uint32_t layerId, uint32_t otherLayer, bool shouldCollide)
	{
		if (ShouldCollide(layerId, otherLayer) && shouldCollide)
		{
			return;
		}

		PhysicsLayer& layerInfo = GetLayer(layerId);
		PhysicsLayer& otherLayerInfo = GetLayer(otherLayer);

		if (shouldCollide)
		{
			layerInfo.CollidesWith |= otherLayerInfo.BitValue;
			otherLayerInfo.CollidesWith |= layerInfo.BitValue;
		}
		else
		{
			layerInfo.CollidesWith &= ~otherLayerInfo.BitValue;
			otherLayerInfo.CollidesWith &= ~layerInfo.BitValue;
		}
	}

	const std::vector<PhysicsLayer>& PhysicsLayerManager::GetLayerCollisions(uint32_t layerId)
	{
		const PhysicsLayer& layer = GetLayer(layerId);

		std::vector<PhysicsLayer> layers;
		for (const auto& otherLayer : sLayers)
		{
			if (otherLayer.ID == layerId)
			{
				continue;
			}

			if (layer.CollidesWith & otherLayer.BitValue)
			{
				layers.push_back(otherLayer);
			}
		}

		return layers;
	}

	PhysicsLayer& PhysicsLayerManager::GetLayer(uint32_t layerId)
	{
		for (auto& layer : sLayers)
		{
			if (layer.ID == layerId)
			{
				return layer;
			}
		}

		return sNullLayer;
	}

	PhysicsLayer& PhysicsLayerManager::GetLayer(const std::string& layerName)
	{
		for (auto& layer : sLayers)
		{
			if (layer.Name == layerName)
			{
				return layer;
			}
		}

		return sNullLayer;
	}

	bool PhysicsLayerManager::ShouldCollide(uint32_t layer1, uint32_t layer2)
	{
		return GetLayer(layer1).CollidesWith & GetLayer(layer2).BitValue;
	}

	bool PhysicsLayerManager::IsLayerValid(uint32_t layerId)
	{
		for (const auto& layer : sLayers)
		{
			if (layer.ID == layerId)
			{
				return true;
			}
		}

		return false;
	}

	void PhysicsLayerManager::ClearLayers()
	{
		sLayers.clear();
	}

	uint32_t PhysicsLayerManager::GetNextLayerID()
	{
		int32_t lastId = -1;

		for (const auto& layer : sLayers)
		{
			if (lastId != -1)
			{
				if (layer.ID != lastId + 1)
				{
					return lastId + 1;
				}
			}

			lastId = layer.ID;
		}

		return sLayers.size();
	}

	std::vector<PhysicsLayer> PhysicsLayerManager::sLayers;
	PhysicsLayer PhysicsLayerManager::sNullLayer = { 0, "NULL", 0, -1 };
}