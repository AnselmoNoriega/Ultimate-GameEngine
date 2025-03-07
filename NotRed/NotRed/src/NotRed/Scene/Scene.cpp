#include "nrpch.h"
#include "Scene.h"

#include "Entity.h"
#include "Prefab.h"

#include "Components.h"

#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Script/ScriptEngine.h"

#include "NotRed/Asset/AssetManager.h"

#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Physics/3D/PhysicsManager.h"
#include "NotRed/Physics/3D/PhysicsSystem.h"
#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/AudioComponent.h"

#include "NotRed/Math/Math.h"
#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/SceneRenderer.h"

#include "NotRed/Debug/Profiler.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// Box2D
#include <box2d/box2d.h>
#include <assimp/scene.h>

// TEMP
#include "NotRed/Core/Input.h"

namespace NR
{
	static const std::string DefaultEntityName = "Entity";

	std::unordered_map<UUID, Scene*> sActiveScenes;

	struct SceneComponent
	{
		UUID SceneID;
	};

	namespace Utils {

		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix);

	}

	// TODO: MOVE TO PHYSICS FILE!
	class ContactListener2D : public b2ContactListener
	{
	public:
		virtual void BeginContact(b2Contact* contact) override
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

			if (!Scene::GetScene(a.GetSceneID())->IsPlaying())
			{
				return;
			}

			// TODO: improve these if checks
			if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
			{
				ScriptEngine::Collision2DBegin(a, b);
			}

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
			{
				ScriptEngine::Collision2DBegin(b, a);
			}
		}

		/// Called when two fixtures cease to touch.
		virtual void EndContact(b2Contact* contact) override
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

			if (!Scene::GetScene(a.GetSceneID())->IsPlaying())
				return;

			// TODO: improve these if checks
			if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
			{
				ScriptEngine::Collision2DEnd(a, b);
			}

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
			{
				ScriptEngine::Collision2DEnd(b, a);
			}
		}

		/// This is called after a contact is updated. This allows you to inspect a
		/// contact before it goes to the solver. If you are careful, you can modify the
		/// contact manifold (e.g. disable contact).
		/// A copy of the old manifold is provided so that you can detect changes.
		/// Note: this is called only for awake bodies.
		/// Note: this is called even when the number of contact points is zero.
		/// Note: this is not called for sensors.
		/// Note: if you set the number of contact points to zero, you will not
		/// get an EndContact callback. However, you may get a BeginContact callback
		/// the next step.
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
		{
			B2_NOT_USED(contact);
			B2_NOT_USED(oldManifold);
		}

		/// This lets you inspect a contact after the solver is finished. This is useful
		/// for inspecting impulses.
		/// Note: the contact manifold does not include time of impact impulses, which can be
		/// arbitrarily large if the sub-step is small. Hence the impulse is provided explicitly
		/// in a separate data structure.
		/// Note: this is only called for contacts that are touching, solid, and awake.
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
		{
			B2_NOT_USED(contact);
			B2_NOT_USED(impulse);
		}
	};

	static ContactListener2D sBox2DContactListener;

	struct Box2DWorldComponent
	{
		std::unique_ptr<b2World> World;
	};

	struct PhysicsSceneComponent
	{
		Ref<PhysicsScene> World;
	};

	// If this function becomes needed outside of Scene.cpp, then consider moving it to be a a constructor for AudioTransform.
	// That would couple Audio.h to Comopnents.h though, which is why I haven't done it that way.
	Audio::Transform GetAudioTransform(const TransformComponent& transformComponent) {
		auto rotationQuat = glm::quat(transformComponent.Rotation);
		return {
			transformComponent.Translation,
			rotationQuat * glm::vec3(0.0f, 0.0f, -1.0f) /* orientation */,
			rotationQuat * glm::vec3(0.0f, 1.0f, 0.0f)  /* up */
		};
	}

	Scene::Scene(const std::string& name, bool isEditorScene, bool initalize)
		: mName(name), mIsEditorScene(isEditorScene)
	{
		if (!initalize)
			return;

		mRegistry.on_construct<ScriptComponent>().connect<&Scene::ScriptComponentConstruct>(this);
		mRegistry.on_destroy<ScriptComponent>().connect<&Scene::ScriptComponentDestroy>(this);

		// This might not be the the best way, but Audio Engine needs to keep track of all audio component
		// to have an easy way to lookup a component associated with active sound.
		mRegistry.on_construct<AudioComponent>().connect<&Scene::AudioComponentConstruct>(this);
		mRegistry.on_destroy<AudioComponent>().connect<&Scene::AudioComponentDestroy>(this);

		mRegistry.on_construct<MeshColliderComponent>().connect<&Scene::MeshColliderComponentConstruct>(this);
		mRegistry.on_destroy<MeshColliderComponent>().connect<&Scene::MeshColliderComponentDestroy>(this);

		mSceneEntity = mRegistry.create();
		mRegistry.emplace<SceneComponent>(mSceneEntity, mSceneID);

		mSceneEntity = mRegistry.create();
		mRegistry.emplace<SceneComponent>(mSceneEntity, mSceneID);

		// TODO: Obviously not necessary in all cases
		Box2DWorldComponent& b2dWorld = mRegistry.emplace<Box2DWorldComponent>(mSceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.8f }));
		b2dWorld.World->SetContactListener(&sBox2DContactListener);

		mRegistry.emplace<PhysicsSceneComponent>(mSceneEntity, Ref<PhysicsScene>::Create(PhysicsManager::GetSettings()));

		sActiveScenes[mSceneID] = this;

		Init();
	}

	Scene::~Scene()
	{
		mRegistry.on_destroy<ScriptComponent>().disconnect();
		mRegistry.on_destroy<AudioComponent>().disconnect();

		mRegistry.clear();
		sActiveScenes.erase(mSceneID);
		ScriptEngine::SceneDestruct(mSceneID);
		AudioEngine::SceneDestruct(mSceneID);
	}

	void Scene::Init()
	{
		auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");
		mSkyboxMaterial = Material::Create(skyboxShader);
		mSkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);

		mSceneRenderer2D = Ref<Renderer2D>::Create();
	}

	// Merge Update/Render into one function?
	void Scene::UpdateRuntime(float dt)
	{
		NR_PROFILE_FUNC();

		// Box2D physics
		auto sceneView = mRegistry.view<Box2DWorldComponent>();
		auto& box2DWorld = mRegistry.get<Box2DWorldComponent>(sceneView.front()).World;
		int32_t velocityIterations = 6;
		int32_t positionIterations = 2;
		{
			NR_PROFILE_FUNC("Box2DWorld::Step");
			box2DWorld->Step(dt, velocityIterations, positionIterations);
		}

		{
			auto view = mRegistry.view<RigidBody2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& rb2d = e.GetComponent<RigidBody2DComponent>();
				b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);

				auto& position = body->GetPosition();
				auto& transform = e.GetComponent<TransformComponent>();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}

		auto physicsScene = GetPhysicsScene();

		if (mShouldSimulate)
		{
			if (mIsPlaying)
			{
				NR_PROFILE_FUNC("Scene::Update - C# Update");
				auto view = mRegistry.view<ScriptComponent>();
				for (auto entity : view)
				{
					Entity e = { entity, this };
					if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
						ScriptEngine::UpdateEntity(e, dt);
				}

				for (auto&& fn : mPostUpdateQueue)
				{
					fn();
				}

				mPostUpdateQueue.clear();
			}

			UpdateAnimation(dt, true);

			physicsScene->Simulate(dt);
		}

		{	//--- Update Audio Listener ---
			//=============================
			
			NR_PROFILE_FUNC("Scene::Update - Update Audio Listener");
			auto view = mRegistry.view<AudioListenerComponent>();
			Entity listener;
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& listenerComponent = e.GetComponent<AudioListenerComponent>();
				if (listenerComponent.Active)
				{
					listener = e;
					auto transform = GetAudioTransform(GetWorldSpaceTransform(listener));
					AudioEngine::Get().UpdateListenerPosition(transform);
					AudioEngine::Get().UpdateListenerConeAttenuation(listenerComponent.ConeInnerAngleInRadians,
						listenerComponent.ConeOuterAngleInRadians,
						listenerComponent.ConeOuterGain);

					if (auto physicsActor = physicsScene->GetActor(listener))
					{
						if (physicsActor->IsDynamic())
						{
							AudioEngine::Get().UpdateListenerVelocity(physicsActor->GetVelocity());
						}
					}
					break;
				}
			}

			// If listener wasn't found, fallback to using main camera as an active listener
			if (listener.mEntityHandle == entt::null)
			{
				listener = GetMainCameraEntity();
				if (listener.mEntityHandle != entt::null)
				{
					// If camera was changed or destroyed during Runtime, it might not have Listener Component (?)
					if (!listener.HasComponent<AudioListenerComponent>())
					{
						listener.AddComponent<AudioListenerComponent>();
					}

					auto transform = GetAudioTransform(GetWorldSpaceTransform(listener));
					AudioEngine::Get().UpdateListenerPosition(transform);

					auto& listenerComponent = listener.GetComponent<AudioListenerComponent>();
					AudioEngine::Get().UpdateListenerConeAttenuation(listenerComponent.ConeInnerAngleInRadians,
						listenerComponent.ConeOuterAngleInRadians,
						listenerComponent.ConeOuterGain);

					if (auto physicsActor = physicsScene->GetActor(listener))
					{
						if (physicsActor->IsDynamic())
						{
							AudioEngine::Get().UpdateListenerVelocity(physicsActor->GetVelocity());
						}
					}
				}
			}
			//! If we don't have Main Camera in the scene, nor any user created Listeners,
			//! we won't be able to hear any sound!
			//! Unless there's a way to retrieve the editor's camera,
			//! which is not a part of this runtime scene.

			//? allow updating listener settings at runtime?
			//auto& audioListenerComponent = view.get(entity); 
		}

		{	//--- Update Audio Components ---
			//===============================

			NR_PROFILE_FUNC("Scene::Update - Update Audio Components");
			auto view = mRegistry.view<AudioComponent>();

			std::vector<Entity> deadEntities;
			deadEntities.reserve(view.size());

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& audioComponent = e.GetComponent<AudioComponent>();

				// 1. Handle Audio Components marked for Auto Destroy

				// AutoDestroy flag is only set for "one-shot" sounds
				if (audioComponent.AutoDestroy && audioComponent.MarkedForDestroy)
				{
					deadEntities.push_back(e);
					continue;
				}

				// 2. Update positions of associated sound sources
				auto transform = GetAudioTransform(GetWorldSpaceTransform(e));

				// 3. Update velocities of associated sound sources
				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				if (auto physicsActor = physicsScene->GetActor(e))
				{
					if (physicsActor->IsDynamic())
						velocity = physicsActor->GetVelocity();
				}

				updateData.emplace_back(SoundSourceUpdateData{ e.GetID(),
					transform,
					velocity,
					audioComponent.VolumeMultiplier,
					audioComponent.PitchMultiplier });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			//-----------------------------------------------------------------------
			AudioEngine::Get().SubmitSourceUpdateData(updateData);

			for (int i = (int)deadEntities.size() - 1; i >= 0; i--)
			{
				DestroyEntity(deadEntities[i]);
			}
		}

	}

	void Scene::UpdateEditor(float dt)
	{
		NR_PROFILE_FUNC();
		UpdateAnimation(dt, false);
	}

	void Scene::RenderRuntime(Ref<SceneRenderer> renderer, float dt)
	{
		NR_PROFILE_FUNC();

		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////
		Entity cameraEntity = GetMainCameraEntity();
		if (!cameraEntity)
			return;

		glm::mat4 cameraViewMatrix = glm::inverse(GetWorldSpaceTransformMatrix(cameraEntity));
		NR_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
		SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
		camera.SetViewportSize(mViewportWidth, mViewportHeight);

		// Process lights
		{
			mLightEnvironment = LightEnvironment();
			// Directional Lights	
			{
				auto lights = mRegistry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : lights)
				{
					auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
					glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
					mLightEnvironment.DirectionalLights[directionalLightIndex++] =
					{
						direction,
						lightComponent.Radiance,
						lightComponent.Intensity,
						lightComponent.CastShadows
					};
				}
				//Point Lights
				{
					auto pointLights = mRegistry.group<PointLightComponent>(entt::get<TransformComponent>);
					mLightEnvironment.PointLights.resize(pointLights.size());
					uint32_t pointLightIndex = 0;
					for (auto e : pointLights)
					{
						Entity entity(e, this);
						auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(e);
						auto transform = entity.HasComponent<RigidBodyComponent>() ? entity.Transform() : GetWorldSpaceTransform(entity);
						mLightEnvironment.PointLights[pointLightIndex++] = {
							transform.Translation,
							lightComponent.Intensity,
							lightComponent.Radiance,
							lightComponent.MinRadius,
							lightComponent.Radius,
							lightComponent.Falloff,
							lightComponent.LightSize,
							lightComponent.CastsShadows,
						};
					}
				}

			}
		}

		{
			auto lights = mRegistry.group<SkyLightComponent>(entt::get<TransformComponent>);
			if (lights.empty())
			{
				mEnvironment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());
			}

			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				if (!AssetManager::IsAssetHandleValid(skyLightComponent.SceneEnvironment) && skyLightComponent.DynamicSky)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
					//skyLightComponent.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
					skyLightComponent.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(preethamEnv, preethamEnv);
				}
				mEnvironment = AssetManager::GetAsset<Environment>(skyLightComponent.SceneEnvironment);
				mEnvironmentIntensity = skyLightComponent.Intensity;
				mSkyboxLod = skyLightComponent.Lod;
				if (mEnvironment)
				{
					SetSkybox(mEnvironment->RadianceMap);
				}
			}
		}

		mSkyboxMaterial->Set("uUniforms.TextureLod", mSkyboxLod);

		renderer->SetScene(this);
		renderer->BeginScene({ camera, cameraViewMatrix, camera.GetPerspectiveNearClip(), camera.GetPerspectiveFarClip(), camera.GetPerspectiveVerticalFOV() });

		// Render Static Meshes
		{
			auto group = mRegistry.group<StaticMeshComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				NR_PROFILE_FUNC("Scene-SubmitStaticMesh");
				auto [transformComponent, staticMeshComponent] = group.get<TransformComponent, StaticMeshComponent>(entity);
				if (AssetManager::IsAssetHandleValid(staticMeshComponent.StaticMesh))
				{
					auto staticMesh = AssetManager::GetAsset<StaticMesh>(staticMeshComponent.StaticMesh);
					if (staticMesh && !staticMesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

						if (mSelectedEntity == entity)
						{
							renderer->SubmitSelectedStaticMesh(staticMesh, staticMeshComponent.Materials, transform);
						}
						else
						{
							renderer->SubmitStaticMesh(staticMesh, staticMeshComponent.Materials, transform);
						}
					}
				}
			}
		}

		// Render Dynamic Meshes
		{
			auto view = mRegistry.view<MeshComponent, TransformComponent>();
			for (auto entity : view)
			{
				NR_PROFILE_FUNC("Scene-SubmitDynamicMesh");
				auto [transformComponent, meshComponent] = view.get<TransformComponent, MeshComponent>(entity);
				if (AssetManager::IsAssetHandleValid(meshComponent.MeshHandle))
				{
					auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.MeshHandle);
					if (mesh && !mesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						if (mesh->IsRigged()) {
							if (e.HasComponent<AnimationComponent>()) {
								auto& anim = e.GetComponent<AnimationComponent>();
								if (AssetManager::IsAssetHandleValid(anim.AnimationController))
								{
									auto animationController = AssetManager::GetAsset<AnimationController>(anim.AnimationController);
									mesh->UpdateBoneTransforms(animationController->GetModelSpaceTransforms());
								}
								else
								{
									mesh->UpdateBoneTransforms({});
								}
							}
							else
							{
								mesh->UpdateBoneTransforms({});
							}
						}

						glm::mat4 transform = e.HasComponent<RigidBodyComponent>() ? e.Transform().GetTransform() : GetWorldSpaceTransformMatrix(e);
						if (mSelectedEntity == entity)
						{
							renderer->SubmitSelectedMesh(mesh, meshComponent.SubmeshIndex, meshComponent.Materials, transform);
						}
						else
						{
							renderer->SubmitMesh(mesh, meshComponent.SubmeshIndex, meshComponent.Materials, transform);
						}
					}
				}
			}
		}

		// Render Particles
		auto groupParticles = mRegistry.group<ParticleComponent>(entt::get<TransformComponent>);
		for (auto entity : groupParticles)
		{
			auto [transformComponent, particleComponent] = groupParticles.get<TransformComponent, ParticleComponent>(entity);
			if (particleComponent.MeshObj && !particleComponent.MeshObj->IsFlagSet(AssetFlag::Missing))
			{
				glm::mat4 transform = GetWorldSpaceTransformMatrix(Entity(entity, this));
				// TODO
				//renderer->SubmitParticles(particleComponent, transform);
			}
		}

		if (renderer->GetOptions().ShowPhysicsColliders != SceneRendererOptions::PhysicsColliderView::None)
		{
			RenderPhysicsDebug(renderer, true);
		}

		renderer->EndScene();
		/////////////////////////////////////////////////////////////////////
		// Render 2D
		if (renderer->GetFinalPassImage())
		{
			mSceneRenderer2D->BeginScene(camera.GetProjectionMatrix() * cameraViewMatrix, cameraViewMatrix);
			mSceneRenderer2D->SetTargetRenderPass(renderer->GetExternalCompositeRenderPass());
			{
#if TODO_SPRITES
				auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRenderer>);
				for (auto entity : group)
				{
					auto [transformComponent, spriteRendererComponent] = group.get<TransformComponent, SpriteRenderer>(entity);
					if (spriteRendererComponent.Texture)
						Renderer2D::DrawQuad(transformComponent.Transform, spriteRendererComponent.Texture, spriteRendererComponent.TilingFactor);
					else
						Renderer2D::DrawQuad(transformComponent.Transform, spriteRendererComponent.Color);
				}
#endif
				auto group = mRegistry.group<TransformComponent>(entt::get<TextComponent>);
				for (auto entity : group)
				{
					auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);
					if (textComponent.FontAsset == Font::GetDefaultFont()->Handle || !AssetManager::IsAssetHandleValid(textComponent.FontAsset))
						mSceneRenderer2D->DrawString(textComponent.TextString, Font::GetDefaultFont(), transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
					else
						mSceneRenderer2D->DrawString(textComponent.TextString, AssetManager::GetAsset<Font>(textComponent.FontAsset), transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
				}
			}

			mSceneRenderer2D->EndScene();
		}
	}

	void Scene::RenderEditor(Ref<SceneRenderer> renderer, float dt, const EditorCamera& editorCamera)
	{
		NR_PROFILE_FUNC();

		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		// Lighting
		{
			mLightEnvironment = LightEnvironment();
			//Directional Lights
			{
				auto dirLights = mRegistry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : dirLights)
				{
					auto [transformComponent, lightComponent] = dirLights.get<TransformComponent, DirectionalLightComponent>(entity);
					glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
					mLightEnvironment.DirectionalLights[directionalLightIndex++] =
					{
						direction,
						lightComponent.Radiance,
						lightComponent.Intensity,
						lightComponent.CastShadows
					};
				}
			}
			//Point Lights
			{
				auto pointLights = mRegistry.group<PointLightComponent>(entt::get<TransformComponent>);
				mLightEnvironment.PointLights.resize(pointLights.size());
				uint32_t pointLightIndex = 0;
				for (auto entity : pointLights)
				{
					auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(entity);
					auto transform = GetWorldSpaceTransform(Entity(entity, this));
					mLightEnvironment.PointLights[pointLightIndex++] = {
						transform.Translation,
						lightComponent.Intensity,
						lightComponent.Radiance,
						lightComponent.MinRadius,
						lightComponent.Radius,
						lightComponent.Falloff,
						lightComponent.LightSize,
						lightComponent.CastsShadows,
					};

				}
			}
		}

		{
			auto lights = mRegistry.group<SkyLightComponent>(entt::get<TransformComponent>);
			if (lights.empty())
				mEnvironment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());

			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				if (!AssetManager::IsAssetHandleValid(skyLightComponent.SceneEnvironment) && skyLightComponent.DynamicSky)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
					//skyLightComponent.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
					skyLightComponent.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(preethamEnv, preethamEnv);
				}
				mEnvironment = AssetManager::GetAsset<Environment>(skyLightComponent.SceneEnvironment);
				mEnvironmentIntensity = skyLightComponent.Intensity;
				mSkyboxLod = skyLightComponent.Lod;
				if (mEnvironment)
				{
					SetSkybox(mEnvironment->RadianceMap);
				}
			}
		}

		mSkyboxMaterial->Set("uUniforms.TextureLod", mSkyboxLod);

		renderer->SetScene(this);
		renderer->BeginScene({ editorCamera, editorCamera.GetViewMatrix(), editorCamera.GetNearClip(), editorCamera.GetFarClip(), editorCamera.GetVerticalFOV() });

		// Render Static Meshes
		{
			auto group = mRegistry.group<StaticMeshComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				auto [transformComponent, staticMeshComponent] = group.get<TransformComponent, StaticMeshComponent>(entity);
				if (AssetManager::IsAssetHandleValid(staticMeshComponent.StaticMesh))
				{
					auto staticMesh = AssetManager::GetAsset<StaticMesh>(staticMeshComponent.StaticMesh);
					NR_CORE_ASSERT(staticMesh, "StaticMesh does not exist!");
					if (staticMesh && !staticMesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

						if (mSelectedEntity == entity)
							renderer->SubmitSelectedStaticMesh(staticMesh, staticMeshComponent.Materials, transform);
						else
							renderer->SubmitStaticMesh(staticMesh, staticMeshComponent.Materials, transform);
					}
				}
			}
		}

		// Render Dynamic Meshes
		{
			auto view = mRegistry.view<MeshComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto [transformComponent, meshComponent] = view.get<TransformComponent, MeshComponent>(entity);
				if (AssetManager::IsAssetHandleValid(meshComponent.MeshHandle))
				{
					auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.MeshHandle);
					NR_CORE_ASSERT(mesh, "Mesh does not exist!");
					if (mesh && !mesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						if (mesh->IsRigged())
						{
							if (e.HasComponent<AnimationComponent>()) 
							{
								auto& anim = e.GetComponent<AnimationComponent>();
								if (AssetManager::IsAssetHandleValid(anim.AnimationController))
								{
									auto animationController = AssetManager::GetAsset<AnimationController>(anim.AnimationController);
									mesh->UpdateBoneTransforms(animationController->GetModelSpaceTransforms());
								}
								else
								{
									mesh->UpdateBoneTransforms({});
								}
							}
							else
							{
								mesh->UpdateBoneTransforms({});
							}
						}

						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

						if (mSelectedEntity == entity)
						{
							renderer->SubmitSelectedMesh(mesh, meshComponent.SubmeshIndex, meshComponent.Materials, transform);
						}
						else
						{
							renderer->SubmitMesh(mesh, meshComponent.SubmeshIndex, meshComponent.Materials, transform);
						}
					}
				}
			}
		}

		// Render Particles
		auto groupParticles = mRegistry.group<ParticleComponent>(entt::get<TransformComponent>);
		for (auto entity : groupParticles)
		{
			auto [transformComponent, particleComponent] = groupParticles.get<TransformComponent, ParticleComponent>(entity);
			if (particleComponent.MeshObj && !particleComponent.MeshObj->IsFlagSet(AssetFlag::Missing))
			{
				glm::mat4 transform = GetWorldSpaceTransformMatrix(Entity(entity, this));
				//TODO
				//renderer->SubmitParticles(particleComponent, transform);
			}
		}

		if (renderer->GetOptions().ShowPhysicsColliders != SceneRendererOptions::PhysicsColliderView::None)
		{
			RenderPhysicsDebug(renderer, false);
		}

		renderer->EndScene();
		/////////////////////////////////////////////////////////////////////

		{
			const auto& camPosition = editorCamera.GetPosition();
			auto camDirection = glm::rotate(editorCamera.GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
			auto camUp = glm::rotate(editorCamera.GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
			Audio::Transform transform{ camPosition, camDirection, camUp };

			AudioEngine::Get().UpdateListenerPosition(transform);
		}


		{	//--- Update Audio Component (editor scene update) ---
			//====================================================

			auto view = mRegistry.view<AudioComponent>();

			std::vector<Entity> deadEntities;

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& audioComponent = e.GetComponent<AudioComponent>();

				// AutoDestroy flag is only set for "one-shot" sounds
				if (audioComponent.AutoDestroy && audioComponent.MarkedForDestroy)
				{
					deadEntities.push_back(e);
					continue;
				}

				auto transform = GetAudioTransform(GetWorldSpaceTransform(e));

				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				updateData.emplace_back(SoundSourceUpdateData{ e.GetID(),
					transform,
					velocity,
					audioComponent.VolumeMultiplier,
					audioComponent.PitchMultiplier });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			//-----------------------------------------------------------------------
			AudioEngine::Get().SubmitSourceUpdateData(updateData);

			for (int i = (int)deadEntities.size() - 1; i >= 0; i--)
			{
				DestroyEntity(deadEntities[i]);
			}
		}

		// Render 2D
		if (renderer->GetFinalPassImage())
		{
			mSceneRenderer2D->BeginScene(editorCamera.GetViewProjection(), editorCamera.GetViewMatrix());
			mSceneRenderer2D->SetTargetRenderPass(renderer->GetExternalCompositeRenderPass());
			{
#if TODO_SPRITES
				auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRenderer>);
				for (auto entity : group)
				{
					auto [transformComponent, spriteRendererComponent] = group.get<TransformComponent, SpriteRenderer>(entity);
					if (spriteRendererComponent.Texture)
					{
						Renderer2D::DrawQuad(transformComponent.Transform, spriteRendererComponent.Texture, spriteRendererComponent.TilingFactor);
					}
					else
					{
						Renderer2D::DrawQuad(transformComponent.Transform, spriteRendererComponent.Color);
					}
				}
#endif
				auto group = mRegistry.group<TransformComponent>(entt::get<TextComponent>);
				for (auto entity : group)
				{
					auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);
					if (textComponent.FontAsset == Font::GetDefaultFont()->Handle || !AssetManager::IsAssetHandleValid(textComponent.FontAsset))
						mSceneRenderer2D->DrawString(textComponent.TextString, Font::GetDefaultFont(), transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
					else
						mSceneRenderer2D->DrawString(textComponent.TextString, AssetManager::GetAsset<Font>(textComponent.FontAsset), transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
				}
			}

			mSceneRenderer2D->EndScene();
		}
	}

	void Scene::RenderSimulation(Ref<SceneRenderer> renderer, float dt, const EditorCamera& editorCamera)
	{
		NR_PROFILE_FUNC();

		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		// Lighting
		{
			mLightEnvironment = LightEnvironment();
			//Directional Lights
			{
				auto dirLights = mRegistry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : dirLights)
				{
					auto [transformComponent, lightComponent] = dirLights.get<TransformComponent, DirectionalLightComponent>(entity);
					glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
					mLightEnvironment.DirectionalLights[directionalLightIndex++] =
					{
						direction,
						lightComponent.Radiance,
						lightComponent.Intensity,
						lightComponent.CastShadows
					};
				}
			}
			//Point Lights
			{
				auto pointLights = mRegistry.group<PointLightComponent>(entt::get<TransformComponent>);
				mLightEnvironment.PointLights.resize(pointLights.size());
				uint32_t pointLightIndex = 0;
				for (auto e : pointLights)
				{
					Entity entity(e, this);
					auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(entity);
					auto transform = entity.HasComponent<RigidBodyComponent>() ? entity.Transform() : GetWorldSpaceTransform(entity);
					mLightEnvironment.PointLights[pointLightIndex++] = {
						transform.Translation,
						lightComponent.Intensity,
						lightComponent.Radiance,
						lightComponent.MinRadius,
						lightComponent.Radius,
						lightComponent.Falloff,
						lightComponent.LightSize,
						lightComponent.CastsShadows,
					};

				}
			}
		}

		{
			auto lights = mRegistry.group<SkyLightComponent>(entt::get<TransformComponent>);
			if (lights.empty())
				mEnvironment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());

			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				if (!AssetManager::IsAssetHandleValid(skyLightComponent.SceneEnvironment) && skyLightComponent.DynamicSky)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
					skyLightComponent.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(preethamEnv, preethamEnv);
				}
				mEnvironment = AssetManager::GetAsset<Environment>(skyLightComponent.SceneEnvironment);
				mEnvironmentIntensity = skyLightComponent.Intensity;
				mSkyboxLod = skyLightComponent.Lod;
				if (mEnvironment)
				{
					SetSkybox(mEnvironment->RadianceMap);
				}
			}
		}

		mSkyboxMaterial->Set("uUniforms.TextureLod", mSkyboxLod);

		auto group = mRegistry.group<MeshComponent>(entt::get<TransformComponent>);
		renderer->SetScene(this);
		renderer->BeginScene({ editorCamera, editorCamera.GetViewMatrix(), editorCamera.GetNearClip(), editorCamera.GetFarClip(), editorCamera.GetVerticalFOV() });

		// Render Static Meshes
		{
			auto group = mRegistry.group<StaticMeshComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				auto [transformComponent, staticMeshComponent] = group.get<TransformComponent, StaticMeshComponent>(entity);
				if (AssetManager::IsAssetHandleValid(staticMeshComponent.StaticMesh))
				{
					auto staticMesh = AssetManager::GetAsset<StaticMesh>(staticMeshComponent.StaticMesh);
					if (!staticMesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

						if (mSelectedEntity == entity)
							renderer->SubmitSelectedStaticMesh(staticMesh, staticMeshComponent.Materials, transform);
						else
							renderer->SubmitStaticMesh(staticMesh, staticMeshComponent.Materials, transform);
					}
				}
			}
		}

		// Render Dynamic Meshes
		{
			auto view = mRegistry.view<MeshComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto [transformComponent, meshComponent] = view.get<TransformComponent, MeshComponent>(entity);
				if (AssetManager::IsAssetHandleValid(meshComponent.MeshHandle))
				{
					auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.MeshHandle);
					if (mesh && !mesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						if (mesh->IsRigged())
						{
							if (e.HasComponent<AnimationComponent>()) {
								// QUESTION: Would it be better to render all the static meshes first, and then all the animated ones?
								//           Does chopping and changing between static graphics pipeline and animated one slow things down?
								auto& anim = e.GetComponent<AnimationComponent>();
								if (AssetManager::IsAssetHandleValid(anim.AnimationController))
								{
									auto animationController = AssetManager::GetAsset<AnimationController>(anim.AnimationController);
									mesh->UpdateBoneTransforms(animationController->GetModelSpaceTransforms());
								}
								else
								{
									mesh->UpdateBoneTransforms({});
								}
							}
							else
							{
								mesh->UpdateBoneTransforms({});
							}
						}

						glm::mat4 transform = e.HasComponent<RigidBodyComponent>() ? e.Transform().GetTransform() : GetWorldSpaceTransformMatrix(e);

						// TODO: Should we render (logically)
						if (mSelectedEntity == entity)
						{
							renderer->SubmitSelectedMesh(mesh, meshComponent.SubmeshIndex, meshComponent.Materials, transform);
						}
						else
						{
							renderer->SubmitMesh(mesh, meshComponent.SubmeshIndex, meshComponent.Materials, transform);
						}
					}
				}
			}
		}

		auto groupParticles = mRegistry.group<ParticleComponent>(entt::get<TransformComponent>);
		for (auto entity : groupParticles)
		{
			auto [transformComponent, particleComponent] = groupParticles.get<TransformComponent, ParticleComponent>(entity);
			if (particleComponent.MeshObj && !particleComponent.MeshObj->IsFlagSet(AssetFlag::Missing))
			{
				glm::mat4 transform = GetWorldSpaceTransformMatrix(Entity(entity, this));
				//TODO
				//renderer->SubmitParticles(particleComponent, transform);
			}
		}

		if (renderer->GetOptions().ShowPhysicsColliders != SceneRendererOptions::PhysicsColliderView::None)
		{
			RenderPhysicsDebug(renderer, true);
		}

		renderer->EndScene();

		// Render 2D
		if (renderer->GetFinalPassImage())
		{
			mSceneRenderer2D->BeginScene(editorCamera.GetViewProjection(), editorCamera.GetViewMatrix());
			mSceneRenderer2D->SetTargetRenderPass(renderer->GetExternalCompositeRenderPass());
			{
#if TODO_SPRITES
				auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRenderer>);
				for (auto entity : group)
				{
					auto [transformComponent, spriteRendererComponent] = group.get<TransformComponent, SpriteRenderer>(entity);
					if (spriteRendererComponent.Texture)
						Renderer2D::DrawQuad(transformComponent.Transform, spriteRendererComponent.Texture, spriteRendererComponent.TilingFactor);
					else
						Renderer2D::DrawQuad(transformComponent.Transform, spriteRendererComponent.Color);
				}
#endif
				auto group = mRegistry.group<TransformComponent>(entt::get<TextComponent>);
				for (auto entity : group)
				{
					auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);
					if (textComponent.FontAsset == Font::GetDefaultFont()->Handle || !AssetManager::IsAssetHandleValid(textComponent.FontAsset))
						mSceneRenderer2D->DrawString(textComponent.TextString, Font::GetDefaultFont(), transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
					else
						mSceneRenderer2D->DrawString(textComponent.TextString, AssetManager::GetAsset<Font>(textComponent.FontAsset), transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
				}
			}

			mSceneRenderer2D->EndScene();
		}
	}

	void Scene::RenderPhysicsDebug(Ref<SceneRenderer> renderer, bool runtime)
	{
		{
			auto view = mRegistry.view<BoxColliderComponent>();
			Ref<StaticMesh> boxDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetBoxDebugMesh());
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = (runtime && e.HasComponent<RigidBodyComponent>()) ? e.Transform().GetTransform() : GetWorldSpaceTransformMatrix(e);
				auto& collider = e.GetComponent<BoxColliderComponent>();
				glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), collider.Size);
				renderer->SubmitPhysicsStaticDebugMesh(boxDebugMesh, transform * colliderTransform);
			}
		}

		{
			auto view = mRegistry.view<SphereColliderComponent>();
			Ref<StaticMesh> sphereDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetSphereDebugMesh());
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = (runtime && e.HasComponent<RigidBodyComponent>()) ? e.Transform().GetTransform() : GetWorldSpaceTransformMatrix(e);
				auto& collider = e.GetComponent<SphereColliderComponent>();
				glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), glm::vec3(collider.Radius * 2.0f));
				renderer->SubmitPhysicsStaticDebugMesh(sphereDebugMesh, transform * colliderTransform);
			}
		}

		{
			auto view = mRegistry.view<CapsuleColliderComponent>();
			Ref<StaticMesh> capsuleDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetCapsuleDebugMesh());
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = (runtime && e.HasComponent<RigidBodyComponent>()) ? e.Transform().GetTransform() : GetWorldSpaceTransformMatrix(e);
				auto& collider = e.GetComponent<CapsuleColliderComponent>();
				glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), glm::vec3(collider.Radius * 2.0f, collider.Height, collider.Radius * 2.0f));
				renderer->SubmitPhysicsStaticDebugMesh(capsuleDebugMesh, transform * colliderTransform);
			}
		}

		{
			auto view = mRegistry.view<MeshColliderComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = (runtime && e.HasComponent<RigidBodyComponent>()) ? e.Transform().GetTransform() : GetWorldSpaceTransformMatrix(e);
				auto& collider = e.GetComponent<MeshColliderComponent>();

				if (e.HasComponent<MeshComponent>())
				{
					Ref<Mesh> debugMesh = PhysicsSystem::GetMeshCache().GetDebugMesh(collider.CollisionMesh);
					if (debugMesh)
					{
						renderer->SubmitPhysicsDebugMesh(debugMesh, collider.SubmeshIndex, transform);
					}
				}
				else if (e.HasComponent<StaticMeshComponent>())
				{
					Ref<StaticMesh> debugMesh = PhysicsSystem::GetMeshCache().GetDebugStaticMesh(collider.CollisionMesh);
					if (debugMesh)
					{
						renderer->SubmitPhysicsStaticDebugMesh(debugMesh, transform);
					}
				}

			}
		}
	}

	void Scene::OnEvent(Event& e)
	{
	}

	void Scene::RuntimeStart()
	{
		NR_PROFILE_FUNC();

		ScriptEngine::SetSceneContext(this);
		AudioEngine::SetSceneContext(this);

		// Box2D physics
		auto sceneView = mRegistry.view<Box2DWorldComponent>();
		auto& world = mRegistry.get<Box2DWorldComponent>(sceneView.front()).World;

		{
			auto view = mRegistry.view<RigidBody2DComponent>();
			mPhysics2DBodyEntityBuffer = new Entity[view.size()];
			uint32_t physicsBodyEntityBufferIndex = 0;
			for (auto entity : view)
			{
				Entity e = { entity, this };
				UUID entityID = e.GetComponent<IDComponent>().ID;
				TransformComponent& transform = e.GetComponent<TransformComponent>();
				auto& rigidBody2D = mRegistry.get<RigidBody2DComponent>(entity);

				b2BodyDef bodyDef;
				if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static)
				{
					bodyDef.type = b2_staticBody;
				}
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic)
				{
					bodyDef.type = b2_dynamicBody;
				}
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic)
				{
					bodyDef.type = b2_kinematicBody;
				}
				bodyDef.position.Set(transform.Translation.x, transform.Translation.y);

				bodyDef.angle = transform.Rotation.z;

				b2Body* body = world->CreateBody(&bodyDef);
				body->SetFixedRotation(rigidBody2D.FixedRotation);
				b2MassData massData;
				body->GetMassData(&massData);
				massData.mass = rigidBody2D.Mass;
				body->SetMassData(&massData);
				body->SetGravityScale(rigidBody2D.GravityScale);
				body->SetLinearDamping(rigidBody2D.LinearDrag);
				body->SetAngularDamping(rigidBody2D.AngularDrag);
				body->SetBullet(rigidBody2D.IsBullet);

				Entity* entityStorage = &mPhysics2DBodyEntityBuffer[physicsBodyEntityBufferIndex++];
				*entityStorage = e;
				body->GetUserData().pointer = reinterpret_cast<uintptr_t>(&entityStorage);
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = mRegistry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& boxCollider2D = mRegistry.get<BoxCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					NR_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2PolygonShape polygonShape;
					polygonShape.SetAsBox(transform.Scale.x * boxCollider2D.Size.x, transform.Scale.y * boxCollider2D.Size.y);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &polygonShape;
					fixtureDef.density = boxCollider2D.Density;
					fixtureDef.friction = boxCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		{
			auto view = mRegistry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& circleCollider2D = mRegistry.get<CircleCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					NR_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2CircleShape circleShape;
					circleShape.m_radius = transform.Scale.x * circleCollider2D.Radius;

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &circleShape;
					fixtureDef.density = circleCollider2D.Density;
					fixtureDef.friction = circleCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		PhysicsManager::ScenePlay();
		GetPhysicsScene()->InitializeScene(this);

		{
			auto view = mRegistry.view<ScriptComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
					ScriptEngine::InstantiateEntityClass(e);
			}

			for (auto entity : view)
			{
				Entity e = { entity, this };
				if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
					ScriptEngine::CreateEntity(e);
			}
		}

		{	//--- Make sure we have an audio listener ---
			//===========================================

			Entity mainCam = GetMainCameraEntity();
			Entity listener;

			auto view = mRegistry.view<AudioListenerComponent>();
			bool listenerFound = !view.empty();

			if (mainCam.mEntityHandle != entt::null)
			{
				if (!mainCam.HasComponent<AudioListenerComponent>())
				{
					mainCam.AddComponent<AudioListenerComponent>();
				}
			}

			if (listenerFound)
			{
				for (auto entity : view)
				{
					listener = { entity, this };
					if (listener.GetComponent<AudioListenerComponent>().Active)
					{
						listenerFound = true;
						break;
					}
					listenerFound = false;
				}
			}

			// If found listener has not been set Active, fallback to using Main Camera
			if (!listenerFound)
			{
				listener = mainCam;
			}

			// Don't update position if we faild to get active listener and Main Camera
			if (listener.mEntityHandle != entt::null)
			{
				// Initialize listener's position
				auto transform = GetAudioTransform(GetWorldSpaceTransform(listener));
				AudioEngine::Get().UpdateListenerPosition(transform);

				auto& listenerComponent = listener.GetComponent<AudioListenerComponent>();
				AudioEngine::Get().UpdateListenerConeAttenuation(listenerComponent.ConeInnerAngleInRadians,
					listenerComponent.ConeOuterAngleInRadians,
					listenerComponent.ConeOuterGain);
			}
		}

		{	//--- Initialize audio component sound positions ---
			//==================================================

			auto view = mRegistry.view<AudioComponent>();

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				auto& audioComponent = view.get<AudioComponent>(entity);

				Entity e = { entity, this };
				auto transform = GetAudioTransform(GetWorldSpaceTransform(e));

				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				updateData.emplace_back(SoundSourceUpdateData{ e.GetID(),
				transform,
				velocity,
				audioComponent.VolumeMultiplier,
				audioComponent.PitchMultiplier });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			//-----------------------------------------------------------------------
			AudioEngine::Get().SubmitSourceUpdateData(updateData);
		}


		mIsPlaying = true;
		mShouldSimulate = true;

		// Now that the scene has initialized, we can start sounds marked to "play on awake"
		// or the sounds that were deserialized in playing state (in the future).
		AudioEngine::RuntimePlaying(mSceneID);
	}

	void Scene::RuntimeStop()
	{
		Input::SetCursorMode(CursorMode::Normal);

		delete[] mPhysics2DBodyEntityBuffer;
		PhysicsManager::SceneStop();
		GetPhysicsScene()->Clear();
		AudioEngine::SetSceneContext(nullptr);
		mIsPlaying = false;
		mShouldSimulate = false;
	}

	void Scene::SimulationStart()
	{
		// Box2D physics
		auto sceneView = mRegistry.view<Box2DWorldComponent>();
		auto& world = mRegistry.get<Box2DWorldComponent>(sceneView.front()).World;

		{
			auto view = mRegistry.view<RigidBody2DComponent>();
			mPhysics2DBodyEntityBuffer = new Entity[view.size()];
			uint32_t physicsBodyEntityBufferIndex = 0;
			for (auto entity : view)
			{
				Entity e = { entity, this };
				UUID entityID = e.GetComponent<IDComponent>().ID;
				TransformComponent& transform = e.GetComponent<TransformComponent>();
				auto& rigidBody2D = mRegistry.get<RigidBody2DComponent>(entity);

				b2BodyDef bodyDef;
				if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static)
					bodyDef.type = b2_staticBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic)
					bodyDef.type = b2_dynamicBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic)
					bodyDef.type = b2_kinematicBody;
				bodyDef.position.Set(transform.Translation.x, transform.Translation.y);

				bodyDef.angle = transform.Rotation.z;

				b2Body* body = world->CreateBody(&bodyDef);
				body->SetFixedRotation(rigidBody2D.FixedRotation);
				Entity* entityStorage = &mPhysics2DBodyEntityBuffer[physicsBodyEntityBufferIndex++];
				*entityStorage = e;
				body->GetUserData().pointer = reinterpret_cast<uintptr_t>(&entityStorage);
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = mRegistry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& boxCollider2D = mRegistry.get<BoxCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					NR_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2PolygonShape polygonShape;
					polygonShape.SetAsBox(transform.Scale.x * boxCollider2D.Size.x, transform.Scale.y * boxCollider2D.Size.y);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &polygonShape;
					fixtureDef.density = boxCollider2D.Density;
					fixtureDef.friction = boxCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		{
			auto view = mRegistry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& circleCollider2D = mRegistry.get<CircleCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					NR_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2CircleShape circleShape;
					circleShape.m_radius = transform.Scale.x * circleCollider2D.Radius;

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &circleShape;
					fixtureDef.density = circleCollider2D.Density;
					fixtureDef.friction = circleCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		PhysicsManager::ScenePlay();
		GetPhysicsScene()->InitializeScene(this);

		mShouldSimulate = true;
	}

	void Scene::SimulationStop()
	{
		Input::SetCursorMode(CursorMode::Normal);

		delete[] mPhysics2DBodyEntityBuffer;
		PhysicsManager::SceneStop();
		GetPhysicsScene()->Clear();

		mShouldSimulate = false;
	}

	void Scene::ScriptComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		auto entityID = registry.get<IDComponent>(entity).ID;
		NR_CORE_ASSERT(mEntityIDMap.find(entityID) != mEntityIDMap.end());
		ScriptEngine::InitScriptEntity(mEntityIDMap.at(entityID));
	}

	void Scene::ScriptComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		if (registry.try_get<IDComponent>(entity))
		{
			auto entityID = registry.get<IDComponent>(entity).ID;
			ScriptEngine::ScriptComponentDestroyed(GetID(), entityID);
		}
	}

	void Scene::AudioComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		auto entityID = registry.get<IDComponent>(entity).ID;
		NR_CORE_ASSERT(mEntityIDMap.find(entityID) != mEntityIDMap.end());
		registry.get<AudioComponent>(entity).ParentHandle = entityID;
		AudioEngine::Get().RegisterAudioComponent(mEntityIDMap.at(entityID));
	}

	void Scene::AudioComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		if (registry.try_get<IDComponent>(entity))
		{
			auto entityID = registry.get<IDComponent>(entity).ID;
			AudioEngine::Get().UnregisterAudioComponent(GetID(), entityID);
		}
	}

	void Scene::MeshColliderComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		NR_PROFILE_FUNC();

		Entity e = { entity, this };
		auto& component = e.GetComponent<MeshColliderComponent>();
		if (e.HasComponent<MeshComponent>())
		{
			auto& mc = e.GetComponent<MeshComponent>();
			component.CollisionMesh = mc.MeshHandle;
			component.SubmeshIndex = mc.SubmeshIndex;
			if (AssetManager::IsAssetHandleValid(component.CollisionMesh) && !PhysicsSystem::GetMeshCache().Exists(component.CollisionMesh))
			{
				CookingFactory::CookMesh(component.CollisionMesh);
			}
		}
		else if (e.HasComponent<StaticMeshComponent>())
		{
			component.CollisionMesh = e.GetComponent<StaticMeshComponent>().StaticMesh;
			if (AssetManager::IsAssetHandleValid(component.CollisionMesh) && !PhysicsSystem::GetMeshCache().Exists(component.CollisionMesh))
			{
				CookingFactory::CookMesh(component.CollisionMesh);
			}
		}
	}

	void Scene::MeshColliderComponentDestroy(entt::registry& registry, entt::entity entity)
	{
	}


	void Scene::UpdateAnimation(float dt, bool isRuntime)
	{
		auto view = mRegistry.view<AnimationComponent>();
		for (auto entity : view)
		{
			Entity e = { entity, this };

			auto& anim = e.GetComponent<AnimationComponent>();
			if (AssetManager::IsAssetHandleValid(anim.AnimationController))
			{
				auto animationController = AssetManager::GetAsset<AnimationController>(anim.AnimationController);

				animationController->SetAnimationPlaying(anim.EnableAnimation);
				if (!anim.EnableAnimation)
				{
					animationController->SetAnimationTime(anim.AnimationTime);
				}

				auto& rootMotion = animationController->Update(dt, anim.EnableRootMotion);
				anim.AnimationTime = animationController->GetAnimationTime();

				if (/*isRuntime &&*/ anim.EnableRootMotion)
				{
					auto rootMotionEntity = FindEntityByID(anim.RootMotionTarget);
					if (rootMotionEntity)
					{
						auto parentEntity = rootMotionEntity.GetParent();
						glm::mat3 modelToWorld = GetWorldSpaceTransformMatrix(e); // glm::mat3 because don't need translation here
						glm::mat3 worldToTarget = parentEntity ? glm::inverse(glm::mat3{ GetWorldSpaceTransformMatrix(parentEntity) }) : glm::mat3(1.0f);

						// Figure out how to apply the root motion, depending on what components the target entity has
						auto& transform = rootMotionEntity.Transform();
						if (mShouldSimulate && rootMotionEntity.HasComponent<CharacterControllerComponent>())
						{
							// 1. target entity is a physics character controller
							//    => apply root motion to character pose
							Ref<PhysicsController> controller = GetPhysicsScene()->GetController(rootMotionEntity);
							NR_CORE_ASSERT(controller);
							{
								// note: translation is applied to the physics controller
								//       rotation (which cannot be applied to the physics controller) is applied directly to the target entity's transform
								glm::vec3 displacement = worldToTarget * modelToWorld * rootMotion.Translation;
								controller->Move(displacement);
								transform.Rotation = glm::eulerAngles(glm::quat(transform.Rotation) * rootMotion.Rotation);
							}
						}
						else if (mShouldSimulate && rootMotionEntity.HasComponent<RigidBodyComponent>())
						{
							// 2. target entity is a physics rigid body.
							//    => apply root motion as kinematic target  (or do nothing if it isnt a kinematic rigidbody. We do not support attempting to convert root motion into physics impulses)
							//
							const Ref<PhysicsActor>& actor = GetPhysicsScene()->GetActor(rootMotionEntity);
							NR_CORE_ASSERT(actor);
							if (actor->IsKinematic())
							{
								glm::vec3 position = transform.Translation + worldToTarget * modelToWorld * rootMotion.Translation;
								glm::vec3 rotation = glm::eulerAngles(glm::quat(transform.Rotation) * rootMotion.Rotation);
								actor->SetKinematicTarget(position, rotation);
							}
						}
						else
						{
							// 3. either we aren't simulating physics, or the target entity is not a physics body
							//    => apply root motion directly to the target entity's transform
							transform.Translation += worldToTarget * modelToWorld * rootMotion.Translation;
							transform.Rotation = glm::eulerAngles(glm::quat(transform.Rotation) * rootMotion.Rotation);
						}
					}
				}
			}
		}
	}

	void Scene::SetViewportSize(uint32_t width, uint32_t height)
	{
		mViewportWidth = width;
		mViewportHeight = height;
	}

	void Scene::SetSkybox(const Ref<TextureCube>& skybox)
	{
		//mSkyboxTexture = skybox;
		//mSkyboxMaterial->Set("uTexture", skybox);
	}

	Entity Scene::GetMainCameraEntity()
	{
		NR_PROFILE_FUNC();

		auto view = mRegistry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& comp = view.get<CameraComponent>(entity);
			if (comp.Primary)
			{
				NR_CORE_ASSERT(comp.CameraObj.GetOrthographicSize() || comp.CameraObj.GetPerspectiveVerticalFOV(), "Camera is not fully initialized");
				return { entity, this };
			}
		}
		return {};
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateChildEntity({}, name);
	}

	Entity Scene::CreateChildEntity(Entity parent, const std::string& name)
	{
		NR_PROFILE_FUNC();

		auto entity = Entity{ mRegistry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = {};

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		if (parent)
			entity.SetParent(parent);

		mEntityIDMap[idComponent.ID] = entity;
		return entity;
	}

	Entity Scene::CreateEntityWithID(UUID uuid, const std::string& name, bool runtimeMap)
	{
		NR_PROFILE_FUNC();

		auto entity = Entity{ mRegistry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = uuid;

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		NR_CORE_ASSERT(mEntityIDMap.find(uuid) == mEntityIDMap.end());
		mEntityIDMap[uuid] = entity;
		return entity;
	}

	void Scene::SubmitToDestroyEntity(Entity entity)
	{
		SubmitPostUpdateFunc([entity]() { entity.mScene->DestroyEntity(entity); });
	}

	void Scene::DestroyEntity(Entity entity, bool excludeChildren, bool first)
	{
		NR_PROFILE_FUNC();

		auto physicsWorld = GetPhysicsScene();

		if (entity.HasComponent<ScriptComponent>())
			ScriptEngine::ScriptComponentDestroyed(mSceneID, entity.GetID());

		if (entity.HasComponent<AudioComponent>())
			AudioEngine::Get().UnregisterAudioComponent(mSceneID, entity.GetID());

		if (!mIsEditorScene)
		{
			if (entity.HasComponent<RigidBodyComponent>())
				physicsWorld->RemoveActor(physicsWorld->GetActor(entity));

			if (entity.HasComponent<RigidBody2DComponent>())
			{
				auto& world = mRegistry.get<Box2DWorldComponent>(mSceneEntity).World;
				b2Body* body = (b2Body*)entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
				world->DestroyBody(body);
			}

		}

		if (!excludeChildren)
		{
			// NOTE(Yan): don't make this a foreach loop because entt will move the children
			//            vector in memory as entities/components get deleted
			for (size_t i = 0; i < entity.Children().size(); i++)
			{
				auto childId = entity.Children()[i];
				Entity child = FindEntityByID(childId);
				if (child)
					DestroyEntity(child, excludeChildren, false);
			}
		}

		if (first)
		{
			if (entity.HasParent())
				entity.GetParent().RemoveChild(entity);
		}

		mRegistry.destroy(entity.mEntityHandle);
	}

	void Scene::ResetTransformsToMesh(Entity entity, bool resetChildren)
	{
		if (entity.HasComponent<MeshComponent>())
		{
			auto& mc = entity.GetComponent<MeshComponent>();
			if (AssetManager::IsAssetHandleValid(mc.MeshHandle))
			{
				Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(mc.MeshHandle);
				const auto& submeshes = mesh->GetMeshSource()->GetSubmeshes();
				if (submeshes.size() > mc.SubmeshIndex)
				{
					entity.Transform().SetTransform(submeshes[mc.SubmeshIndex].LocalTransform);
				}
			}
		}

		if (resetChildren)
		{
			for (UUID childID : entity.Children())
			{
				Entity child = FindEntityByID(childID);
				ResetTransformsToMesh(child, resetChildren);
			}

		}

	}

	template<typename T>
	static void CopyComponent(entt::registry& dstRegistry, entt::registry& srcRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto components = srcRegistry.view<T>();
		for (auto srcEntity : components)
		{
			entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

			auto& srcComponent = srcRegistry.get<T>(srcEntity);
			auto& destComponent = dstRegistry.emplace_or_replace<T>(destEntity, srcComponent);
		}
	}

	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
	{
		NR_PROFILE_FUNC();

		if (registry.try_get<T>(src))
		{
			auto& srcComponent = registry.get<T>(src);
			registry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::registry& dstRegistry, entt::entity src, entt::registry& srcRegistry)
	{
		NR_PROFILE_FUNC();

		if (srcRegistry.try_get<T>(src))
		{
			auto& srcComponent = srcRegistry.get<T>(src);
			dstRegistry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		NR_PROFILE_FUNC();

		auto parentNewEntity = [&entity, scene = this](Entity newEntity)
			{
				if (entity.HasParent())
				{
					Entity parent = scene->FindEntityByID(entity.GetParentID());
					NR_CORE_ASSERT(parent, "Failed to find parent entity");
					newEntity.SetParentID(entity.GetParentID());
					parent.Children().push_back(newEntity.GetID());
				}
			};

		if (entity.HasComponent<PrefabComponent>())
		{
			auto prefabID = entity.GetComponent<PrefabComponent>().PrefabID;
			NR_CORE_VERIFY(AssetManager::IsAssetHandleValid(prefabID));
			const auto& entityTransform = entity.GetComponent<TransformComponent>();
			Entity prefabInstance = Instantiate(AssetManager::GetAsset<Prefab>(prefabID), &entityTransform.Translation, &entityTransform.Rotation, &entityTransform.Scale);
			parentNewEntity(prefabInstance);
			return prefabInstance;
		}

		Entity newEntity;
		if (entity.HasComponent<TagComponent>())
			newEntity = CreateEntity(entity.GetComponent<TagComponent>().Tag);
		else
			newEntity = CreateEntity();

		CopyComponentIfExists<TransformComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		// NOTE(Peter): We can't use this method for copying the RelationshipComponent since we
		//				need to duplicate the entire child hierarchy and basically reconstruct the entire RelationshipComponent from the ground up
		//CopyComponentIfExists<RelationshipComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<MeshComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<StaticMeshComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<AnimationComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<ParticleComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<ScriptComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<CameraComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<TextComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<RigidBodyComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<CharacterControllerComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<FixedJointComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<BoxColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<SphereColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<CapsuleColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<MeshColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<PointLightComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<SkyLightComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<AudioComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
		CopyComponentIfExists<AudioListenerComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);

#if _DEBUG && 0
		// Check that nothing has been forgotten...
		bool foundAll = true;
		mRegistry.visit(entity, [&](entt::id_type type) {
			if (type != entt::type_index<RelationshipComponent>().value())
			{
				bool founde = false;
				mRegistry.visit(newEntity, [type, &founde](entt::id_type newType) {if (newType == type) founde = true; });
				foundAll = foundAll && founde;
			}
			});
		NR_CORE_ASSERT(foundAll, "At least one component was not duplicated - have you added a new component type and not dealt with it here?");
#endif

		auto childIds = entity.Children(); // need to take a copy of children here, because the collection is mutated below
		for (auto childId : childIds)
		{
			Entity childEntity = FindEntityByID(childId);
			NR_CORE_ASSERT(childEntity, "Failed to find child entity"); // Check your scene file.  It may be corrupted from earlier (buggy) attempts at entity duplication
			if (childEntity)
			{
				Entity childDuplicate = DuplicateEntity(FindEntityByID(childId));

				// At this point childDuplicate is a child of entity, we need to remove it from that entity
				UnparentEntity(childDuplicate, false);

				childDuplicate.SetParentID(newEntity.GetID());
				newEntity.Children().push_back(childDuplicate.GetID());
			}
		}

		parentNewEntity(newEntity);

		return newEntity;
	}

	Entity Scene::CreatePrefabEntity(Entity entity, Entity parent, const glm::vec3* translation, const glm::vec3* rotation, const glm::vec3* scale)
	{
		NR_PROFILE_FUNC();

		NR_CORE_VERIFY(entity.HasComponent<PrefabComponent>());

		Entity newEntity = CreateEntity();
		if (parent)
		{
			newEntity.SetParent(parent);
		}

		CopyComponentIfExists<TagComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<PrefabComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<TransformComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<MeshComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<StaticMeshComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<AnimationComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<ParticleComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<PointLightComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<SkyLightComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<ScriptComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CameraComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<TextComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<RigidBodyComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CharacterControllerComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<FixedJointComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<BoxColliderComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<SphereColliderComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CapsuleColliderComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<MeshColliderComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<AudioComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<AudioListenerComponent>(newEntity, mRegistry, entity, entity.mScene->mRegistry);

		if (translation)
		{
			newEntity.Transform().Translation = *translation;
		}
		if (rotation)
		{
			newEntity.Transform().Rotation = *rotation;
		}
		if (scale)
		{
			newEntity.Transform().Scale = *scale;
		}

		// This debug checking is not guaranteed to work - because the same type could have different type_id in different registries.
#if _DEBUG && 0
		// Check that nothing has been forgotten...
		bool foundAll = true;
		entity.mScene->mRegistry.visit(entity, [&](entt::id_type type) {
			if (type != entt::type_index<RelationshipComponent>().value())
			{
				bool founde = false;
				mRegistry.visit(newEntity, [type, &founde](entt::id_type newType) {if (newType == type) founde = true; });
				foundAll = foundAll && founde;
			}
			});
		NR_CORE_ASSERT(foundAll, "At least one component was not duplicated - have you added a new component type and not dealt with it here?");
#endif

		// Create children
		for (auto childId : entity.Children())
		{
			CreatePrefabEntity(entity.mScene->FindEntityByID(childId), newEntity);
		}

		if (!mIsEditorScene)
		{
			if (newEntity.HasComponent<RigidBodyComponent>())
			{
				GetPhysicsScene()->CreateActor(newEntity);
			}
			else if (newEntity.HasAny<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>())
			{
				auto& rigidbody = newEntity.AddComponent<RigidBodyComponent>();
				rigidbody.BodyType = RigidBodyComponent::Type::Static;
				GetPhysicsScene()->CreateActor(newEntity);
			}

			if (newEntity.HasComponent<ScriptComponent>())
			{
				if (ScriptEngine::ModuleExists(newEntity.GetComponent<ScriptComponent>().ModuleName))
				{
					ScriptEngine::InstantiateEntityClass(newEntity);
					ScriptEngine::CreateEntity(newEntity);
				}
			}

		}
		return newEntity;
	}

	Entity Scene::Instantiate(Ref<Prefab> prefab, const glm::vec3* translation, const glm::vec3* rotation, const glm::vec3* scale)
	{
		NR_PROFILE_FUNC();

		Entity result;

		auto entities = prefab->mScene->GetAllEntitiesWith<RelationshipComponent>();
		for (auto e : entities)
		{
			Entity entity = { e, prefab->mScene.Raw() };
			if (!entity.HasParent())
			{
				result = CreatePrefabEntity(entity, {}, translation, rotation, scale);
				break;
			}
		}

		return result;
	}

	void Scene::BuildMeshEntityHierarchy(Entity parent, Ref<Mesh> mesh, const void* assimpScene, void* assimpNode)
	{
		aiScene* aScene = (aiScene*)assimpScene;
		aiNode* node = (aiNode*)assimpNode;

		// Skip empty root node
		if (node == aScene->mRootNode && node->mNumMeshes == 0)
		{
			for (uint32_t i = 0; i < node->mNumChildren; i++)
			{
				BuildMeshEntityHierarchy(parent, mesh, assimpScene, node->mChildren[i]);
			}

			return;
		}

		glm::mat4 transform = Utils::Mat4FromAIMatrix4x4(node->mTransformation);
		auto nodeName = node->mName.C_Str();

		Entity nodeEntity = CreateChildEntity(parent, nodeName);
		nodeEntity.Transform().SetTransform(transform);

		if (node->mNumMeshes == 1)
		{
			// Node == Mesh in this case
			uint32_t submeshIndex = node->mMeshes[0];
			nodeEntity.AddComponent<MeshComponent>(mesh->Handle, submeshIndex);

			// TODO(Yan): import settings should determine this
			nodeEntity.AddComponent<MeshColliderComponent>(mesh->Handle, submeshIndex);
			nodeEntity.AddComponent<RigidBodyComponent>();
		}
		else if (node->mNumMeshes > 1)
		{
			// Create one entity per child mesh, parented under node
			for (uint32_t i = 0; i < node->mNumMeshes; i++)
			{
				uint32_t submeshIndex = node->mMeshes[i];
				auto meshName = aScene->mMeshes[submeshIndex]->mName.C_Str();

				Entity childEntity = CreateChildEntity(nodeEntity, meshName);

				childEntity.AddComponent<MeshComponent>(mesh->Handle, submeshIndex);

				// TODO(Yan): import settings should determine this
				childEntity.AddComponent<MeshColliderComponent>(mesh->Handle, submeshIndex);
				childEntity.AddComponent<RigidBodyComponent>();
			}

		}

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			BuildMeshEntityHierarchy(nodeEntity, mesh, assimpScene, node->mChildren[i]);
		}
	}

	Entity Scene::InstantiateMesh(Ref<Mesh> mesh)
	{
		auto& assetData = AssetManager::GetMetadata(mesh->Handle);
		Entity rootEntity = CreateEntity(assetData.FilePath.stem().string());
		auto aScene = mesh->GetMeshSource()->mScene;
		BuildMeshEntityHierarchy(rootEntity, mesh, aScene, aScene->mRootNode);
		return rootEntity;
	}

	Entity Scene::FindEntityByTag(const std::string& tag)
	{
		NR_PROFILE_FUNC();

		// TODO: If this becomes used often, consider indexing by tag
		auto view = mRegistry.view<TagComponent>();
		for (auto entity : view)
		{
			const auto& canditate = view.get<TagComponent>(entity).Tag;
			if (canditate == tag)
			{
				return Entity(entity, this);
			}
		}

		return Entity{};
	}

	Entity Scene::FindEntityByID(UUID id)
	{
		NR_PROFILE_FUNC();

		auto view = mRegistry.view<IDComponent>();
		for (auto entity : view)
		{
			auto& idComponent = mRegistry.get<IDComponent>(entity);
			if (idComponent.ID == id)
				return Entity(entity, this);
		}

		return Entity{};
	}

	void Scene::ConvertToLocalSpace(Entity entity)
	{
		NR_PROFILE_FUNC();

		Entity parent = FindEntityByID(entity.GetParentID());

		if (!parent)
		{
			return;
		}

		auto& transform = entity.Transform();
		glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);
		glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
		Math::DecomposeTransform(localTransform, transform.Translation, transform.Rotation, transform.Scale);
	}

	void Scene::ConvertToWorldSpace(Entity entity)
	{
		NR_PROFILE_FUNC();

		Entity parent = FindEntityByID(entity.GetParentID());

		if (!parent)
		{
			return;
		}

		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		auto& entityTransform = entity.Transform();
		Math::DecomposeTransform(transform, entityTransform.Translation, entityTransform.Rotation, entityTransform.Scale);
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		NR_PROFILE_FUNC();

		glm::mat4 transform(1.0f);

		Entity parent = FindEntityByID(entity.GetParentID());
		if (parent)
		{
			transform = GetWorldSpaceTransformMatrix(parent);
		}

		return transform * entity.Transform().GetTransform();
	}

	// TODO: Definitely cache this at some point
	TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
	{
		NR_PROFILE_FUNC();

		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transformComponent;
		Math::DecomposeTransform(transform, transformComponent.Translation, transformComponent.Rotation, transformComponent.Scale);

		return transformComponent;
	}

	void Scene::ParentEntity(Entity entity, Entity parent)
	{
		NR_PROFILE_FUNC();

		if (parent.IsDescendantOf(entity))
		{
			UnparentEntity(parent);

			Entity newParent = FindEntityByID(entity.GetParentID());
			if (newParent)
			{
				UnparentEntity(entity);
				ParentEntity(parent, newParent);
			}
		}
		else
		{
			Entity previousParent = FindEntityByID(entity.GetParentID());

			if (previousParent)
			{
				UnparentEntity(entity);
			}
		}

		entity.SetParentID(parent.GetID());
		parent.Children().push_back(entity.GetID());

		ConvertToLocalSpace(entity);
	}

	void Scene::UnparentEntity(Entity entity, bool convertToWorldSpace)
	{
		NR_PROFILE_FUNC();

		Entity parent = FindEntityByID(entity.GetParentID());
		if (!parent)
		{
			return;
		}

		auto& parentChildren = parent.Children();
		parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), entity.GetID()), parentChildren.end());

		if (convertToWorldSpace)
		{
			ConvertToWorldSpace(entity);
		}

		entity.SetParentID(0);
	}

	// Copy to runtime
	void Scene::CopyTo(Ref<Scene>& target)
	{
		NR_PROFILE_FUNC();

		// Environment
		target->mLight = mLight;
		target->mLightMultiplier = mLightMultiplier;

		target->mEnvironment = mEnvironment;
		target->mSkyboxTexture = mSkyboxTexture;
		target->mSkyboxMaterial = mSkyboxMaterial;
		target->mSkyboxLod = mSkyboxLod;

		std::unordered_map<UUID, entt::entity> enttMap;
		auto idComponents = mRegistry.view<IDComponent>();
		for (auto entity : idComponents)
		{
			auto uuid = mRegistry.get<IDComponent>(entity).ID;
			auto name = mRegistry.get<TagComponent>(entity).Tag;
			Entity e = target->CreateEntityWithID(uuid, name, true);
			enttMap[uuid] = e.mEntityHandle;
		}

		CopyComponent<PrefabComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<TagComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<TransformComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<RelationshipComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<MeshComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<StaticMeshComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<AnimationComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<ParticleComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<DirectionalLightComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<PointLightComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<SkyLightComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<ScriptComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<CameraComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<SpriteRendererComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<TextComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<RigidBody2DComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<BoxCollider2DComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<CircleCollider2DComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<RigidBodyComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<CharacterControllerComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<FixedJointComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<BoxColliderComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<SphereColliderComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<CapsuleColliderComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<MeshColliderComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<AudioComponent>(target->mRegistry, mRegistry, enttMap);
		CopyComponent<AudioListenerComponent>(target->mRegistry, mRegistry, enttMap);

		const auto& entityInstanceMap = ScriptEngine::GetEntityInstanceMap();
		if (entityInstanceMap.find(target->GetID()) != entityInstanceMap.end())
		{
			ScriptEngine::CopyEntityScriptData(target->GetID(), mSceneID);
		}

		target->SetPhysics2DGravity(GetPhysics2DGravity());

		// Sort IdComponent by by entity handle (which is essentially the order in which they were created)
		// This ensures a consistent ordering when iterating IdComponent (for example: when rendering scene hierarchy panel)
		target->mRegistry.sort<IDComponent>([&target](const auto lhs, const auto rhs)
			{
				auto lhsEntity = target->GetEntityMap().find(lhs.ID);
				auto rhsEntity = target->GetEntityMap().find(rhs.ID);
				return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
			});

	}

	Ref<Scene> Scene::GetScene(UUID uuid)
	{
		if (sActiveScenes.find(uuid) != sActiveScenes.end())
		{
			return sActiveScenes.at(uuid);
		}

		return {};
	}

	float Scene::GetPhysics2DGravity() const
	{
		return mRegistry.get<Box2DWorldComponent>(mSceneEntity).World->GetGravity().y;
	}

	void Scene::SetPhysics2DGravity(float gravity)
	{
		mRegistry.get<Box2DWorldComponent>(mSceneEntity).World->SetGravity({ 0.0f, gravity });
	}

	Ref<PhysicsScene> Scene::GetPhysicsScene() const
	{
		NR_CORE_ASSERT(mRegistry.try_get<PhysicsSceneComponent>(mSceneEntity));
		return mRegistry.get<PhysicsSceneComponent>(mSceneEntity).World;
	}

	void Scene::SceneTransition(const std::string& scene)
	{
		if (mSceneTransitionCallback)
		{
			mSceneTransitionCallback(scene);
		}

		// Debug
		if (!mSceneTransitionCallback)
		{
			NR_CORE_WARN("Cannot transition scene - no callback set!");
		}
	}

	Ref<Scene> Scene::CreateEmpty()
	{
		return new Scene("Empty", false, false);
	}
}