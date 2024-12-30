#include "nrpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Core/Application.h"
#include "NotRed/Scene/Prefab.h"
#include "NotRed/Math/Math.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsActor.h"
#include "NotRed/Physics/PhysicsLayer.h"
#include "NotRed/Physics/CookingFactory.h"
#include "NotRed/Renderer/MeshFactory.h"

#include "NotRed/Asset/AssetManager.h"

#include "NotRed/ImGui/ImGui.h"
#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/AudioComponent.h"

namespace NR
{
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
		: mContext(context)
	{
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		mContext = scene;
		mSelectionContext = {};
		if (mSelectionContext && false)
		{
			auto& entityMap = mContext->GetEntityMap();
			UUID selectedEntityID = mSelectionContext.GetID();
			if (entityMap.find(selectedEntityID) != entityMap.end())
			{
				mSelectionContext = entityMap.at(selectedEntityID);
			}
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		mSelectionContext = entity;

		if (mSelectionChangedCallback)
		{
			mSelectionChangedCallback(mSelectionContext);
		}
	}

	void SceneHierarchyPanel::ImGuiRender(bool window)
	{
		if (window)
		{
			ImGui::Begin("Scene Hierarchy");
		}

		ImRect windowRect = { ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax() };

		if (mContext)
		{
			for (auto entity : mContext->mRegistry.view<IDComponent, RelationshipComponent>())
			{
				Entity e(entity, mContext.Raw());
				if (e.GetParentID() == 0)
				{
					DrawEntityNode({ entity, mContext.Raw() });
				}
			}

			if (ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetCurrentWindow()->ID))
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

				if (payload)
				{
					Entity& entity = *(Entity*)payload->Data;
					mContext->UnparentEntity(entity);
				}

				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextWindow(0, 1))
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Entity"))
					{
						auto newEntity = mContext->CreateEntity("Entity");
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Camera"))
					{
						auto newEntity = mContext->CreateEntity("Camera");
						newEntity.AddComponent<CameraComponent>();
						SetSelected(newEntity);
					}
					if (ImGui::BeginMenu("3D"))
					{
						if (ImGui::MenuItem("Cube"))
						{
							auto newEntity = mContext->CreateEntity("Cube");
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Cube.nrmesh");

							newEntity.AddComponent<MeshComponent>(mesh);
							auto& bcc = newEntity.AddComponent<BoxColliderComponent>();
							bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);

							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Sphere"))
						{
							auto newEntity = mContext->CreateEntity("Sphere");
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Sphere.nrmesh");

							newEntity.AddComponent<MeshComponent>(mesh);
							auto& scc = newEntity.AddComponent<SphereColliderComponent>();
							scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);

							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Capsule"))
						{
							auto newEntity = mContext->CreateEntity("Capsule");
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Capsule.nrmesh");

							newEntity.AddComponent<MeshComponent>(mesh);
							auto& ccc = newEntity.AddComponent<CapsuleColliderComponent>();
							ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);

							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Cylinder"))
						{
							auto newEntity = mContext->CreateEntity("Cylinder");
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Cylinder.nrmesh");

							newEntity.AddComponent<MeshComponent>(mesh);
							auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
							CookingFactory::CookMesh(collider);

							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Torus"))
						{
							auto newEntity = mContext->CreateEntity("Torus");
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Torus.nrmesh");

							newEntity.AddComponent<MeshComponent>(mesh);
							auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
							CookingFactory::CookMesh(collider);

							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Plane"))
						{
							auto newEntity = mContext->CreateEntity("Plane");
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Plane.nrmesh");

							newEntity.AddComponent<MeshComponent>(mesh);
							auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
							CookingFactory::CookMesh(collider);

							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Cone"))
						{
							auto newEntity = mContext->CreateEntity("Cone");
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Cone.fbx");

							newEntity.AddComponent<MeshComponent>(mesh);
							auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
							CookingFactory::CookMesh(collider);

							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Particles"))
						{
							auto newEntity = mContext->CreateEntity("Particles");
							newEntity.AddComponent<ParticleComponent>();
							SetSelected(newEntity);
						}
						ImGui::EndMenu();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Directional Light"))
					{
						auto newEntity = mContext->CreateEntity("Directional Light");
						newEntity.AddComponent<DirectionalLightComponent>();
						newEntity.GetComponent<TransformComponent>().GetTransform() = glm::toMat4(glm::quat(glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f })));
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Point Light"))
					{
						auto newEntity = mContext->CreateEntity("Point Light");
						newEntity.AddComponent<PointLightComponent>();
						newEntity.GetComponent<TransformComponent>().Translation = glm::vec3{ 0 };
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Sky Light"))
					{
						auto newEntity = mContext->CreateEntity("Sky Light");
						newEntity.AddComponent<SkyLightComponent>();
						SetSelected(newEntity);
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::End();

			ImGui::Begin("Properties");

			if (mSelectionContext)
			{
				DrawComponents(mSelectionContext);
			}
		}

		if (window)
		{
			ImGui::End();
		}
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const char* name = "Entity";
		name = entity.GetComponent<TagComponent>().Tag.c_str();

		ImGuiTreeNodeFlags flags = (entity == mSelectionContext ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		if (entity.Children().empty())
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		const bool missingMesh = entity.HasComponent<MeshComponent>() && (entity.GetComponent<MeshComponent>().MeshObj && entity.GetComponent<MeshComponent>().MeshObj->IsFlagSet(AssetFlag::Missing));
		if (missingMesh)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.3f, 1.0f));
		}

		bool isPrefab = entity.HasComponent<PrefabComponent>();
		if (isPrefab)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.32f, 0.7f, 0.87f, 1.0f));
		}

		const bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, name);

		if (isPrefab)
		{
			ImGui::PopStyleColor();
		}

		if (ImGui::IsItemClicked())
		{
			mSelectionContext = entity;
			if (mSelectionChangedCallback)
			{
				mSelectionChangedCallback(mSelectionContext);
			}
		}

		if (missingMesh)
		{
			ImGui::PopStyleColor();
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				entityDeleted = true;
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::Text(entity.GetComponent<TagComponent>().Tag.c_str());
			ImGui::SetDragDropPayload("scene_entity_hierarchy", &entity, sizeof(Entity));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

			if (payload)
			{
				Entity& droppedEntity = *(Entity*)payload->Data;
				mContext->ParentEntity(droppedEntity, entity);
			}

			ImGui::EndDragDropTarget();
		}

		if (opened)
		{
			for (auto child : entity.Children())
			{
				Entity e = mContext->FindEntityByID(child);
				if (e)
				{
					DrawEntityNode(e);
				}
			}
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			mContext->DestroyEntity(entity);
			if (entity == mSelectionContext)
			{
				mSelectionContext = {};
			}
			mEntityDeletedCallback(entity);
		}
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, bool canBeRemoved = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			ImGui::PushID((void*)typeid(T).hash_code());
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
			ImGui::PopStyleVar();

			bool resetValues = false;
			bool removeComponent = false;

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }) || rightClicked)
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Reset"))
				{
					resetValues = true;
				}

				if (canBeRemoved)
				{
					if (ImGui::MenuItem("Remove component"))
					{
						removeComponent = true;
					}
				}

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent || resetValues)
			{
				entity.RemoveComponent<T>();
			}

			if (resetValues)
			{
				entity.AddComponent<T>();
			}

			ImGui::PopID();
		}
	}

	static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		bool modified = false;

		const ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			values.x = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		return modified;
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		ImGui::AlignTextToFramePadding();

		auto id = entity.GetComponent<IDComponent>().ID;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, 256);
			memcpy(buffer, tag.c_str(), tag.length());
			ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);
			if (ImGui::InputText("##Tag", buffer, 256))
			{
				tag = std::string(buffer);
			}
			ImGui::PopItemWidth();
		}

		ImGui::SameLine();
		ImGui::TextDisabled("%llx", id);

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 textSize = ImGui::CalcTextSize("Add Component");
		ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.y));

		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("AddComponentPanel");
		}

		if (ImGui::BeginPopup("AddComponentPanel"))
		{
			if (!mSelectionContext.HasComponent<CameraComponent>())
			{
				if (ImGui::Button("Camera"))
				{
					mSelectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<MeshComponent>())
			{
				if (ImGui::Button("Mesh"))
				{
					mSelectionContext.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<ParticleComponent>())
			{
				if (ImGui::Button("Particle"))
				{
					mSelectionContext.AddComponent<ParticleComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<DirectionalLightComponent>())
			{
				if (ImGui::Button("Directional Light"))
				{
					mSelectionContext.AddComponent<DirectionalLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<PointLightComponent>())
			{
				if (ImGui::Button("Point Light"))
				{
					mSelectionContext.AddComponent<PointLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<SkyLightComponent>())
			{
				if (ImGui::Button("Sky Light"))
				{
					mSelectionContext.AddComponent<SkyLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<ScriptComponent>())
			{
				if (ImGui::Button("Script"))
				{
					mSelectionContext.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<SpriteRendererComponent>())
			{
				if (ImGui::Button("Sprite Renderer"))
				{
					mSelectionContext.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<RigidBody2DComponent>())
			{
				if (ImGui::Button("Rigidbody 2D"))
				{
					mSelectionContext.AddComponent<RigidBody2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<BoxCollider2DComponent>())
			{
				if (ImGui::Button("Box Collider 2D"))
				{
					mSelectionContext.AddComponent<BoxCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<CircleCollider2DComponent>())
			{
				if (ImGui::Button("Circle Collider 2D"))
				{
					mSelectionContext.AddComponent<CircleCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<RigidBodyComponent>())
			{
				if (ImGui::Button("Rigidbody"))
				{
					mSelectionContext.AddComponent<RigidBodyComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<BoxColliderComponent>())
			{
				if (ImGui::Button("Box Collider"))
				{
					auto& bcc = mSelectionContext.AddComponent<BoxColliderComponent>();
					bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<SphereColliderComponent>())
			{
				if (ImGui::Button("Sphere Collider"))
				{
					auto& scc = mSelectionContext.AddComponent<SphereColliderComponent>();
					scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<SphereColliderComponent>())
			{
				if (ImGui::Button("Capsule Collider"))
				{
					auto& ccc = mSelectionContext.AddComponent<CapsuleColliderComponent>();
					ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<MeshColliderComponent>())
			{
				if (ImGui::Button("Mesh Collider"))
				{
					MeshColliderComponent& component = mSelectionContext.AddComponent<MeshColliderComponent>();
					if (mSelectionContext.HasComponent<MeshComponent>())
					{
						component.CollisionMesh = mSelectionContext.GetComponent<MeshComponent>().MeshObj;
						CookingFactory::CookMesh(component);
					}

					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<Audio::AudioComponent>())
			{
				if (ImGui::Button("Audio"))
				{
					mSelectionContext.AddComponent<Audio::AudioComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!mSelectionContext.HasComponent<AudioListenerComponent>())
			{
				if (ImGui::Button("Audio Listener"))
				{
					auto view = mContext->GetAllEntitiesWith<AudioListenerComponent>();
					bool listenerExists = !view.empty();
					auto& listenerComponent = mSelectionContext.AddComponent<AudioListenerComponent>();

					listenerComponent.Active = !listenerExists;
					Audio::AudioEngine::Get().RegisterNewListener(listenerComponent);
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
			{
				DrawVec3Control("Translation", component.Translation);
				//glm::vec3 rotation = glm::degrees(component.Rotation);
				DrawVec3Control("Rotation", component.Rotation);
				//component.Rotation = glm::radians(rotation);
				DrawVec3Control("Scale", component.Scale, 1.0f);
			}, false);

		DrawComponent<MeshComponent>("Mesh", entity, [&](MeshComponent& mc)
			{
				UI::BeginPropertyGrid();

				UI::PropertyAssetReferenceError error;

				if (UI::PropertyAssetReferenceWithConversion<Mesh, MeshAsset>("Mesh", mc.MeshObj,
					[=](Ref<MeshAsset> meshAsset)
					{
						if (mMeshAssetConvertCallback)
						{
							mMeshAssetConvertCallback(entity, meshAsset);
						}
					}, &error))
				{
					if (entity.HasComponent<MeshColliderComponent>())
					{
						auto& mcc = entity.GetComponent<MeshColliderComponent>();
						mcc.CollisionMesh = mc.MeshObj;
						CookingFactory::CookMesh(mcc, true);
					}
				}

				if (error == UI::PropertyAssetReferenceError::InvalidMetadata)
				{
					if (mInvalidMetadataCallback)
					{
						mInvalidMetadataCallback(entity, UI::sPropertyAssetReferenceAssetHandle);
					}
				}
				UI::EndPropertyGrid();

				if (mc.MeshObj && mc.MeshObj->IsValid())
				{
					if (UI::BeginTreeNode("Materials"))
					{
						UI::BeginPropertyGrid();
						auto meshMaterialTable = mc.MeshObj->GetMaterials();
						if (mc.Materials->GetMaterialCount() < meshMaterialTable->GetMaterialCount())
						{
							mc.Materials->SetMaterialCount(meshMaterialTable->GetMaterialCount());
						}

						for (size_t i = 0; i < mc.Materials->GetMaterialCount(); ++i)
						{
							if (i == meshMaterialTable->GetMaterialCount())
							{
								ImGui::Separator();
							}

							bool hasLocalMaterial = mc.Materials->HasMaterial(i);
							bool hasMeshMaterial = meshMaterialTable->HasMaterial(i);
							Ref<MaterialAsset> meshMaterialAsset;
							if (hasMeshMaterial)
							{
								meshMaterialAsset = meshMaterialTable->GetMaterial(i);
							}

							Ref<MaterialAsset> materialAsset = hasLocalMaterial ? mc.Materials->GetMaterial(i) : meshMaterialAsset;
							std::string label = fmt::format("[Material {0}]", i);
							UI::PropertyAssetReferenceSettings settings;
							if (hasLocalMaterial || !hasMeshMaterial)
							{
								if (hasLocalMaterial)
								{
									settings.AdvanceToNextColumn = false;
									settings.WidthOffset = ImGui::GetStyle().ItemSpacing.x + 28.0f;
								}

								UI::PropertyAssetReferenceTarget<MaterialAsset>(label.c_str(), nullptr, materialAsset, [i, materialTable = mc.Materials](Ref<MaterialAsset> materialAsset) mutable
									{
										materialTable->SetMaterial(i, materialAsset);
									}, settings);
							}
							else
							{
								std::string meshMaterialName = meshMaterialAsset->GetMaterial()->GetName();
								if (meshMaterialName.empty())
								{
									meshMaterialName = "Unnamed Material";
								}

								UI::PropertyAssetReferenceTarget<MaterialAsset>(label.c_str(), meshMaterialName.c_str(), materialAsset, [i, materialTable = mc.Materials](Ref<MaterialAsset> materialAsset) mutable
									{
										materialTable->SetMaterial(i, materialAsset);
									}, settings);
							}

							if (hasLocalMaterial)
							{
								ImGui::SameLine();
								float prevItemHeight = ImGui::GetItemRectSize().y;
								if (ImGui::Button("X", { prevItemHeight, prevItemHeight }))
								{
									mc.Materials->ClearMaterial(i);
								}
								ImGui::NextColumn();
							}
						}
						UI::EndPropertyGrid();
						UI::EndTreeNode();
					}
				}
			});

		DrawComponent<ParticleComponent>("Particles", entity, [&](ParticleComponent& pc)
			{
				UI::BeginPropertyGrid();

				if (UI::Property("Particle Count", pc.ParticleCount, 0, 100000))
				{
					if (pc.ParticleCount < 1)
					{
						pc.ParticleCount = 1;
					}
					pc.MeshObj = Ref<Mesh>::Create(Ref<MeshAsset>::Create(pc.ParticleCount));
				}

				if (UI::PropertyColor("Star Color", pc.StarColor))
				{
					pc.MeshObj->GetMaterials()->GetMaterial(0)->GetMaterial()->Set("uGalaxySpecs.StarColor", pc.StarColor);
				}

				if (UI::PropertyColor("Dust Color", pc.DustColor))
				{
					pc.MeshObj->GetMaterials()->GetMaterial(0)->GetMaterial()->Set("uGalaxySpecs.DustColor", pc.DustColor);
				}

				if (UI::PropertyColor("h2Region Color", pc.h2RegionColor))
				{
					pc.MeshObj->GetMaterials()->GetMaterial(0)->GetMaterial()->Set("uGalaxySpecs.h2RegionColor", pc.h2RegionColor);
				}

				UI::EndPropertyGrid();
			});

		DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& cc)
			{
				UI::BeginPropertyGrid();

				// Projection Type
				const char* projTypeStrings[] = { "Perspective", "Orthographic" };
				int currentProj = (int)cc.CameraObj.GetProjectionType();
				if (UI::PropertyDropdown("Projection", projTypeStrings, 2, &currentProj))
				{
					cc.CameraObj.SetProjectionType((SceneCamera::ProjectionType)currentProj);
				}

				if (cc.CameraObj.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
				{
					float verticalFOV = cc.CameraObj.GetPerspectiveVerticalFOV();
					if (UI::Property("Vertical FOV", verticalFOV))
					{
						cc.CameraObj.SetPerspectiveVerticalFOV(verticalFOV);
					}

					float nearClip = cc.CameraObj.GetPerspectiveNearClip();
					if (UI::Property("Near Clip", nearClip))
					{
						cc.CameraObj.SetPerspectiveNearClip(nearClip);
					}
					ImGui::SameLine();
					float farClip = cc.CameraObj.GetPerspectiveFarClip();
					if (UI::Property("Far Clip", farClip))
					{
						cc.CameraObj.SetPerspectiveFarClip(farClip);
					}
				}
				else if (cc.CameraObj.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
				{
					float orthoSize = cc.CameraObj.GetOrthographicSize();
					if (UI::Property("Size", orthoSize))
					{
						cc.CameraObj.SetOrthographicSize(orthoSize);
					}

					float nearClip = cc.CameraObj.GetOrthographicNearClip();
					if (UI::Property("Near Clip", nearClip))
					{
						cc.CameraObj.SetOrthographicNearClip(nearClip);
					}
					ImGui::SameLine();
					float farClip = cc.CameraObj.GetOrthographicFarClip();
					if (UI::Property("Far Clip", farClip))
					{
						cc.CameraObj.SetOrthographicFarClip(farClip);
					}
				}
				UI::EndPropertyGrid();
			});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& mc)
			{
			});

		DrawComponent<DirectionalLightComponent>("Directional Light", entity, [](DirectionalLightComponent& dlc)
			{
				UI::BeginPropertyGrid();
				UI::PropertyColor("Radiance", dlc.Radiance);
				UI::Property("Intensity", dlc.Intensity);
				UI::Property("Cast Shadows", dlc.CastShadows);
				UI::Property("Soft Shadows", dlc.SoftShadows);
				UI::Property("Source Size", dlc.LightSize);
				UI::EndPropertyGrid();
			});

		DrawComponent<PointLightComponent>("Point Light", entity, [](PointLightComponent& dlc)
			{
				UI::BeginPropertyGrid();
				UI::PropertyColor("Radiance", dlc.Radiance);
				UI::Property("Intensity", dlc.Intensity, 0.05f, 0.f, 500.f);
				UI::Property("Radius", dlc.Radius, 0.1f, 0.f, std::numeric_limits<float>::max());
				UI::Property("Falloff", dlc.Falloff, 0.005f, 0.f, 1.f);
				UI::EndPropertyGrid();
			});

		DrawComponent<SkyLightComponent>("Sky Light", entity, [](SkyLightComponent& slc)
			{
				UI::BeginPropertyGrid();
				UI::PropertyAssetReference("Environment Map", slc.SceneEnvironment);
				UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
				if (slc.SceneEnvironment->RadianceMap)
				{
					UI::PropertySlider("Lod", slc.Lod, 0, slc.SceneEnvironment->RadianceMap->GetMipLevelCount());
				}
				else
				{
					UI::BeginDisabled();
					UI::PropertySlider("Lod", slc.Lod, 0, 10);
					UI::EndDisabled();
				}
				ImGui::Separator();
				UI::Property("Dynamic Sky", slc.DynamicSky);
				if (slc.DynamicSky)
				{
					bool changed = UI::Property("Turbidity", slc.TurbidityAzimuthInclination.x, 0.01f);
					changed |= UI::Property("Azimuth", slc.TurbidityAzimuthInclination.y, 0.01f);
					changed |= UI::Property("Inclination", slc.TurbidityAzimuthInclination.z, 0.01f);
					if (changed)
					{
						Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(slc.TurbidityAzimuthInclination.x, slc.TurbidityAzimuthInclination.y, slc.TurbidityAzimuthInclination.z);
						slc.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
					}
				}
				UI::EndPropertyGrid();
			});

		DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
			{
				UI::BeginPropertyGrid();

				bool isError = !ScriptEngine::ModuleExists(sc.ModuleName);
				std::string name = ScriptEngine::StripNamespace(Project::GetActive()->GetConfig().DefaultNamespace, sc.ModuleName);

				if (UI::Property("Module Name", name, isError))
				{
					// Shutdown old script
					if (!isError)
					{
						ScriptEngine::ShutdownScriptEntity(entity, sc.ModuleName);
					}

					if (ScriptEngine::ModuleExists(name))
					{
						sc.ModuleName = name;
						isError = false;
					}
					else if (ScriptEngine::ModuleExists(Project::GetActive()->GetConfig().DefaultNamespace + "." + name))
					{
						sc.ModuleName = Project::GetActive()->GetConfig().DefaultNamespace + "." + name;
						isError = false;
					}
					else
					{
						sc.ModuleName = name;
						isError = true;
					}

					if (!isError)
					{
						ScriptEngine::InitScriptEntity(entity);
					}
				}

				// Public Fields
				if (!isError)
				{
					EntityInstanceData& entityInstanceData = ScriptEngine::GetEntityInstanceData(entity.GetSceneID(), id);
					auto& moduleFieldMap = entityInstanceData.ModuleFieldMap;
					if (moduleFieldMap.find(sc.ModuleName) != moduleFieldMap.end())
					{
						auto& publicFields = moduleFieldMap.at(sc.ModuleName);
						for (auto& [name, field] : publicFields)
						{
							bool isRuntime = mContext->mIsPlaying && field.IsRuntimeAvailable();
							UI::SetNextPropertyReadOnly(field.IsReadOnly);
							switch (field.Type)
							{
							case FieldType::Int:
							{
								int value = isRuntime ? field.GetValue<int>() : field.GetStoredValue<int>();
								if (UI::Property(field.Name.c_str(), value))
								{
									if (isRuntime)
									{
										field.SetValue(value);
									}
									else
									{
										field.SetStoredValue(value);
									}
								}
								break;
							}
							case FieldType::String:
							{
								const std::string& value = isRuntime ? field.GetValue<std::string>() : field.GetStoredValue<const std::string&>();
								char buffer[256];
								memset(buffer, 0, 256);
								memcpy(buffer, value.c_str(), value.length());
								if (UI::Property(field.Name.c_str(), buffer, 256))
								{
									if (isRuntime)
									{
										field.SetValue<const std::string&>(buffer);
									}
									else
									{
										field.SetStoredValue<const std::string&>(buffer);
									}
								}
								break;
							}
							case FieldType::Float:
							{
								float value = isRuntime ? field.GetValue<float>() : field.GetStoredValue<float>();
								if (UI::Property(field.Name.c_str(), value, 0.2f))
								{
									if (isRuntime)
									{
										field.SetValue(value);
									}
									else
									{
										field.SetStoredValue(value);
									}
								}
								break;
							}
							case FieldType::Vec2:
							{
								glm::vec2 value = isRuntime ? field.GetValue<glm::vec2>() : field.GetStoredValue<glm::vec2>();
								if (UI::Property(field.Name.c_str(), value, 0.2f))
								{
									if (isRuntime)
									{
										field.SetValue(value);
									}
									else
									{
										field.SetStoredValue(value);
									}
								}
								break;
							}
							case FieldType::Vec3:
							{
								glm::vec3 value = isRuntime ? field.GetValue<glm::vec3>() : field.GetStoredValue<glm::vec3>();
								if (UI::Property(field.Name.c_str(), value, 0.2f))
								{
									if (isRuntime)
									{
										field.SetValue(value);
									}
									else
									{
										field.SetStoredValue(value);
									}
								}
								break;
							}
							case FieldType::Vec4:
							{
								glm::vec4 value = isRuntime ? field.GetValue<glm::vec4>() : field.GetStoredValue<glm::vec4>();
								if (UI::Property(field.Name.c_str(), value, 0.2f))
								{
									if (isRuntime)
									{
										field.SetValue(value);
									}
									else
									{
										field.SetStoredValue(value);
									}
								}
								break;
							}
							case FieldType::Asset:
							{
								UUID uuid = isRuntime ? field.GetValue<UUID>() : field.GetStoredValue<UUID>();
								Ref<Prefab> prefab = AssetManager::IsAssetHandleValid(uuid) ? AssetManager::GetAsset<Prefab>(uuid) : nullptr;
								if (UI::PropertyAssetReference(field.Name.c_str(), prefab))
								{
									if (isRuntime)
									{
										field.SetValue(prefab->Handle);
									}
									else
									{
										field.SetStoredValue(prefab->Handle);
									}
								}
								break;
							}
							}
						}
					}
				}

				UI::EndPropertyGrid();
#if TODO
				if (ImGui::Button("Run Script"))
				{
					ScriptEngine::OnCreateEntity(entity);
				}
#endif
		});

		DrawComponent<RigidBody2DComponent>("Rigidbody 2D", entity, [](RigidBody2DComponent& rb2dc)
			{
				UI::BeginPropertyGrid();

				// Rigidbody2D Type
				const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
				UI::PropertyDropdown("Type", rb2dTypeStrings, 3, (int*)&rb2dc.BodyType);

				if (rb2dc.BodyType == RigidBody2DComponent::Type::Dynamic)
				{
					UI::BeginPropertyGrid();
					UI::Property("Fixed Rotation", rb2dc.FixedRotation);
					UI::EndPropertyGrid();
				}

				UI::EndPropertyGrid();
			});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
			{
				UI::BeginPropertyGrid();

				UI::Property("Offset", bc2dc.Offset);
				UI::Property("Size", bc2dc.Size);
				UI::Property("Density", bc2dc.Density);
				UI::Property("Friction", bc2dc.Friction);

				UI::EndPropertyGrid();
			});

		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
			{
				UI::BeginPropertyGrid();

				UI::Property("Offset", cc2dc.Offset);
				UI::Property("Radius", cc2dc.Radius);
				UI::Property("Density", cc2dc.Density);
				UI::Property("Friction", cc2dc.Friction);

				UI::EndPropertyGrid();
			});

		DrawComponent<RigidBodyComponent>("Rigidbody", entity, [&](RigidBodyComponent& rbc)
			{
				UI::BeginPropertyGrid();
				const char* rbTypeStrings[] = { "Static", "Dynamic" };
				UI::PropertyDropdown("Type", rbTypeStrings, 2, (int*)&rbc.BodyType);

				if (rbc.BodyType == RigidBodyComponent::Type::Dynamic)
				{
					if (UI::Property("Is Kinematic", rbc.IsKinematic))
					{
						if (!rbc.IsKinematic && entity.HasComponent<MeshColliderComponent>())
						{
							auto& mcc = entity.GetComponent<MeshColliderComponent>();
							mcc.IsConvex = true;
						}
					}
				}

				if (!PhysicsLayerManager::IsLayerValid(rbc.Layer))
				{
					rbc.Layer = 0;
				}

				int layerCount = PhysicsLayerManager::GetLayerCount();
				const auto& layerNames = PhysicsLayerManager::GetLayerNames();
				UI::PropertyDropdown("Layer", layerNames, layerCount, (int*)&rbc.Layer);

				if (rbc.BodyType == RigidBodyComponent::Type::Dynamic)
				{
					UI::BeginPropertyGrid();
					UI::Property("Mass", rbc.Mass);
					UI::Property("Linear Drag", rbc.LinearDrag);
					UI::Property("Angular Drag", rbc.AngularDrag);
					UI::Property("Disable Gravity", rbc.DisableGravity);
					UI::EndPropertyGrid();

					if (ImGui::TreeNode("Constraints", "Constraints"))
					{
						UI::BeginPropertyGrid();

						UI::BeginCheckboxGroup("Freeze Position");
						UI::PropertyCheckboxGroup("X", rbc.LockPositionX);
						UI::PropertyCheckboxGroup("Y", rbc.LockPositionY);
						UI::PropertyCheckboxGroup("Z", rbc.LockPositionZ);
						UI::EndCheckboxGroup();

						UI::BeginCheckboxGroup("Freeze Rotation");
						UI::PropertyCheckboxGroup("X", rbc.LockRotationX);
						UI::PropertyCheckboxGroup("Y", rbc.LockRotationY);
						UI::PropertyCheckboxGroup("Z", rbc.LockRotationZ);
						UI::EndCheckboxGroup();

						UI::EndPropertyGrid();
						ImGui::TreePop();
					}
				}

				UI::EndPropertyGrid();
			});

		DrawComponent<BoxColliderComponent>("Box Collider", entity, [](BoxColliderComponent& bcc)
			{
				UI::BeginPropertyGrid();

				if (UI::Property("Size", bcc.Size))
				{
					bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
				}

				UI::Property("Offset", bcc.Offset);
				UI::Property("Is Trigger", bcc.IsTrigger);
				UI::PropertyAssetReference("Material", bcc.Material);

				UI::EndPropertyGrid();
			});

		DrawComponent<SphereColliderComponent>("Sphere Collider", entity, [](SphereColliderComponent& scc)
			{
				UI::BeginPropertyGrid();

				if (UI::Property("Radius", scc.Radius))
				{
					scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
				}
				UI::Property("Is Trigger", scc.IsTrigger);
				UI::PropertyAssetReference("Material", scc.Material);

				UI::EndPropertyGrid();
			});

		DrawComponent<CapsuleColliderComponent>("Capsule Collider", entity, [](CapsuleColliderComponent& ccc)
			{
				UI::BeginPropertyGrid();

				bool changed = false;
				if (UI::Property("Radius", ccc.Radius))
				{
					changed = true;
				}
				if (UI::Property("Height", ccc.Height))
				{
					changed = true;
				}
				if (changed)
				{
					ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
				}
				UI::Property("Is Trigger", ccc.IsTrigger);
				UI::PropertyAssetReference("Material", ccc.Material);

				UI::EndPropertyGrid();
			});

		DrawComponent<MeshColliderComponent>("Mesh Collider", entity, [&](MeshColliderComponent& mcc)
			{
				UI::BeginPropertyGrid();

				bool cookMesh = false;
				if (mcc.OverrideMesh && UI::PropertyAssetReference("Mesh", mcc.CollisionMesh))
				{
					cookMesh = true;
				}

				bool wasConvex = mcc.IsConvex;
				if (UI::Property("Is Convex", mcc.IsConvex))
				{
					cookMesh = true;

					if (wasConvex && !mcc.IsConvex && entity.HasComponent<RigidBodyComponent>())
					{
						auto& rb = entity.GetComponent<RigidBodyComponent>();
						if (rb.BodyType == RigidBodyComponent::Type::Dynamic && !rb.IsKinematic)
						{
							mcc.IsConvex = true;
							cookMesh = false;
							NR_CORE_ERROR("MeshColliderComponent must be convex for non-kinematic dynamic Rigidbodies!");
						}
					}
				}

				UI::Property("Is Trigger", mcc.IsTrigger);
				UI::PropertyAssetReference("Material", mcc.Material);

				if (UI::Property("Override Mesh", mcc.OverrideMesh))
				{
					if (!mcc.OverrideMesh && entity.HasComponent<MeshComponent>())
					{
						mcc.CollisionMesh = entity.GetComponent<MeshComponent>().MeshObj;
						cookMesh = true;
					}
				}

				if (cookMesh)
				{
					CookingFactory::CookMesh(mcc, true);
				}

				UI::EndPropertyGrid();
			});


		DrawComponent<AudioListenerComponent>("Audio Listener", entity, [&](AudioListenerComponent& alc)
			{
				UI::BeginPropertyGrid();
				if (UI::Property("Active", alc.Active))
				{
					auto view = mContext->GetAllEntitiesWith<AudioListenerComponent>();
					if (alc.Active == true)
					{
						for (auto ent : view)
						{
							Entity e{ ent, mContext.Raw() };
							e.GetComponent<AudioListenerComponent>().Active = ent == entity;
						}
					}
				}
				float inAngle = glm::degrees(alc.ConeInnerAngleInRadians);
				float outAngle = glm::degrees(alc.ConeOuterAngleInRadians);
				float outGain = alc.ConeOuterGain;
				//? Have to manually clamp here because UI::Property doesn't take flags to pass in ImGuiSliderFlags_ClampOnInput
				if (UI::Property("Inner Cone Angle", inAngle, 1.0f, 0.0f, 360.0f))
				{
					if (inAngle > 360.0f)
					{
						inAngle = 360.0f;
					}
					alc.ConeInnerAngleInRadians = glm::radians(inAngle);
				}
				if (UI::Property("Outer Cone Angle", outAngle, 1.0f, 0.0f, 360.0f))
				{
					if (outAngle > 360.0f)
					{
						outAngle = 360.0f;
					}
					alc.ConeOuterAngleInRadians = glm::radians(outAngle);
				}
				if (UI::Property("Outer Gain", outGain, 0.01f, 0.0f, 1.0f))
				{
					if (outGain > 1.0f)
					{
						outGain = 1.0f;
					}
					alc.ConeOuterGain = outGain;
				}
				UI::EndPropertyGrid();
			});

		DrawComponent<Audio::AudioComponent>("Audio", entity, [&](Audio::AudioComponent& ac)
			{
				auto propertyGridSpacing = []
					{
						ImGui::Spacing();
						ImGui::NextColumn();
						ImGui::NextColumn();
					};

				auto singleColumnSeparator = []
					{
						ImDrawList* draw_list = ImGui::GetWindowDrawList();
						ImVec2 p = ImGui::GetCursorScreenPos();
						draw_list->AddLine(ImVec2(p.x - 9999, p.y), ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
					};

				auto& colors = ImGui::GetStyle().Colors;
				auto oldSCol = colors[ImGuiCol_Separator];
				const float brM = 0.6f;
				colors[ImGuiCol_Separator] = ImVec4{ oldSCol.x * brM, oldSCol.y * brM, oldSCol.z * brM, 1.0f };

				Ref<Audio::SoundConfig> soundConfig = ac.SoundConfig;

				ImGui::Spacing();

				UI::PushID();
				UI::BeginPropertyGrid();

				bool bWasEmpty = soundConfig->FileAsset == nullptr;
				if (UI::PropertyAssetReference("Sound", soundConfig->FileAsset))
				{
					if (bWasEmpty)
					{
						soundConfig->FileAsset.Create();
					}
				}

				propertyGridSpacing();
				propertyGridSpacing();

				if (UI::Property("Volume Multiplier", soundConfig->VolumeMultiplier, 0.01f, 0.0f, 1.0f))
				{
					ac.VolumeMultiplier = soundConfig->VolumeMultiplier;
				}
				if (UI::Property("Pitch Multiplier", soundConfig->PitchMultiplier, 0.01f, 0.0f, 24.0f))
				{
					ac.PitchMultiplier = soundConfig->PitchMultiplier;
				}

				propertyGridSpacing();
				propertyGridSpacing();

				UI::Property("Play on Awake", ac.PlayOnAwake);
				UI::Property("Looping", soundConfig->Looping);

				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();

				auto logFrequencyToNormScale = [](float frequencyN)
					{
						return (log2(frequencyN) + 8.0f) / 8.0f;
					};

				auto sliderScaleToLogFreq = [](float sliderValue)
					{
						return pow(2.0f, 8.0f * sliderValue - 8.0f);
					};

				float lpfFreq = 1.0f - soundConfig->LPFilterValue;
				if (UI::Property("Low-Pass Filter", lpfFreq, 0.0f, 0.0f, 1.0f))
				{
					lpfFreq = std::clamp(lpfFreq, 0.0f, 1.0f);
					soundConfig->LPFilterValue = 1.0f - lpfFreq;
				}

				float hpfFreq = soundConfig->HPFilterValue;
				if (UI::Property("High-Pass Filter", hpfFreq, 0.0f, 0.0f, 1.0f))
				{
					hpfFreq = std::clamp(hpfFreq, 0.0f, 1.0f);
					soundConfig->HPFilterValue = hpfFreq;
				}

				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();

				UI::Property("Master Reverb send", soundConfig->MasterReverbSend, 0.01f, 0.0f, 1.0f);

				UI::EndPropertyGrid();
				UI::PopID();

				if (soundConfig->FileAsset != nullptr)
				{
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					const float space = 10.0f;
					const ImVec2 buttonSize{ 70.0f, 30.0f };
					ImGui::SetCursorPosX(ImGui::GetColumnWidth() - (buttonSize.x * 3));
					if (ImGui::Button("Play", buttonSize))
					{
						Audio::AudioPlayback::Play(ac.ParentHandle);
					}

					ImGui::SameLine(0.0f, space);
					if (ImGui::Button("Stop", buttonSize))
					{
						Audio::AudioPlayback::StopActiveSound(ac.ParentHandle);
					}
					ImGui::SameLine(0.0f, space);
					if (ImGui::Button("Pause", buttonSize))
					{
						Audio::AudioPlayback::PauseActiveSound(ac.ParentHandle);
					}

					ImGui::SetCursorPosX(0);
				}

				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::Spacing();

				ImGui::Text("Spatialization");
				ImGui::SameLine(contentRegionAvailable.x - (ImGui::GetFrameHeight() + GImGui->Style.FramePadding.y));
				ImGui::Checkbox("##enabled", &soundConfig->SpatializationEnabled);

				if (soundConfig->SpatializationEnabled)
				{
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
					ImGui::Spacing();

					using AttModel = Audio::AttenuationModel;
					auto& spatialConfig = soundConfig->Spatialization;
					auto getTextForModel = [&](AttModel model)
						{
							switch (model)
							{
							case AttModel::None:             return "None";
							case AttModel::Inverse:          return "Inverse";
							case AttModel::Linear:           return "Linear";
							case AttModel::Exponential:      return "Exponential";
							}
						};

					const auto& attenModStr = std::vector<std::string>{ getTextForModel(AttModel::None),
																		getTextForModel(AttModel::Inverse),
																		getTextForModel(AttModel::Linear),
																		getTextForModel(AttModel::Exponential) };

					UI::BeginPropertyGrid();

					int32_t selectedModel = static_cast<int32_t>(spatialConfig.AttenuationMod);
					if (UI::PropertyDropdown("Attenuaion Model", attenModStr, attenModStr.size(), &selectedModel))
					{
						spatialConfig.AttenuationMod = static_cast<AttModel>(selectedModel);
					}

					singleColumnSeparator();
					propertyGridSpacing();
					propertyGridSpacing();

					UI::Property("Min Gain", spatialConfig.MinGain, 0.01f, 0.0f, 1.0f);
					UI::Property("Max Gain", spatialConfig.MaxGain, 0.01f, 0.0f, 1.0f);
					UI::Property("Min Distance", spatialConfig.MinDistance, 1.00f, 0.0f, FLT_MAX);
					UI::Property("Max Distance", spatialConfig.MaxDistance, 1.00f, 0.0f, FLT_MAX);

					singleColumnSeparator();
					propertyGridSpacing();
					propertyGridSpacing();

					float inAngle = glm::degrees(spatialConfig.ConeInnerAngleInRadians);
					float outAngle = glm::degrees(spatialConfig.ConeOuterAngleInRadians);
					float outGain = spatialConfig.ConeOuterGain;

					if (UI::Property("Cone Inner Angle", inAngle, 1.0f, 0.0f, 360.0f))
					{
						if (inAngle > 360.0f) inAngle = 360.0f;
						spatialConfig.ConeInnerAngleInRadians = glm::radians(inAngle);
					}
					if (UI::Property("Cone Outer Angle", outAngle, 1.0f, 0.0f, 360.0f))
					{
						if (outAngle > 360.0f) outAngle = 360.0f;
						spatialConfig.ConeOuterAngleInRadians = glm::radians(outAngle);
					}
					if (UI::Property("Cone Outer Gain", outGain, 0.01f, 0.0f, 1.0f))
					{
						if (outGain > 1.0f) outGain = 1.0f;
						spatialConfig.ConeOuterGain = outGain;
					}

					singleColumnSeparator();
					propertyGridSpacing();
					propertyGridSpacing();

					UI::Property("Doppler Factor", spatialConfig.DopplerFactor, 0.01f, 0.0f, 1.0f);

					propertyGridSpacing();
					propertyGridSpacing();

					UI::EndPropertyGrid();
				}

				colors[ImGuiCol_Separator] = oldSCol;
			});
	}
}