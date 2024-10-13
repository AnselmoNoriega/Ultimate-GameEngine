#include "nrpch.h"
#include "MeshViewerPanel.h"

#include <assimp/scene.h>
#include <filesystem>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/matrix_transform.hpp>

#include <stack>

#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Asset/MeshSerializer.h"

#include "NotRed/ImGui/ImGui.h"
#include "NotRed/Math/Math.h"

namespace NR
{
	MeshViewerPanel::MeshViewerPanel()
		: AssetEditor(nullptr)
	{}

	void MeshViewerPanel::Update(float dt)
	{
		for (auto&& [name, sceneData] : mOpenMeshes)
		{
			sceneData->mCamera.SetActive(sceneData->mViewportPanelFocused);
			sceneData->mCamera.Update(dt);
			if (sceneData->mIsViewportVisible)
			{
				sceneData->mScene->RenderEditor(sceneData->mSceneRenderer, dt, sceneData->mCamera);
			}
		}
	}

	void MeshViewerPanel::OnEvent(Event& e)
	{
		for (auto&& [name, sceneData] : mOpenMeshes)
		{
			if (sceneData->mViewportPanelMouseOver)
			{
				sceneData->mCamera.OnEvent(e);
			}
		}
	}

	void MeshViewerPanel::RenderViewport()
	{}

	namespace Utils 
	{
		static std::stack<float> sMinXSizes, sMinYSizes;
		static std::stack<float> sMaxXSizes, sMaxYSizes;

		static void PushMinSizeX(float minSize)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			sMinXSizes.push(style.WindowMinSize.x);
			style.WindowMinSize.x = minSize;
		}

		static void PopMinSizeX()
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowMinSize.x = sMinXSizes.top();
			sMinXSizes.pop();
		}

		static void PushMinSizeY(float minSize)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			sMinYSizes.push(style.WindowMinSize.y);
			style.WindowMinSize.y = minSize;
		}

		static void PopMinSizeY()
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowMinSize.y = sMinYSizes.top();
			sMinYSizes.pop();
		}

	}

	void MeshViewerPanel::ImGuiRender()
	{
		if (mOpenMeshes.empty() || !mWindowOpen)
		{
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto boldFont = io.Fonts->Fonts[0];
		auto largeFont = io.Fonts->Fonts[1];

		const char* windowName = "Asset Viewer";

		// Dockspace
		{
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin(windowName, &mWindowOpen, window_flags);
			ImGui::PopStyleVar();

			auto window = ImGui::GetCurrentWindow();
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (window->TitleBarRect().Contains(ImGui::GetMousePos()))
				{
					auto monitor = ImGui::GetPlatformIO().Monitors[window->Viewport->PlatformMonitor];
					ImGui::SetWindowPos(windowName, { monitor.WorkPos });
					ImGui::SetWindowSize(windowName, { monitor.WorkSize });
				}
			}

			if (mResetDockspace)
			{
				mResetDockspace = false;
				{
					ImGuiID assetViewerDockspaceID = ImGui::GetID("AssetViewerDockspace");
					ImGui::DockBuilderRemoveNode(assetViewerDockspaceID);
					ImGuiID rootNode = ImGui::DockBuilderAddNode(assetViewerDockspaceID, ImGuiDockNodeFlags_DockSpace);
					ImGui::DockBuilderFinish(assetViewerDockspaceID);
				}
			}

			ImGuiID assetViewerDockspaceID = ImGui::GetID("AssetViewerDockspace");
			ImGui::DockSpace(assetViewerDockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

			for (auto&& [name, sceneData] : mOpenMeshes)
			{
				RenderMeshTab(assetViewerDockspaceID, sceneData);
			}
			ImGui::End();
		}

		if (!mTabToFocus.empty())
		{
			ImGui::SetWindowFocus(mTabToFocus.c_str());
			mTabToFocus.clear();
		}

		// Window has been closed
		if (!mWindowOpen)
		{
			mOpenMeshes.clear();
		}

	}

	void MeshViewerPanel::SetAsset(const Ref<Asset>& asset)
	{
		Ref<MeshAsset> mesh = (Ref<MeshAsset>)asset;

		const std::string& path = mesh->GetFilePath();
		size_t found = path.find_last_of("/\\");
		std::string name = found != std::string::npos ? path.substr(found + 1) : path;
		found = name.find_last_of(".");
		name = found != std::string::npos ? name.substr(0, found) : name;

		if (mOpenMeshes.find(name) != mOpenMeshes.end())
		{
			ImGui::SetWindowFocus(name.c_str());
			mWindowOpen = true;
			return;
		}

		auto& sceneData = mOpenMeshes[name] = std::make_shared<MeshScene>();
		sceneData->mMesh = mesh;
		sceneData->mName = name;
		sceneData->mScene = Ref<Scene>::Create("MeshViewerPanel", true);
		sceneData->mMeshEntity = sceneData->mScene->CreateEntity("Mesh");
		sceneData->mMeshEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(sceneData->mMesh));
		sceneData->mMeshEntity.AddComponent<SkyLightComponent>().DynamicSky = true;

		sceneData->mDirectionaLight = sceneData->mScene->CreateEntity("DirectionalLight");
		sceneData->mDirectionaLight.AddComponent<DirectionalLightComponent>();
		sceneData->mDirectionaLight.GetComponent<TransformComponent>().Rotation = glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f });
		sceneData->mSceneRenderer = Ref<SceneRenderer>::Create(sceneData->mScene);
		sceneData->mSceneRenderer->SetShadowSettings(-15.0f, 15.0f, 0.95f);

		ResetCamera(sceneData->mCamera);
		mTabToFocus = name.c_str();
		mWindowOpen = true;
	}

	void MeshViewerPanel::ResetCamera(EditorCamera& camera)
	{
		camera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
	}

	void MeshViewerPanel::RenderMeshTab(ImGuiID dockspaceID, const std::shared_ptr<MeshScene>& sceneData)
	{
		ImGui::PushID(sceneData->mName.c_str());

		const char* meshTabName = sceneData->mName.c_str();
		std::string toolBarName = fmt::format("##{}-{}", meshTabName, "toolbar");
		std::string viewportPanelName = fmt::format("Viewport##{}", meshTabName);
		std::string propertiesPanelName = fmt::format("Properties##{}", meshTabName);

		{
			ImGui::Begin(meshTabName, nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Window"))
				{
					if (ImGui::MenuItem("Reset Layout"))
						sceneData->mResetDockspace = true;
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGuiID tabDockspaceID = ImGui::GetID("MeshViewerDockspace");
			if (sceneData->mResetDockspace)
			{
				sceneData->mResetDockspace = false;
				ImGui::DockBuilderDockWindow(meshTabName, dockspaceID);

				// Setup dockspace
				ImGui::DockBuilderRemoveNode(tabDockspaceID);
				ImGuiID rootNode = ImGui::DockBuilderAddNode(tabDockspaceID, ImGuiDockNodeFlags_DockSpace);
				ImGuiID dockDown;
				ImGuiID dockUp = ImGui::DockBuilderSplitNode(rootNode, ImGuiDir_Up, 0.05f, nullptr, &dockDown);
				ImGuiID dockLeft;
				ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockDown, ImGuiDir_Right, 0.5f, nullptr, &dockLeft);

				ImGui::DockBuilderDockWindow(toolBarName.c_str(), dockUp);
				ImGui::DockBuilderDockWindow(viewportPanelName.c_str(), dockLeft);
				ImGui::DockBuilderDockWindow(propertiesPanelName.c_str(), dockRight);
				ImGui::DockBuilderFinish(tabDockspaceID);
			}

			Utils::PushMinSizeX(370.0f);
			ImGui::DockSpace(tabDockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGuiWindowClass window_class;
				window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
				ImGui::SetNextWindowClass(&window_class);
				ImGui::Begin(toolBarName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				ImGui::Button("Tool A", ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::Button("Tool B", ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::Button("Tool C", ImVec2(64, 64));
				ImGui::End();
				ImGui::SetNextWindowClass(&window_class);
				ImGui::Begin(viewportPanelName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);

				sceneData->mViewportPanelMouseOver = ImGui::IsWindowHovered();
				sceneData->mViewportPanelFocused = ImGui::IsWindowFocused();

				auto viewportOffset = ImGui::GetCursorPos();
				auto viewportSize = ImGui::GetContentRegionAvail();

				if (viewportSize.y > 0)
				{
					sceneData->mSceneRenderer->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
					sceneData->mCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 1000.0f));
					sceneData->mCamera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
					if (sceneData->mSceneRenderer->GetFinalPassImage())
					{
						UI::ImageButton(sceneData->mSceneRenderer->GetFinalPassImage(), viewportSize, { 0, 1 }, { 1, 0 });
						sceneData->mIsViewportVisible = ImGui::IsItemVisible();
					}

					static int counter = 0;
					auto windowSize = ImGui::GetWindowSize();
					ImVec2 minBound = ImGui::GetWindowPos();
					minBound.x += viewportOffset.x;
					minBound.y += viewportOffset.y;
					ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
					mViewportBounds[0] = { minBound.x, minBound.y };
					mViewportBounds[1] = { maxBound.x, maxBound.y };
				}

				ImGui::End();
				ImGui::PopStyleVar();
			}
			{
				ImGui::Begin(propertiesPanelName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
				DrawMeshNode(sceneData->mMesh);
				ImGui::End();
			}

			ImGui::End();
			Utils::PopMinSizeX();
		}
		ImGui::PopID();
	}

	void MeshViewerPanel::DrawMeshNode(const Ref<MeshAsset>& mesh)
	{
		// Mesh Hierarchy
		auto rootNode = mesh->mScene->mRootNode;
		MeshNodeHierarchy(mesh, rootNode);
		if (ImGui::Button("Create Mesh"))
		{
			NR_CORE_ASSERT(false, "See above");
		}
	}

	static glm::mat4 AssimpMat4ToMat4(const aiMatrix4x4& matrix)
	{
		glm::mat4 result;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
		result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
		result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
		result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
		return result;
	}

	void MeshViewerPanel::MeshNodeHierarchy(const Ref<MeshAsset>& mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = AssimpMat4ToMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		static bool checked = true;
		
		ImGui::Checkbox("##checkbox", &checked);
		ImGui::SameLine();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		
		if (node->mNumChildren == 0)
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		if (ImGui::TreeNodeEx(node->mName.C_Str(), flags))
		{
#if TRANSFORM_INFO
		{
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);
				ImGui::Text("World Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);
				ImGui::Text("Local Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}
#endif

			for (uint32_t i = 0; i < node->mNumChildren; ++i)
			{
				MeshNodeHierarchy(mesh, node->mChildren[i], transform, level + 1);
			}
			ImGui::TreePop();
		}
	}
}