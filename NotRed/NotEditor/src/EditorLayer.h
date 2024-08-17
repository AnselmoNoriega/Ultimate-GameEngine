#pragma once

#include "NotRed.h"

#include "NotRed/ImGui/ImGuiLayer.h"
#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

namespace NR
{
	class EditorLayer : public Layer
	{
	public:
		enum class PropertyFlag
		{
			None, 
			ColorProperty
		};

	public:
		EditorLayer();
		virtual ~EditorLayer();

		void Attach() override;
		void Detach() override;
		void Update(float dt) override;

		void ImGuiRender() override;
		void OnEvent(Event& event) override;

		// ImGui UI helpers
		void Property(const std::string& name, bool& value);
		void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		void Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
		void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		void Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
		void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		
	private:
		Ref<Shader> mQuadShader;
		Ref<Shader> mHDRShader;
		Ref<Shader> mGridShader;
		Ref<Mesh> mMesh;
		Ref<Mesh> mSphereMesh, mPlaneMesh;
		Ref<Texture2D> mBRDFLUT;

		Ref<MaterialInstance> mMeshMaterial;
		Ref<MaterialInstance> mGridMaterial;
		std::vector<Ref<MaterialInstance>> mMetalSphereMaterialInstances;
		std::vector<Ref<MaterialInstance>> mDielectricSphereMaterialInstances;

		float mGridScale = 16.025f, mGridSize = 0.025f;
		float mMeshScale = 1.0f;

		struct AlbedoInput
		{
			glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; 
			Ref<Texture2D> TextureMap;
			bool SRGB = true;
			bool UseTexture = false;
		};
		AlbedoInput mAlbedoInput;

		struct NormalInput
		{
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		NormalInput mNormalInput;

		struct MetalnessInput
		{
			float Value = 1.0f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		MetalnessInput mMetalnessInput;

		struct RoughnessInput
		{
			float Value = 0.5f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		RoughnessInput mRoughnessInput;

		std::unique_ptr<FrameBuffer> mFramebuffer, mFinalPresentBuffer;

		Ref<VertexBuffer> mVertexBuffer;
		Ref<IndexBuffer> mIndexBuffer;
		Ref<TextureCube> mEnvironmentCubeMap, mEnvironmentIrradiance;

		Camera mCamera;

		struct Light
		{
			glm::vec3 Direction;
			glm::vec3 Radiance;
		};
		Light mLight;
		float mLightMultiplier = 0.3f;

		// PBR params
		float mExposure = 1.0f;

		bool mRadiancePrefilter = false;

		float mEnvMapRotation = 0.0f;

		enum class Scene : uint32_t
		{
			Spheres, 
			Model
		};
		Scene mScene;

		// Editor resources
		Ref<Texture2D> mCheckerboardTex;
	};

}