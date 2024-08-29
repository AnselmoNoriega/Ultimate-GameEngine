#pragma once

#include "NotRed/Scene/Entity.h"

namespace NR
{
	enum class ForceMode : uint16_t
	{
		Force,
		Impulse,
		VelocityChange,
		Acceleration
	};

	enum class FilterGroup : uint32_t
	{
		Static = 1 << 0,
		Dynamic = 1 << 1,
		Kinematic = 1 << 2,
		All = Static | Dynamic | Kinematic
	};

	struct SceneParams
	{
		glm::vec3 Gravity = { 0.0f, -9.81f, 0.0f };
	};

	struct PhysicsLayer
	{
		uint32_t ID;
		std::string Name;
		uint32_t BitValue;
	};

	class PhysicsLayerManager
	{
	public:
		static uint32_t AddLayer(const std::string& name);
		static void RemoveLayer(uint32_t layerId);

		static void SetLayerCollision(uint32_t layerId, uint32_t otherLayer, bool collides);
		static const std::vector<PhysicsLayer>& GetLayerCollisions(uint32_t layerId);

		static const PhysicsLayer& GetLayerInfo(uint32_t layerId);
		static const PhysicsLayer& GetLayerInfo(const std::string& layerName);
		static uint32_t GetLayerCount() { return sLayers.size(); }

		static const std::vector<std::string>& GetLayerNames();

		static void ClearLayers();

	private:
		static void Init();
		static void Shutdown();

	private:
		static std::unordered_map<uint32_t, PhysicsLayer> sLayers;
		static std::unordered_map<uint32_t, std::vector<PhysicsLayer>> sLayerCollisions;

	private:
		friend class PhysicsManager;
	};

	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void CreateScene(const SceneParams& params);
		static void CreateActor(Entity e, int entityCount);

		static void Simulate(float dt);

		static void DestroyScene();

		static void ConnectVisualDebugger();
		static void DisconnectVisualDebugger();
	};
}