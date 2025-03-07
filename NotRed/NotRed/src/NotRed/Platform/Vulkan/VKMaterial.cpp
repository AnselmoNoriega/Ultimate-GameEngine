#include "nrpch.h"
#include "VKMaterial.h"

#include "NotRed/Platform/Vulkan/VKContext.h"
#include "NotRed/Platform/Vulkan/VKTexture.h"
#include "NotRed/Platform/Vulkan/VKImage.h"
#include "NotRed/Platform/Vulkan/VKPipeline.h"
#include "NotRed/Platform/Vulkan/VKUniformBuffer.h"

#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Core/Timer.h"

namespace NR
{
    VKMaterial::VKMaterial(const Ref<Shader>& shader, const std::string& name)
        : mShader(shader), mName(name),
        mWriteDescriptors(Renderer::GetConfig().FramesInFlight),
        mDirtyDescriptorSets(Renderer::GetConfig().FramesInFlight, true)
    {
        Init();
        Renderer::RegisterShaderDependency(shader, this);
    }

    VKMaterial::VKMaterial(Ref<Material> material, const std::string& name)
        : mShader(material->GetShader()), mName(name),
        mWriteDescriptors(Renderer::GetConfig().FramesInFlight),
        mDirtyDescriptorSets(Renderer::GetConfig().FramesInFlight, true)
    {
        if (name.empty())
        {
            mName = material->GetName();
        }

        Renderer::RegisterShaderDependency(mShader, this);
        auto vulkanMaterial = material.As<VKMaterial>();

        mUniformStorageBuffer = Buffer::Copy(vulkanMaterial->mUniformStorageBuffer.Data, vulkanMaterial->mUniformStorageBuffer.Size);
        mResidentDescriptors = vulkanMaterial->mResidentDescriptors;
        mResidentDescriptorArrays = vulkanMaterial->mResidentDescriptorArrays;
        mPendingDescriptors = vulkanMaterial->mPendingDescriptors;
        mTextures = vulkanMaterial->mTextures;
        mTextureArrays = vulkanMaterial->mTextureArrays;
        mImages = vulkanMaterial->mImages;
        mImageHashes = vulkanMaterial->mImageHashes;
    }
     
    VKMaterial::~VKMaterial()
    {
    }

    void VKMaterial::Init()
    {
        AllocateStorage();

        mMaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
        mMaterialFlags |= (uint32_t)MaterialFlag::Blend;

#define INVALIDATE_ON_RT 1
#if INVALIDATE_ON_RT
        Ref<VKMaterial> instance = this;
        Renderer::Submit([instance]() mutable
            {
                instance->Invalidate();
            });
#else
        Invalidate();
#endif
    }

    void VKMaterial::Invalidate()
    {
        uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

        auto shader = mShader.As<VKShader>();
        if (shader->HasDescriptorSet(0))
        {
            const auto& shaderDescriptorSets = shader->GetShaderDescriptorSets();
            if (!shaderDescriptorSets.empty())
            {
                for (auto&& [binding, descriptor] : mResidentDescriptors)
                {
                    mPendingDescriptors.push_back(descriptor);
                }
            }
        }
    }

    void VKMaterial::AllocateStorage()
    {
        const auto& shaderBuffers = mShader->GetShaderBuffers();

        if (shaderBuffers.size() > 0)
        {
            uint32_t size = 0;
            for (auto [name, shaderBuffer] : shaderBuffers)
            {
                size += shaderBuffer.Size;
            }

            mUniformStorageBuffer.Allocate(size);
            mUniformStorageBuffer.ZeroInitialize();
        }
    }

    void VKMaterial::ShaderReloaded()
    {
        return;
        AllocateStorage();
    }

    const ShaderUniform* VKMaterial::FindUniformDeclaration(const std::string& name)
    {
        const auto& shaderBuffers = mShader->GetShaderBuffers();

        NR_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

        if (shaderBuffers.size() > 0)
        {
            const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
            if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
            {
                return nullptr;
            }

            return &buffer.Uniforms.at(name);
        }
        return nullptr;
    }

    const ShaderResourceDeclaration* VKMaterial::FindResourceDeclaration(const std::string& name)
    {
        auto& resources = mShader->GetResources();
        for (const auto& [n, resource] : resources)
        {
            if (resource.GetName() == name)
                return &resource;
        }
        return nullptr;
    }

    void VKMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
    {
        const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
        NR_CORE_ASSERT(resource);

        uint32_t binding = resource->GetRegister();
        // Texture is already set
        if (binding < mTextureArrays.size() && mTextureArrays[binding].size() < arrayIndex && texture->GetHash() == mTextureArrays[binding][arrayIndex]->GetHash())
        {
            return;
        }

        if (binding >= mTextureArrays.size())
        {
            mTextureArrays.resize(binding + 1);
        }

        if (arrayIndex >= mTextureArrays[binding].size())
        {
            mTextureArrays[binding].resize(arrayIndex + 1);
        }

        mTextureArrays[binding][arrayIndex] = texture;
        const VkWriteDescriptorSet* wds = mShader.As<VKShader>()->GetDescriptorSet(name);
        NR_CORE_ASSERT(wds);

        if (mResidentDescriptorArrays.find(binding) == mResidentDescriptorArrays.end())
        {
            mResidentDescriptorArrays[binding] = std::make_shared<PendingDescriptorArray>(PendingDescriptorArray{ PendingDescriptorType::Texture2D, *wds, {}, {}, {} });
        }

        auto& residentDesriptorArray = mResidentDescriptorArrays.at(binding);
        if (arrayIndex >= residentDesriptorArray->Textures.size())
        {
            residentDesriptorArray->Textures.resize(arrayIndex + 1);
        }

        residentDesriptorArray->Textures[arrayIndex] = texture;
        
        InvalidateDescriptorSets();
    }

    void VKMaterial::SetVulkanDescriptor(const std::string& name, const Ref<TextureCube>& texture)
    {
        const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
        NR_CORE_ASSERT(resource);

        uint32_t binding = resource->GetRegister();
        if (binding < mTextures.size() && mTextures[binding] && texture->GetHash() == mTextures[binding]->GetHash())
        {
            return;
        }

        if (binding >= mTextures.size())
        {
            mTextures.resize(binding + 1);
        }
        mTextures[binding] = texture;

        const VkWriteDescriptorSet* wds = mShader.As<VKShader>()->GetDescriptorSet(name);
        NR_CORE_ASSERT(wds);
        mResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::TextureCube, *wds, {}, texture.As<Texture>(), nullptr });
        mPendingDescriptors.push_back(mResidentDescriptors.at(binding));
        
        InvalidateDescriptorSets();
    }

    void VKMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image)
    {
        const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
        NR_CORE_ASSERT(resource);

        uint32_t binding = resource->GetRegister();
        if (binding < mImages.size() && mImages[binding] && mImageHashes.at(binding) == image->GetHash())
        {
            return;
        }

        if (resource->GetRegister() >= mImages.size())
        {
            mImages.resize(resource->GetRegister() + 1);
        }
        mImages[resource->GetRegister()] = image;
        mImageHashes[resource->GetRegister()] = image->GetHash();

        const VkWriteDescriptorSet* wds = mShader.As<VKShader>()->GetDescriptorSet(name);
        NR_CORE_ASSERT(wds);
        mResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Image2D, *wds, {}, nullptr, image.As<Image>() });
        mPendingDescriptors.push_back(mResidentDescriptors.at(binding));
        
        InvalidateDescriptorSets();
    }

    void VKMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture)
    {
        const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
        NR_CORE_ASSERT(resource);

        uint32_t binding = resource->GetRegister();
        if (binding < mTextures.size() && mTextures[binding] && texture->GetHash() == mTextures[binding]->GetHash())
        {
            return;
        }

        if (binding >= mTextures.size())
        {
            mTextures.resize(binding + 1);
        }
        mTextures[binding] = texture;

        const VkWriteDescriptorSet* wds = mShader.As<VKShader>()->GetDescriptorSet(name);
        NR_CORE_ASSERT(wds);
        mResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Texture2D, *wds, {}, texture.As<Texture>(), nullptr });
        mPendingDescriptors.push_back(mResidentDescriptors.at(binding));

        InvalidateDescriptorSets();
    }

    void VKMaterial::Set(const std::string& name, uint32_t value)
    {
        Set<uint32_t>(name, value);
    }

    void VKMaterial::Set(const std::string& name, bool value)
    {
        Set<int>(name, (int)value);
    }

    void VKMaterial::Set(const std::string& name, int value)
    {
        Set<int>(name, value);
    }

    void VKMaterial::Set(const std::string& name, float value)
    {
        Set<float>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::ivec2& value)
    {
        Set<glm::ivec2>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::ivec3& value)
    {
        Set<glm::ivec3>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::ivec4& value)
    {
        Set<glm::ivec4>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::vec2& value)
    {
        Set<glm::vec2>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::vec3& value)
    {
        Set<glm::vec3>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::vec4& value)
    {
        Set<glm::vec4>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::mat3& value)
    {
        Set<glm::mat3>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const glm::mat4& value)
    {
        Set<glm::mat4>(name, value);
    }

    void VKMaterial::Set(const std::string& name, const Ref<Texture2D>& texture)
    {
        SetVulkanDescriptor(name, texture);
    }

    void VKMaterial::Set(const std::string& name, const Ref<TextureCube>& texture)
    {
        SetVulkanDescriptor(name, texture);
    }

    void VKMaterial::Set(const std::string& name, const Ref<Image2D>& image)
    {
        SetVulkanDescriptor(name, image);
    }

    void VKMaterial::Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
    {
        SetVulkanDescriptor(name, texture, arrayIndex);
    }

    uint32_t& VKMaterial::GetUInt(const std::string& name)
    {
        return Get<uint32_t>(name);
    }

    int32_t& VKMaterial::GetInt(const std::string& name)
    {
        return Get<int32_t>(name);
    }

    bool& VKMaterial::GetBool(const std::string& name)
    {
        return Get<bool>(name);
    }

    float& VKMaterial::GetFloat(const std::string& name)
    {
        return Get<float>(name);
    }

    glm::vec2& VKMaterial::GetVector2(const std::string& name)
    {
        return Get<glm::vec2>(name);
    }

    glm::vec3& VKMaterial::GetVector3(const std::string& name)
    {
        return Get<glm::vec3>(name);
    }

    glm::vec4& VKMaterial::GetVector4(const std::string& name)
    {
        return Get<glm::vec4>(name);
    }

    glm::mat3& VKMaterial::GetMatrix3(const std::string& name)
    {
        return Get<glm::mat3>(name);
    }

    glm::mat4& VKMaterial::GetMatrix4(const std::string& name)
    {
        return Get<glm::mat4>(name);
    }

    Ref<Texture2D> VKMaterial::GetTexture2D(const std::string& name)
    {
        return GetResource<Texture2D>(name);
    }

    Ref<TextureCube> VKMaterial::TryGetTextureCube(const std::string& name)
    {
        return TryGetResource<TextureCube>(name);
    }

    Ref<Texture2D> VKMaterial::TryGetTexture2D(const std::string& name)
    {
        return TryGetResource<Texture2D>(name);
    }

    Ref<TextureCube> VKMaterial::GetTextureCube(const std::string& name)
    {
        return GetResource<TextureCube>(name);
    }

    void VKMaterial::RT_UpdateForRendering(const std::vector<std::vector<VkWriteDescriptorSet>>& uniformBufferWriteDescriptors)
    {
        NR_SCOPE_PERF("VulkanMaterial::RT_UpdateForRendering");

        auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();

        for (auto&& [binding, descriptor] : mResidentDescriptors)
        {
            if (descriptor->Type == PendingDescriptorType::Image2D)
            {
                Ref<VKImage2D> image = descriptor->Image.As<VKImage2D>();
                NR_CORE_ASSERT(image->GetImageInfo().ImageView, "ImageView is null");
                if (descriptor->WDS.pImageInfo && image->GetImageInfo().ImageView != descriptor->WDS.pImageInfo->imageView)
                {
                    mPendingDescriptors.emplace_back(descriptor);
                    
                    InvalidateDescriptorSets();
                }
            }
        }
        std::vector<VkDescriptorImageInfo> arrayImageInfos;

        uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
        if (mDirtyDescriptorSets[frameIndex] || true)
        {
            mDirtyDescriptorSets[frameIndex] = false;
            mWriteDescriptors[frameIndex].clear();

            if (!uniformBufferWriteDescriptors.empty())
            {
                for (auto& wd : uniformBufferWriteDescriptors[frameIndex])
                {
                    mWriteDescriptors[frameIndex].push_back(wd);
                }
            }

            for (auto&& [binding, pd] : mResidentDescriptors)
            {
                if (pd->Type == PendingDescriptorType::Texture2D)
                {
                    Ref<VKTexture2D> texture = pd->Texture.As<VKTexture2D>();
                    pd->ImageInfo = texture->GetVulkanDescriptorInfo();
                    pd->WDS.pImageInfo = &pd->ImageInfo;
                }
                else if (pd->Type == PendingDescriptorType::TextureCube)
                {
                    Ref<VKTextureCube> texture = pd->Texture.As<VKTextureCube>();
                    pd->ImageInfo = texture->GetVulkanDescriptorInfo();
                    pd->WDS.pImageInfo = &pd->ImageInfo;
                }
                else if (pd->Type == PendingDescriptorType::Image2D)
                {
                    Ref<VKImage2D> image = pd->Image.As<VKImage2D>();
                    pd->ImageInfo = image->GetDescriptor();
                    pd->WDS.pImageInfo = &pd->ImageInfo;
                }

                mWriteDescriptors[frameIndex].push_back(pd->WDS);
            }

            for (auto&& [binding, pd] : mResidentDescriptorArrays)
            {
                if (pd->Type == PendingDescriptorType::Texture2D)
                {
                    for (auto tex : pd->Textures)
                    {
                        Ref<VKTexture2D> texture = tex.As<VKTexture2D>();
                        arrayImageInfos.emplace_back(texture->GetVulkanDescriptorInfo());
                    }
                }

                pd->WDS.pImageInfo = arrayImageInfos.data();
                pd->WDS.descriptorCount = arrayImageInfos.size();
                mWriteDescriptors[frameIndex].push_back(pd->WDS);
            }
        }

        auto vulkanShader = mShader.As<VKShader>();
        auto descriptorSet = vulkanShader->AllocateDescriptorSet();
        mDescriptorSets[frameIndex] = descriptorSet;

        for (auto& writeDescriptor : mWriteDescriptors[frameIndex])
        {
            writeDescriptor.dstSet = descriptorSet.DescriptorSets[0];
        }

        vkUpdateDescriptorSets(vulkanDevice, (uint32_t)mWriteDescriptors[frameIndex].size(), mWriteDescriptors[frameIndex].data(), 0, nullptr);

        mPendingDescriptors.clear();
    }

    void VKMaterial::InvalidateDescriptorSets()
    {
        const uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
        for (uint32_t i = 0; i < framesInFlight; ++i)
        {
            mDirtyDescriptorSets[i] = true;
        }
    }
}