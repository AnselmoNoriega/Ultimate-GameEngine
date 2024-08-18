#include "EditorLayer.h"

#include "NotRed/ImGui/ImGuizmo.h"

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
		: mScene(Scene::Model), mCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f))
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

		mMesh.reset(new Mesh("Assets/Models/m1911/m1911.fbx"));
		mMeshMaterial.reset(new MaterialInstance(mMesh->GetMaterial()));

		mQuadShader = Shader::Create("Assets/Shaders/Quad");
		mHDRShader = Shader::Create("Assets/Shaders/HDR");

		mSphereMesh.reset(new Mesh("Assets/Models/Sphere1m.fbx"));
		mPlaneMesh.reset(new Mesh("Assets/Models/Plane1m.obj"));

		mGridShader = Shader::Create("Assets/Shaders/Grid");
		mGridMaterial = MaterialInstance::Create(Material::Create(mGridShader));
		mGridMaterial->Set("uScale", mGridScale);
		mGridMaterial->Set("uRes", mGridSize);

		// Editor
		mCheckerboardTex.reset(Texture2D::Create("Assets/editor/Checkerboard.tga"));

		// Environment
		mEnvironmentCubeMap.reset(TextureCube::Create("Assets/Textures/Environments/Arches_E_PineTree_Radiance.tga"));
		//mEnvironmentCubeMap.reset(TextureCube::Create("assets/textures/environments/DebugCubeMap.tga"));
		mEnvironmentIrradiance.reset(TextureCube::Create("Assets/Textures/Environments/Arches_E_PineTree_Irradiance.tga"));
		mBRDFLUT.reset(Texture2D::Create("Assets/Textures/BRDF_LUT.tga"));

		// Render Passes
		FrameBufferSpecification geoFramebufferSpec;
		geoFramebufferSpec.Width = 1280;
		geoFramebufferSpec.Height = 720;
		geoFramebufferSpec.Format = FrameBufferFormat::RGBA16F;
		geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification geoRenderPassSpec;
		geoRenderPassSpec.TargetFrameBuffer = NR::FrameBuffer::Create(geoFramebufferSpec);
		mGeoPass = RenderPass::Create(geoRenderPassSpec);

		FrameBufferSpecification compFramebufferSpec;
		compFramebufferSpec.Width = 1280;
		compFramebufferSpec.Height = 720;
		compFramebufferSpec.Format = FrameBufferFormat::RGBA8;
		compFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification compRenderPassSpec;
		compRenderPassSpec.TargetFrameBuffer = NR::FrameBuffer::Create(compFramebufferSpec);
		mCompositePass = RenderPass::Create(compRenderPassSpec);

		float x = -4.0f;
		float roughness = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			Ref<MaterialInstance> mi(new MaterialInstance(mSphereMesh->GetMaterial()));
			mi->Set("uMetalness", 1.0f);
			mi->Set("uRoughness", roughness);
			mi->Set("uModelMatrix", translate(mat4(1.0f), vec3(x, 0.0f, 0.0f)));
			x += 1.1f;
			roughness += 0.15f;
			mMetalSphereMaterialInstances.push_back(mi);
		}

		x = -4.0f;
		roughness = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			Ref<MaterialInstance> mi(new MaterialInstance(mSphereMesh->GetMaterial()));
			mi->Set("uMetalness", 0.0f);
			mi->Set("uRoughness", roughness);
			mi->Set("uModelMatrix", translate(mat4(1.0f), vec3(x, 1.2f, 0.0f)));
			x += 1.1f;
			roughness += 0.15f;
			mDielectricSphereMaterialInstances.push_back(mi);
		}

		// Create Quad
		x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0);
		data[3].TexCoord = glm::vec2(0, 1);

		mFullscreenQuadVertexArray = VertexArray::Create();
		auto quadVB = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		quadVB->SetLayout({
			{ ShaderDataType::Float3, "aPosition" },
			{ ShaderDataType::Float2, "aTexCoord" }
			});

		uint32_t indices[6] = { 0, 1, 2, 
								2, 3, 0, };
		auto quadIB = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		mFullscreenQuadVertexArray->AddVertexBuffer(quadVB);
		mFullscreenQuadVertexArray->SetIndexBuffer(quadIB);

		mLight.Direction = { -0.5f, -0.5f, 1.0f };
		mLight.Radiance = { 1.0f, 1.0f, 1.0f };

		mTransform = glm::scale(glm::mat4(1.0f), glm::vec3(mMeshScale));
	}

	void EditorLayer::Detach()
	{
	}

	void EditorLayer::Update(float dt)
	{
		using namespace NR;
		using namespace glm;

		mCamera.Update(dt);
		auto viewProjection = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix();

		Renderer::BeginRenderPass(mGeoPass);

		mQuadShader->Bind();
		mQuadShader->SetMat4("uInverseVP", inverse(viewProjection));
		mEnvironmentIrradiance->Bind(0);
		mFullscreenQuadVertexArray->Bind();
		Renderer::DrawIndexed(mFullscreenQuadVertexArray->GetIndexBuffer()->GetCount(), false);

		mMeshMaterial->Set("uAlbedoColor", mAlbedoInput.Color);
		mMeshMaterial->Set("uMetalness", mMetalnessInput.Value);
		mMeshMaterial->Set("uRoughness", mRoughnessInput.Value);
		mMeshMaterial->Set("uViewProjectionMatrix", viewProjection);
		mMeshMaterial->Set("uModelMatrix", scale(mat4(1.0f), vec3(mMeshScale)));
		mMeshMaterial->Set("lights", mLight);
		mMeshMaterial->Set("uCameraPosition", mCamera.GetPosition());
		mMeshMaterial->Set("uRadiancePrefilter", mRadiancePrefilter ? 1.0f : 0.0f);
		mMeshMaterial->Set("uAlbedoTexToggle", mAlbedoInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uNormalTexToggle", mNormalInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uMetalnessTexToggle", mMetalnessInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uRoughnessTexToggle", mRoughnessInput.UseTexture ? 1.0f : 0.0f);
		mMeshMaterial->Set("uEnvMapRotation", mEnvMapRotation);

		mMeshMaterial->Set("uEnvRadianceTex", mEnvironmentCubeMap);
		mMeshMaterial->Set("uEnvIrradianceTex", mEnvironmentIrradiance);
		mMeshMaterial->Set("uBRDFLUTTexture", mBRDFLUT);

		mSphereMesh->GetMaterial()->Set("uAlbedoColor", mAlbedoInput.Color);
		mSphereMesh->GetMaterial()->Set("uMetalness", mMetalnessInput.Value);
		mSphereMesh->GetMaterial()->Set("uRoughness", mRoughnessInput.Value);
		mSphereMesh->GetMaterial()->Set("uViewProjectionMatrix", viewProjection);
		mSphereMesh->GetMaterial()->Set("uModelMatrix", scale(mat4(1.0f), vec3(mMeshScale)));
		mSphereMesh->GetMaterial()->Set("lights", mLight);
		mSphereMesh->GetMaterial()->Set("uCameraPosition", mCamera.GetPosition());
		mSphereMesh->GetMaterial()->Set("uRadiancePrefilter", mRadiancePrefilter ? 1.0f : 0.0f);
		mSphereMesh->GetMaterial()->Set("uAlbedoTexToggle", mAlbedoInput.UseTexture ? 1.0f : 0.0f);
		mSphereMesh->GetMaterial()->Set("uNormalTexToggle", mNormalInput.UseTexture ? 1.0f : 0.0f);
		mSphereMesh->GetMaterial()->Set("uMetalnessTexToggle", mMetalnessInput.UseTexture ? 1.0f : 0.0f);
		mSphereMesh->GetMaterial()->Set("uRoughnessTexToggle", mRoughnessInput.UseTexture ? 1.0f : 0.0f);
		mSphereMesh->GetMaterial()->Set("uEnvMapRotation", mEnvMapRotation);
		mSphereMesh->GetMaterial()->Set("uEnvRadianceTex", mEnvironmentCubeMap);
		mSphereMesh->GetMaterial()->Set("uEnvIrradianceTex", mEnvironmentIrradiance);
		mSphereMesh->GetMaterial()->Set("uBRDFLUTTexture", mBRDFLUT);

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

		if (mScene == Scene::Spheres)
		{
			for (int i = 0; i < 8; i++)
			{
				mSphereMesh->Render(dt, glm::mat4(1.0f), mMetalSphereMaterialInstances[i]);
			}

			for (int i = 0; i < 8; i++)
			{
				mSphereMesh->Render(dt, glm::mat4(1.0f), mDielectricSphereMaterialInstances[i]);
			}
		}
		else if (mScene == Scene::Model)
		{
			if (mMesh)
			{
				mMesh->Render(dt, mTransform, mMeshMaterial);
			}
		}

		mGridMaterial->Set("uMVP", viewProjection * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f)));
		mPlaneMesh->Render(dt, mGridMaterial);

		Renderer::EndRenderPass();

		Renderer::BeginRenderPass(mCompositePass);
		mHDRShader->Bind();
		mHDRShader->SetFloat("uExposure", mExposure);
		mGeoPass->GetSpecification().TargetFrameBuffer->BindTexture();
		mFullscreenQuadVertexArray->Bind();
		Renderer::DrawIndexed(mFullscreenQuadVertexArray->GetIndexBuffer()->GetCount(), false);
		Renderer::EndRenderPass();
	}

	void EditorLayer::Property(const std::string& name, bool& value)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::Checkbox(id.c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
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
			ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);

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
		ImGui::RadioButton("Spheres", (int*)&mScene, (int)Scene::Spheres);
		ImGui::SameLine();
		ImGui::RadioButton("Model", (int*)&mScene, (int)Scene::Model);

		ImGui::Begin("Environment");

		ImGui::Columns(2);
		ImGui::AlignTextToFramePadding();

		Property("Light Direction", mLight.Direction);
		Property("Light Radiance", mLight.Radiance, PropertyFlag::ColorProperty);
		Property("Light Multiplier", mLightMultiplier, 0.0f, 5.0f);
		Property("Exposure", mExposure, 0.0f, 5.0f);

		Property("Mesh Scale", mMeshScale, 0.0f, 2.0f);

		Property("Radiance Prefiltering", mRadiancePrefilter);
		Property("Env Map Rotation", mEnvMapRotation, -360.0f, 360.0f);

		ImGui::Columns(1);

		ImGui::End();

		ImGui::Separator();
		{
			ImGui::Text("Mesh");
			std::string fullpath = mMesh ? mMesh->GetFilePath() : "None";
			size_t found = fullpath.find_last_of("/\\");
			std::string path = found != std::string::npos ? fullpath.substr(found + 1) : fullpath;
			ImGui::Text(path.c_str()); ImGui::SameLine();
			if (ImGui::Button("...##Mesh"))
			{
				std::string filename = Application::Get().OpenFile("");
				if (filename != "")
				{
					mMesh.reset(new Mesh(filename));
					mMeshMaterial.reset(new MaterialInstance(mMesh->GetMaterial()));
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
							mAlbedoInput.TextureMap.reset(Texture2D::Create(filename, mAlbedoInput.SRGB));
					}
				}
				ImGui::SameLine();
				ImGui::BeginGroup();
				ImGui::Checkbox("Use##AlbedoMap", &mAlbedoInput.UseTexture);
				if (ImGui::Checkbox("sRGB##AlbedoMap", &mAlbedoInput.SRGB))
				{
					if (mAlbedoInput.TextureMap)
						mAlbedoInput.TextureMap.reset(Texture2D::Create(mAlbedoInput.TextureMap->GetPath(), mAlbedoInput.SRGB));
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
							mNormalInput.TextureMap.reset(Texture2D::Create(filename));
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
							mMetalnessInput.TextureMap.reset(Texture2D::Create(filename));
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
							mRoughnessInput.TextureMap.reset(Texture2D::Create(filename));
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

		auto viewportSize = ImGui::GetContentRegionAvail();

		mGeoPass->GetSpecification().TargetFrameBuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		mCompositePass->GetSpecification().TargetFrameBuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		mCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
		mCamera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		ImGui::Image((void*)mCompositePass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(), viewportSize, { 0, 1 }, { 1, 0 });

		if (mGizmoType != -1)
		{
			float rw = (float)ImGui::GetWindowWidth();
			float rh = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);
			ImGuizmo::Manipulate(glm::value_ptr(mCamera.GetViewMatrix()), glm::value_ptr(mCamera.GetProjectionMatrix()), (ImGuizmo::OPERATION)mGizmoType, ImGuizmo::LOCAL, glm::value_ptr(mTransform));
		}
		
		ImGui::End();
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Docking"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows, 
				// which we can't undo at the moment without finer window depth/z control.
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

				if (ImGui::MenuItem("Flag: NoSplit", "", (opt_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 opt_flags ^= ImGuiDockNodeFlags_NoSplit;
				if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (opt_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  opt_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
				if (ImGui::MenuItem("Flag: NoResize", "", (opt_flags & ImGuiDockNodeFlags_NoResize) != 0))                opt_flags ^= ImGuiDockNodeFlags_NoResize;
				//if (ImGui::MenuItem("Flag: PassthruDockspace", "", (opt_flags & ImGuiDockNodeFlags_PassthruDockspace) != 0))       opt_flags ^= ImGuiDockNodeFlags_PassthruDockspace;
				if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (opt_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          opt_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
				ImGui::Separator();
				if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
					p_open = false;
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

		ImGui::End();

		if (mMesh)
		{
			mMesh->ImGuiRender();
		}
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::OnKeyPressedEvent));
	}

	bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		switch (e.GetKeyCode())
		{
		case NR_KEY_Q: mGizmoType = -1; break;
		case NR_KEY_W: mGizmoType = ImGuizmo::OPERATION::TRANSLATE; break;
		case NR_KEY_E: mGizmoType = ImGuizmo::OPERATION::ROTATE; break;
		case NR_KEY_R: mGizmoType = ImGuizmo::OPERATION::SCALE; break;
		default: return false;
		}
	}
}