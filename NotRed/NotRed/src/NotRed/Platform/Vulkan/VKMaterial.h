#pragma once

#include "NotRed/Renderer/Material.h"

#include "NotRed/Platform/Vulkan/VKShader.h"

namespace NR
{
    class VKMaterial : public Material
    {
    public:
        VKMaterial(const Ref<Shader>& shader, const std::string& name = "");
        virtual ~VKMaterial();

        void Invalidate() override;

        void Set(const std::string& name, uint32_t value) override;
        void Set(const std::string& name, bool value) override;
        void Set(const std::string& name, int value) override;
        void Set(const std::string& name, float value) override;
        void Set(const std::string& name, const glm::vec2& value) override;
        void Set(const std::string& name, const glm::vec3& value) override;
        void Set(const std::string& name, const glm::vec4& value) override;
        void Set(const std::string& name, const glm::mat3& value) override;
        void Set(const std::string& name, const glm::mat4& value) override;

        void Set(const std::string& name, const Ref<Texture2D>& texture) override;
        void Set(const std::string& name, const Ref<TextureCube>& texture) override;
        void Set(const std::string& name, const Ref<Image2D>& image) override;

        uint32_t& GetUInt(const std::string& name) override;
        bool& GetBool(const std::string& name) override;
        int32_t& GetInt(const std::string& name) override;
        float& GetFloat(const std::string& name) override;
        glm::vec2& GetVector2(const std::string& name) override;
        glm::vec3& GetVector3(const std::string& name) override;
        glm::vec4& GetVector4(const std::string& name) override;
        glm::mat3& GetMatrix3(const std::string& name) override;
        glm::mat4& GetMatrix4(const std::string& name) override;

        Ref<Texture2D> GetTexture2D(const std::string& name) override;
        Ref<TextureCube> GetTextureCube(const std::string& name) override;

        Ref<Texture2D> TryGetTexture2D(const std::string& name) override;
        Ref<TextureCube> TryGetTextureCube(const std::string& name) override;

        template <typename T>
        void Set(const std::string& name, const T& value)
        {
            auto decl = FindUniformDeclaration(name);
            NR_CORE_ASSERT(decl, "Could not find uniform!");
            if (!decl)
            {
                return;
            }

            auto& buffer = mUniformStorageBuffer;
            buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());
        }

        template<typename T>
        T& Get(const std::string& name)
        {
            auto decl = FindUniformDeclaration(name);
            NR_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
            auto& buffer = mUniformStorageBuffer;
            return buffer.Read<T>(decl->GetOffset());
        }

        template<typename T>
        Ref<T> GetResource(const std::string& name)
        {
            auto decl = FindResourceDeclaration(name);
            NR_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
            uint32_t slot = decl->GetRegister();
            NR_CORE_ASSERT(slot < mTextures.size(), "Texture slot is invalid!");
            return Ref<T>(mTextures[slot]);
        }

        template<typename T>
        Ref<T> TryGetResource(const std::string& name)
        {
            auto decl = FindResourceDeclaration(name);
            if (!decl)
            {
                return nullptr;
            }

            uint32_t slot = decl->GetRegister();
            if (slot >= mTextures.size())
            {
                return nullptr;
            }

            return Ref<T>(mTextures[slot]);
        }

        virtual uint32_t GetFlags() const override { return mMaterialFlags; }
        virtual bool GetFlag(MaterialFlag flag) const override { return (uint32_t)flag & mMaterialFlags; }
        virtual void ModifyFlags(MaterialFlag flag, bool addFlag = true) override
        {
            if (addFlag)
            {
                mMaterialFlags |= (uint32_t)flag;
            }
            else
            {
                mMaterialFlags &= ~(uint32_t)flag;
            }
        }

        virtual Ref<Shader> GetShader() override { return mShader; }
        virtual const std::string& GetName() const override { return mName; }

        Buffer GetUniformStorageBuffer() { return mUniformStorageBuffer; }

        void UpdateForRendering();

        const VKShader::ShaderMaterialDescriptorSet& GetDescriptorSet() { return mDescriptorSet; }

    private:
        void Init();
        void AllocateStorage();
        void ShaderReloaded();

        void SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture);
        void SetVulkanDescriptor(const std::string& name, const Ref<TextureCube>& texture);
        void SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image);
        void SetVulkanDescriptor(const std::string& name, const VkDescriptorImageInfo& imageInfo);

        const ShaderUniform* FindUniformDeclaration(const std::string& name);
        const ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);

    private:
        enum class PendingDescriptorType
        {
            None,
            Texture2D,
            TextureCube,
            Image2D
        };

        struct PendingDescriptor
        {
            PendingDescriptorType Type = PendingDescriptorType::None;
            VkWriteDescriptorSet WDS;
            VkDescriptorImageInfo ImageInfo{};
            Ref<Texture> Texture;
            Ref<Image> Image;
            VkDescriptorImageInfo SubmittedImageInfo{};
        };

    private:
        Ref<Shader> mShader;
        std::string mName;

        std::vector<std::shared_ptr<PendingDescriptor>> mResidentDescriptors;
        std::vector<std::shared_ptr<PendingDescriptor>> mPendingDescriptors;

        uint32_t mMaterialFlags = 0;

        Buffer mUniformStorageBuffer;
        std::vector<Ref<Texture>> mTextures;
        std::vector<Ref<Image>> mImages;

        std::unordered_map<uint32_t, uint64_t> mImageHashes;

        VKShader::ShaderMaterialDescriptorSet mDescriptorSet;
        std::vector<VkWriteDescriptorSet> mWriteDescriptors;

        std::unordered_map<std::string, VkDescriptorImageInfo> mImageInfos;
    };

}