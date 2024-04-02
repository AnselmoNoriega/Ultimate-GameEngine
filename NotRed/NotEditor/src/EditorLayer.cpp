#include "EditorLayer.h"

#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Math/Math.h"
#include "NotRed/Utils/PlatformUtils.h"
#include "NotRed/Scene/SceneSerializer.h"

#include "imgui/imgui.h"
#include "ImGuizmo.h"

namespace NR
{
	extern const std::filesystem::path gAssetsPath;

	EditorLayer::EditorLayer()
		:Layer("EditorLayer")
	{
	}

	void EditorLayer::Attach()
	{
		NR_PROFILE_FUNCTION();

		FramebufferStruct fbSpecs;
		fbSpecs.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpecs.Width = 1600;
		fbSpecs.Height = 900;
		mFramebuffer = Framebuffer::Create(fbSpecs);

		mActiveScene = CreateRef<Scene>();
		mSceneHierarchyPanel.SetContext(mActiveScene);

		mEditorCamera = EditorCamera(30.0f, 16 / 9, 0.1f, 1000.0f);
	}

	void EditorLayer::Detach()
	{
	}

	void EditorLayer::Update(float deltaTime)
	{
		NR_PROFILE_FUNCTION();

		if (FramebufferStruct spec = mFramebuffer->GetSpecification();
			mViewportSize.x > 0.0f && mViewportSize.y > 0.0f &&
			(spec.Width != mViewportSize.x || spec.Height != mViewportSize.y))
		{
			mFramebuffer->Resize((uint32_t)mViewportSize.x, (uint32_t)mViewportSize.y);
			mEditorCamera.SetViewportSize(mViewportSize.x, mViewportSize.y);
			mActiveScene->ViewportResize((uint32_t)mViewportSize.x, (uint32_t)mViewportSize.y);
		}

		if (mViewportFocused || mViewportHovered)
		{
			mEditorCamera.Update(deltaTime);
		}

		{
			NR_PROFILE_SCOPE("Render Start");
			Renderer2D::ResetStats();
			mFramebuffer->Bind();
			RenderCommand::SetClearColor({ 0.05f, 0.05f, 0.05f, 1 });
			RenderCommand::Clear();

			mFramebuffer->ClearAttachment(1, -1);
		}

		{
			NR_PROFILE_SCOPE("Rendering");

			mActiveScene->UpdateEditor(deltaTime, mEditorCamera);

			auto [mx, my] = ImGui::GetMousePos();
			mx -= mViewportBounds[0].x;
			my -= mViewportBounds[0].y;
			glm::vec2 viewportSize = mViewportBounds[1] - mViewportBounds[0];
			my = viewportSize.y - my;

			if (mx >= 0.0f && my >= 0.0f && mx < viewportSize.x && my < viewportSize.y)
			{
				int pixelData = mFramebuffer->GetPixel(1, mx, my);
				mHoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, mActiveScene.get());
			}

			mFramebuffer->Unbind();
		}
	}

	void EditorLayer::ImGuiRender()
	{
		NR_PROFILE_FUNCTION();

		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
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
		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;
		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();
		if (opt_fullscreen)
			ImGui::PopStyleVar(2);
		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 200.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows, 
				// which we can't undo at the moment without finer window depth/z control.
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					NewScene();
				}
				if (ImGui::MenuItem("Open...", "Ctrl+O"))
				{
					OpenScene();
				}
				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
				{
					SaveSceneAs();
				}

				if (ImGui::MenuItem("Exit")) Application::Get().Close();
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		mSceneHierarchyPanel.ImGuiRender();
		mBrowserPanel.ImGuiRender();

		ImGui::Begin("Stats");

		std::string hoveredEntityName = "None";
		if (mHoveredEntity)
		{
			hoveredEntityName = mHoveredEntity.GetComponent<TagComponent>().Tag;
		}
		ImGui::Text("Hovered Entity: %s", hoveredEntityName.c_str());

		auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0,0 });
		ImGui::Begin("Viewport");

		auto minBound = ImGui::GetWindowContentRegionMin();
		auto maxBound = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		mViewportBounds[0] = { minBound.x + viewportOffset.x, minBound.y + viewportOffset.y };
		mViewportBounds[1] = { maxBound.x + viewportOffset.x, maxBound.y + viewportOffset.y };

		ImVec2 vpSize = ImGui::GetContentRegionAvail();
		mViewportSize = { vpSize.x, vpSize.y };

		mViewportFocused = ImGui::IsWindowFocused();
		mViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->SetEventsActive(!mViewportFocused && !mViewportHovered);

		uint32_t textureID = mFramebuffer->GetTextureRendererID();
		ImGui::Image((void*)textureID, ImVec2{ mViewportSize.x, mViewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1,0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;

				const wchar_t* fileExtension = wcsrchr(path, L'.');

				if (!wcscmp(fileExtension, L".nr"))
				{
					OpenScene(std::filesystem::path(gAssetsPath) / path);
				}
				else if (!wcscmp(fileExtension, L".png"))
				{
					if (mHoveredEntity && mHoveredEntity.HasComponent<SpriteRendererComponent>())
					{
						std::filesystem::path texturePath = std::filesystem::path(gAssetsPath) / path;
						mHoveredEntity.GetComponent<SpriteRendererComponent>().Texture = Texture2D::Create(texturePath.string());
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		Entity selectedEntity = mSceneHierarchyPanel.GetSelectedEntity();
		if (selectedEntity && mGizmoType >= 0)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

			const glm::mat4& camProj = mEditorCamera.GetProjection();
			glm::mat4 cameraView = mEditorCamera.GetViewMatrix();
			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GetTransform();

			bool snap = Input::IsKeyPressed(KeyCode::Left_Control);
			float snapValue = mGizmoType != ImGuizmo::OPERATION::ROTATE ? 1.0f : 45.0f;

			float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(camProj),
				(ImGuizmo::OPERATION)mGizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
				nullptr, snap ? snapValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);

				glm::vec3 deltaRotation = rotation - tc.Rotation;
				tc.Translation = translation;
				tc.Rotation += deltaRotation;
				tc.Scale = scale;
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& myEvent)
	{
		mEditorCamera.OnEvent(myEvent);

		EventDispatcher dispatch(myEvent);
		dispatch.Dispatch<KeyPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::KeyPressed));
		dispatch.Dispatch<MouseButtonPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::MouseButtonPressed));
	}

	bool EditorLayer::KeyPressed(KeyPressedEvent& e)
	{
		if (e.GetRepeatCount() > 0)
		{
			return false;
		}

		bool ctrl = Input::IsKeyPressed(KeyCode::Left_Control) || Input::IsKeyPressed(KeyCode::Right_Control);
		bool shift = Input::IsKeyPressed(KeyCode::Left_Shift) || Input::IsKeyPressed(KeyCode::Right_Shift);

		switch ((KeyCode)e.GetKeyCode())
		{
		case KeyCode::N:
		{
			if (ctrl)
			{
				NewScene();
			}
			break;
		}
		case KeyCode::O:
		{
			if (ctrl)
			{
				OpenScene();
			}
			break;
		}
		case KeyCode::S:
		{
			if (ctrl && shift)
			{
				SaveSceneAs();
			}
			break;
		}

		//Gizmos
		case KeyCode::Q:
		{
			mGizmoType = -1;
			break;
		}case KeyCode::W:
		{
			mGizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		}case KeyCode::E:
		{
			mGizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		}case KeyCode::R:
		{
			mGizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		}
		}

		return false;
	}

	bool EditorLayer::MouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == (int)MouseCode::ButtonLeft)
		{
			if (mViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(KeyCode::Left_Alt))
			{
				mSceneHierarchyPanel.SetSelecetedEntity(mHoveredEntity);
			}
		}

		return false;
	}

	void EditorLayer::NewScene()
	{
		mActiveScene = CreateRef<Scene>();
		mActiveScene->ViewportResize((uint32_t)mViewportSize.x, (uint32_t)mViewportSize.y);
		mSceneHierarchyPanel.SetContext(mActiveScene);
	}

	void EditorLayer::OpenScene()
	{
		std::string filepath = FileDialogs::OpenFile("NotRed Scene (*.nr)\0*.nr\0");

		if (!filepath.empty())
		{
			OpenScene(filepath);
		}
	}

	void EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		mActiveScene = CreateRef<Scene>();
		mActiveScene->ViewportResize((uint32_t)mViewportSize.x, (uint32_t)mViewportSize.y);
		mSceneHierarchyPanel.SetContext(mActiveScene);

		SceneSerializer serializer(mActiveScene);
		serializer.Deserialize(path.string());
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("NotRed Scene (*.nr)\0*.nr\0");

		if (!filepath.empty())
		{
			SceneSerializer serializer(mActiveScene);
			serializer.Serialize(filepath);
		}
	}
}
