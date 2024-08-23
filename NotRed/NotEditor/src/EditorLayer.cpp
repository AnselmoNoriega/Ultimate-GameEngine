#include "EditorLayer.h"

#include "NotRed/ImGui/ImGuizmo.h"
#include "NotRed/Renderer/Renderer2D.h"

namespace NR
{
	static void ImGuiShowHelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	EditorLayer::EditorLayer()
		: mSceneType(SceneType::Model)
	{
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::Attach()
	{
		// ImGui Colors
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f); // Window background
		colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.5f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.3f, 0.3f, 0.3f, 0.5f); // Widget backgrounds
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4f, 0.4f, 0.4f, 0.4f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.6f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.0f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.0f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);
		colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.4f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.0f);
		colors[ImGuiCol_Header] = ImVec4(0.7f, 0.7f, 0.7f, 0.31f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.7f, 0.7f, 0.7f, 0.8f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.5f, 0.52f, 1.0f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.5f, 0.5f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.0f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.43f, 0.35f, 1.0f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.6f, 0.15f, 1.0f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.6f, 0.6f, 1.0f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);

		using namespace glm;

		auto environment = Environment::Load("Assets/Env/HDR_blue_nebulae-1.hdr");

		// Model Scene
		{
			mScene = Ref<Scene>::Create("Model Scene");
			mCameraEntity = mScene->CreateEntity("Camera");
			mCameraEntity.AddComponent<CameraComponent>(Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f)));

			mScene->SetEnvironment(environment);

			mMeshEntity = mScene->CreateEntity();

			auto mesh = Ref<Mesh>::Create("Assets/Meshes/TestScene.fbx");
			mMeshEntity.AddComponent<MeshComponent>(mesh);

			mMeshMaterial = mesh->GetMaterial();

			mMeshEntity.AddComponent<ScriptComponent>("Script");

			// Test Sandbox
			auto mapGenerator = mScene->CreateEntity("Map Generator");
			mapGenerator.AddComponent<ScriptComponent>("Example.MapGenerator");
		}
		// Sphere Scene
		{
			mSphereScene = Ref<Scene>::Create("PBR Sphere Scene");
			auto cameraEntity = mSphereScene->CreateEntity("Camera");
			cameraEntity.AddComponent<CameraComponent>(Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f)));

			mSphereScene->SetEnvironment(environment);

			auto sphereMesh = Ref<Mesh>::Create("Assets/Models/Sphere1m.fbx");
			mSphereBaseMaterial = sphereMesh->GetMaterial();

			float x = -4.0f;
			float roughness = 0.0f;
			for (int i = 0; i < 8; ++i)
			{
				auto sphereEntity = mSphereScene->CreateEntity();

				Ref<MaterialInstance> mi = Ref<MaterialInstance>::Create(mSphereBaseMaterial);
				mi->Set("uMetalness", 1.0f);
				mi->Set("uRoughness", roughness);
				x += 1.1f;
				roughness += 0.15f;
				mMetalSphereMaterialInstances.push_back(mi);
			}

			x = -4.0f;
			roughness = 0.0f;
			for (int i = 0; i < 8; ++i)
			{
				auto sphereEntity = mSphereScene->CreateEntity();

				Ref<MaterialInstance> mi(new MaterialInstance(mSphereBaseMaterial));
				mi->Set("uMetalness", 0.0f);
				mi->Set("uRoughness", roughness);
				x += 1.1f;
				roughness += 0.15f;
				mDielectricSphereMaterialInstances.push_back(mi);
			}
		}

		mActiveScene = mScene;
		mSceneHierarchyPanel = CreateScope<SceneHierarchyPanel>(mActiveScene);

		mCheckerboardTex = Texture2D::Create("Assets/Editor/Checkerboard.tga");

		auto& light = mScene->GetLight();
		light.Direction = { -0.5f, -0.5f, 1.0f };
		light.Radiance = { 1.0f, 1.0f, 1.0f };

		mCurrentlySelectedTransform = &mMeshEntity.Transform();
	}

	void EditorLayer::Detach()
	{
	}

	void EditorLayer::Update(float dt)
	{
		using namespace NR;
		using namespace glm;

		mMeshMaterial->Set("uAlbedoColor", mAlbedoInput.Color);
		mMeshMaterial->Set("uMetalness", mMetalnessInput.Value);
		mMeshMaterial->Set("uRoughness", mRoughnessInput.Value);
		mMeshMaterial->Set("uAlbedoTexToggle", mAlbedoInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uNormalTexToggle", mNormalInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uMetalnessTexToggle", mMetalnessInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uRoughnessTexToggle", mRoughnessInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uEnvMapRotation", mEnvMapRotation);

		mSphereBaseMaterial->Set("uAlbedoColor", mAlbedoInput.Color);
		mSphereBaseMaterial->Set("lights", mScene->GetLight());
		mSphereBaseMaterial->Set("uRadiancePrefilter", mRadiancePrefilter ? 1.0f : 0.0f);
		mSphereBaseMaterial->Set("uAlbedoTexToggle", mAlbedoInput.UseTexture ? 1.0f : 0.0f);
		mSphereBaseMaterial->Set("uNormalTexToggle", mNormalInput.UseTexture ? 1.0f : 0.0f);
		mSphereBaseMaterial->Set("uMetalnessTexToggle", mMetalnessInput.UseTexture ? 1.0f : 0.0f);
		mSphereBaseMaterial->Set("uRoughnessTexToggle", mRoughnessInput.UseTexture ? 1.0f : 0.0f);
		mSphereBaseMaterial->Set("uEnvMapRotation", mEnvMapRotation);

		if (mAlbedoInput.TextureMap)
		{
			mMeshMaterial->Set("uAlbedoTexture", mAlbedoInput.TextureMap);
		}
		if (mNormalInput.TextureMap)
		{
			mMeshMaterial->Set("uNormalTexture", mNormalInput.TextureMap);
		}
		if (mMetalnessInput.TextureMap)
		{
			mMeshMaterial->Set("uMetalnessTexture", mMetalnessInput.TextureMap);
		}
		if (mRoughnessInput.TextureMap)
		{
			mMeshMaterial->Set("uRoughnessTexture", mRoughnessInput.TextureMap);
		}

		mActiveScene->Update(dt);

		if (mDrawOnTopBoundingBoxes)
		{
			NR::Renderer::BeginRenderPass(NR::SceneRenderer::GetFinalRenderPass(), false);
			auto viewProj = mCameraEntity.GetComponent<CameraComponent>().CameraObj.GetViewProjection();

			NR::Renderer2D::BeginScene(viewProj, false);
			Renderer::DrawAABB(mMeshEntity.GetComponent<MeshComponent>(), mMeshEntity.GetComponent<TransformComponent>());
			NR::Renderer2D::EndScene();
			NR::Renderer::EndRenderPass();
		}

		if (mSelectionContext.size())
		{
			auto& selection = mSelectionContext[0];

			NR::Renderer::BeginRenderPass(NR::SceneRenderer::GetFinalRenderPass(), false);
			auto viewProj = mCameraEntity.GetComponent<CameraComponent>().CameraObj.GetViewProjection();
			NR::Renderer2D::BeginScene(viewProj, false);
			glm::vec4 color = (mSelectionMode == SelectionMode::Entity) ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f } : glm::vec4{ 0.2f, 0.9f, 0.2f, 1.0f };
			Renderer::DrawAABB(selection.Mesh->BoundingBox, selection.EntityObj.GetComponent<TransformComponent>().Transform * selection.Mesh->Transform, color);
			NR::Renderer2D::EndScene();
			NR::Renderer::EndRenderPass();
		}
	}

	bool EditorLayer::Property(const std::string& name, bool& value)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::Checkbox(id.c_str(), &value);
		bool result = ImGui::Checkbox(id.c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return result;
	}

	void EditorLayer::Property(const std::string& name, float& value, float min, float max, EditorLayer::PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::SliderFloat(id.c_str(), &value, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void EditorLayer::Property(const std::string& name, glm::vec2& value, EditorLayer::PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void EditorLayer::Property(const std::string& name, glm::vec2& value, float min, float max, EditorLayer::PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void EditorLayer::Property(const std::string& name, glm::vec3& value, EditorLayer::PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void EditorLayer::Property(const std::string& name, glm::vec3& value, float min, float max, EditorLayer::PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void EditorLayer::Property(const std::string& name, glm::vec4& value, EditorLayer::PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void EditorLayer::Property(const std::string& name, glm::vec4& value, float min, float max, EditorLayer::PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
		{
			ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		}
		else
		{
			ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void EditorLayer::ImGuiRender()
	{
		static bool p_open = true;

		static bool opt_fullscreen_persistant = true;
		static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
		bool opt_fullscreen = opt_fullscreen_persistant;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		ImGuiDockNodeFlags ImGuiDockNodeFlags_PassthruDockspace = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoDockingInCentralNode;
		//if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
		//{
		//	window_flags |= ImGuiWindowFlags_NoBackground;
		//}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Dockspace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
		}

		// Editor Panel ------------------------------------------------------------------------------
		ImGui::Begin("Model");
		if (ImGui::RadioButton("Spheres", (int*)&mSceneType, (int)SceneType::Spheres))
		{
			mActiveScene = mSphereScene;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Model", (int*)&mSceneType, (int)SceneType::Model))
		{
			mActiveScene = mScene;
		}

		ImGui::Begin("Environment");

		if (ImGui::Button("Load Environment Map"))
		{
			std::string filename = Application::Get().OpenFile("*.hdr");
			if (filename != "")
			{
				mActiveScene->SetEnvironment(Environment::Load(filename));
			}
		}

		ImGui::SliderFloat("Skybox LOD", &mScene->GetSkyboxLod(), 0.0f, 11.0f);

		ImGui::Columns(2);
		ImGui::AlignTextToFramePadding();

		auto& light = mScene->GetLight();
		Property("Light Direction", light.Direction);
		Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
		Property("Light Multiplier", light.Multiplier, 0.0f, 5.0f);
		Property("Exposure", mCameraEntity.GetComponent<CameraComponent>().CameraObj.GetExposure(), 0.0f, 5.0f);

		Property("Radiance Prefiltering", mRadiancePrefilter);
		Property("Env Map Rotation", mEnvMapRotation, -360.0f, 360.0f);

		if (Property("Show Bounding Boxes", mUIShowBoundingBoxes))
		{
			ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
		}
		if (mUIShowBoundingBoxes && Property("On Top", mUIShowBoundingBoxesOnTop))
		{
			ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
		}

		const char* label = (mSelectionMode == SelectionMode::Entity) ? "Entity" : "Mesh";
		if (ImGui::Button(label))
		{
			mSelectionMode = (mSelectionMode == SelectionMode::Entity) ? SelectionMode::SubMesh : SelectionMode::Entity;
		}

		ImGui::Columns(1);

		ImGui::End();

		ImGui::Separator();
		{
			ImGui::Text("Mesh");
			auto meshComponent = mMeshEntity.GetComponent<MeshComponent>();
			std::string fullpath = meshComponent.MeshObj ? meshComponent.MeshObj->GetFilePath() : "None";
			size_t found = fullpath.find_last_of("/\\");
			std::string path = found != std::string::npos ? fullpath.substr(found + 1) : fullpath;
			ImGui::Text(path.c_str()); ImGui::SameLine();
			if (ImGui::Button("...##Mesh"))
			{
				std::string filename = Application::Get().OpenFile("");
				if (filename != "")
				{
					auto newMesh = Ref<Mesh>::Create(filename);
					meshComponent.MeshObj = newMesh;
				}
			}
		}
		ImGui::Separator();

		// Textures ------------------------------------------------------------------------------
		{
			// Albedo
			if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(mAlbedoInput.TextureMap ? (void*)mAlbedoInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (mAlbedoInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(mAlbedoInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)mAlbedoInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Application::Get().OpenFile("");
						if (filename != "")
						{
							mAlbedoInput.TextureMap = Texture2D::Create(filename, mAlbedoInput.SRGB);
						}
					}
				}
				ImGui::SameLine();
				ImGui::BeginGroup();
				ImGui::Checkbox("Use##AlbedoMap", &mAlbedoInput.UseTexture);
				if (ImGui::Checkbox("sRGB##AlbedoMap", &mAlbedoInput.SRGB))
				{
					if (mAlbedoInput.TextureMap)
					{
						mAlbedoInput.TextureMap = Texture2D::Create(mAlbedoInput.TextureMap->GetPath(), mAlbedoInput.SRGB);
					}
				}
				ImGui::EndGroup();
				ImGui::SameLine();
				ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(mAlbedoInput.Color), ImGuiColorEditFlags_NoInputs);
			}
		}
		{
			// Normals
			if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(mNormalInput.TextureMap ? (void*)mNormalInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (mNormalInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(mNormalInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)mNormalInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Application::Get().OpenFile("");
						if (filename != "")
							mNormalInput.TextureMap = Texture2D::Create(filename);
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##NormalMap", &mNormalInput.UseTexture);
			}
		}
		{
			// Metalness
			if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(mMetalnessInput.TextureMap ? (void*)mMetalnessInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (mMetalnessInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(mMetalnessInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)mMetalnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Application::Get().OpenFile("");
						if (filename != "")
							mMetalnessInput.TextureMap = Texture2D::Create(filename);
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##MetalnessMap", &mMetalnessInput.UseTexture);
				ImGui::SameLine();
				ImGui::SliderFloat("Value##MetalnessInput", &mMetalnessInput.Value, 0.0f, 1.0f);
			}
		}
		{
			// Roughness
			if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(mRoughnessInput.TextureMap ? (void*)mRoughnessInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (mRoughnessInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(mRoughnessInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)mRoughnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Application::Get().OpenFile("");
						if (filename != "")
							mRoughnessInput.TextureMap = Texture2D::Create(filename);
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##RoughnessMap", &mRoughnessInput.UseTexture);
				ImGui::SameLine();
				ImGui::SliderFloat("Value##RoughnessInput", &mRoughnessInput.Value, 0.0f, 1.0f);
			}
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Shader::sAllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload();
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}


		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");

		auto viewportOffset = ImGui::GetCursorPos();
		auto viewportSize = ImGui::GetContentRegionAvail();

		SceneRenderer::SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		mCameraEntity.GetComponent<CameraComponent>().CameraObj.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
		mCameraEntity.GetComponent<CameraComponent>().CameraObj.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		ImGui::Image((void*)SceneRenderer::GetFinalColorBufferRendererID(), viewportSize, { 0, 1 }, { 1, 0 });

		static int counter = 0;
		auto windowSize = ImGui::GetWindowSize();
		ImVec2 minBound = ImGui::GetWindowPos();
		minBound.x += viewportOffset.x;
		minBound.y += viewportOffset.y;

		ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
		mViewportBounds[0] = { minBound.x, minBound.y };
		mViewportBounds[1] = { maxBound.x, maxBound.y };
		mAllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound);

		if (mGizmoType != -1 && mSelectionContext.size())
		{
			auto& selection = mSelectionContext[0];

			float rw = (float)ImGui::GetWindowWidth();
			float rh = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

			auto& camera = mCameraEntity.GetComponent<CameraComponent>().CameraObj;
			bool snap = Input::IsKeyPressed(NR_KEY_LEFT_CONTROL);

			auto& entityTransform = selection.EntityObj.Transform();
			float snapValue[3] = { mSnapValue, mSnapValue, mSnapValue };
			if (mSelectionMode == SelectionMode::Entity)
			{
				ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()),
					glm::value_ptr(camera.GetProjectionMatrix()),
					(ImGuizmo::OPERATION)mGizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(entityTransform),
					nullptr,
					snap ? snapValue : nullptr);
			}
			else
			{
				glm::mat4 transformBase = entityTransform * selection.Mesh->Transform;
				ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()),
					glm::value_ptr(camera.GetProjectionMatrix()),
					(ImGuizmo::OPERATION)mGizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(transformBase),
					nullptr,
					snap ? snapValue : nullptr);

				selection.Mesh->Transform = glm::inverse(entityTransform) * transformBase;
			}
		}
		
		ImGui::End();
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Docking"))
			{
				if (ImGui::MenuItem("Flag: NoSplit", "", (opt_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 
				{
					opt_flags ^= ImGuiDockNodeFlags_NoSplit;
				}
				if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (opt_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  
				{
					opt_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
				}
				if (ImGui::MenuItem("Flag: NoResize", "", (opt_flags & ImGuiDockNodeFlags_NoResize) != 0))                
				{
					opt_flags ^= ImGuiDockNodeFlags_NoResize;
				}
				if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (opt_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          
				{
					opt_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
				}

				ImGui::Separator();
				
				if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL)) 
				{
					p_open = false;
				}
				ImGui::EndMenu();
			}
			ImGuiShowHelpMarker(
				"You can _always_ dock _any_ window into another by holding the SHIFT key while moving a window. Try it now!" "\n"
				"This demo app has nothing to do with it!" "\n\n"
				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)"
			);

			ImGui::EndMenuBar();
		}

		mSceneHierarchyPanel->ImGuiRender();

		ImGui::End();
	}

	void EditorLayer::ShowBoundingBoxes(bool show, bool onTop)
	{
		SceneRenderer::GetOptions().ShowBoundingBoxes = show && !onTop;
		mDrawOnTopBoundingBoxes = show && onTop;
	}

	void EditorLayer::OnEvent(Event& e)
	{
		mScene->OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::OnKeyPressedEvent));
		dispatcher.Dispatch<MouseButtonPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
	}

	bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		switch (e.GetKeyCode())
		{
		case KeyCode::Q: mGizmoType = -1; break;
		case KeyCode::W: mGizmoType = ImGuizmo::OPERATION::TRANSLATE; break;
		case KeyCode::E: mGizmoType = ImGuizmo::OPERATION::ROTATE; break;
		case KeyCode::R: mGizmoType = ImGuizmo::OPERATION::SCALE; break;
		case KeyCode::G:
		{
			if (NR::Input::IsKeyPressed(NR_KEY_LEFT_CONTROL))
			{
				SceneRenderer::GetOptions().ShowGrid = !SceneRenderer::GetOptions().ShowGrid;
			}
			break;
		}
		case KeyCode::B:
		{
			if (NR::Input::IsKeyPressed(NR_KEY_LEFT_CONTROL))
			{
				mUIShowBoundingBoxes = !mUIShowBoundingBoxes;
				ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
			}
			break;
		}
		default: return false;
		}
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		auto [mx, my] = Input::GetMousePosition();
		if (e.GetMouseButton() == NR_MOUSE_BUTTON_LEFT && !Input::IsKeyPressed(KeyCode::LeftAlt) && !ImGuizmo::IsOver())
		{
			auto [mouseX, mouseY] = GetMouseViewportSpace();
			if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
			{
				auto [origin, direction] = CastRay(mouseX, mouseY);

				mSelectionContext.clear();
				auto meshEntities = mScene->GetAllEntitiesWith<MeshComponent>();
				for (auto e : meshEntities)
				{
					Entity entity = { e, mScene.Raw() };
					auto mesh = entity.GetComponent<MeshComponent>().MeshObj;
					if (!mesh)
					{
						continue;
					}

					auto& submeshes = mesh->GetSubmeshes();
					constexpr float lastT = std::numeric_limits<float>::max();
					for (uint32_t i = 0; i < submeshes.size(); ++i)
					{
						auto& submesh = submeshes[i];
						Ray ray = {
							glm::inverse(entity.Transform() * submesh.Transform) * glm::vec4(origin, 1.0f),
							glm::inverse(glm::mat3(entity.Transform()) * glm::mat3(submesh.Transform)) * direction
						};

						float t;
						bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
						if (intersects)
						{
							const auto& triangleCache = mesh->GetTriangleCache(i);
							for (const auto& triangle : triangleCache)
							{
								if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
								{
									NR_WARN("INTERSECTION: {0}, t = {1}", submesh.NodeName, t);
									mSelectionContext.push_back({ entity, &submesh, t });
									break;
								}
							}
						}
					}
				}

				std::sort(mSelectionContext.begin(), mSelectionContext.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });
				if (mSelectionContext.size())
				{
					Selected(mSelectionContext[0]);
				}
			}
		}
		return false;
	}

	std::pair<float, float> EditorLayer::GetMouseViewportSpace()
	{
		auto [mx, my] = ImGui::GetMousePos();
		mx -= mViewportBounds[0].x;
		my -= mViewportBounds[0].y;
		auto viewportWidth = mViewportBounds[1].x - mViewportBounds[0].x;
		auto viewportHeight = mViewportBounds[1].y - mViewportBounds[0].y;

		return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
	}

	std::pair<glm::vec3, glm::vec3> EditorLayer::CastRay(float mx, float my)
	{
		glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

		auto inverseProj = glm::inverse(mCameraEntity.GetComponent<CameraComponent>().CameraObj.GetProjectionMatrix());
		auto inverseView = glm::inverse(glm::mat3(mCameraEntity.GetComponent<CameraComponent>().CameraObj.GetViewMatrix()));

		glm::vec4 ray = inverseProj * mouseClipPos;
		glm::vec3 rayPos = mCameraEntity.GetComponent<CameraComponent>().CameraObj.GetPosition();
		glm::vec3 rayDir = inverseView * glm::vec3(ray);

		return { rayPos, rayDir };
	}

	void EditorLayer::Selected(const SelectedSubmesh& selectionContext)
	{
		mSceneHierarchyPanel->SetSelected(selectionContext.EntityObj);
	}

	Ray EditorLayer::CastMouseRay()
	{
		auto [mouseX, mouseY] = GetMouseViewportSpace();
		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
		{
			auto [origin, direction] = CastRay(mouseX, mouseY);
			return Ray(origin, direction);
		}
		return Ray::Zero();
	}
}