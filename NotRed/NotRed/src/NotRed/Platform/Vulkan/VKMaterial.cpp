#include "nrpch.h"
#include "VKMaterial.h"

#include "NotRed/Platform/Vulkan/VKContext.h"
#include "NotRed/Platform/Vulkan/VKTexture.h"
#include "NotRed/Platform/Vulkan/VKImage.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    VKMaterial::VKMaterial(const Ref<Shader>& shader, const std::string& name)
        : mShader(shader), mName(name)
    {
        Init();
        Renderer::RegisterShaderDependency(shader, this);
    }

    VKMaterial::~VKMaterial()
    {
    }

    void VKMaterial::Init()
    {
        AllocateStorage();

        mMaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
        mMaterialFlags |= (uint32_t)MaterialFlag::Blend;

        Ref<VKShader> vulkanShader = mShader.As<VKShader>();
        for (uint32_t i = 0; i < vulkanShader->GetUniformBufferCount(); ++i)
        {
            auto& uniformBuffer = vulkanShader->GetUniformBuffer(i);
            VkWriteDescriptorSet writeDescriptorSet = {};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.pBufferInfo = &uniformBuffer.Descriptor;
            writeDescriptorSet.dstBinding = uniformBuffer.BindingPoint;
            mWriteDescriptors.push_back(writeDescriptorSet);
        }

        Ref<VKMaterial> instance = this;
        Renderer::Submit([instance]() mutable
            {
                auto shader = instance->GetShader().As<VKShader>();
                const auto& shaderDescriptorSets = shader->GetShaderDescriptorSets();
                if (!shaderDescriptorSets.empty())
                {
                    instance->mDescriptorSet = shader->CreateDescriptorSets();
                }
            });
    }

    void VKMaterial::Invalidate()
    {
        auto shader = mShader.As<VKShader>();
        const auto& shaderDescriptorSets = shader->GetShaderDescriptorSets();
        if (!shaderDescriptorSets.empty())
        {
            mDescriptorSet = shader->CreateDescriptorSets();
            Ref<VKShader> vulkanShader = mShader.As<VKShader>();
            for (uint32_t i = 0; i < vulkanShader->GetUniformBufferCount(); i++)
            {
                auto& uniformBuffer = vulkanShader->GetUniformBuffer(i);
                VkWriteDescriptorSet writeDescriptorSet = {};
                writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSet.descriptorCount = 1;
                writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeDescriptorSet.pBufferInfo = &uniformBuffer.Descriptor;
                writeDescriptorSet.dstBinding = uniformBuffer.BindingPoint;
                mWriteDescriptors.push_back(writeDescriptorSet);
            }
            for (auto&& [binding, descriptor] : mResidentDescriptors)
            {
                mPendingDescriptors.push_back(descriptor);
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
    }

    void VKMaterial::SetVulkanDescriptor(const std::string& name, const VkDescriptorImageInfo& imageInfo)
    {
        const VkWriteDescriptorSet* wds = mShader.As<VKShader>()->GetDescriptorSet(name);
        NR_CORE_ASSERT(wds);

        if (mImageInfos.find(name) != mImageInfos.end())
        {
            if (mImageInfos.at(name).imageView == imageInfo.imageView)
            {
                return;
            }
        }

        mImageInfos[name] = imageInfo;

        VkWriteDescriptorSet descriptorSet = *wds;
        descriptorSet.pImageInfo = &mImageInfos.at(name);
        mWriteDescriptors.push_back(descriptorSet);
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

    void VKMaterial::UpdateForRendering()
    {
        Ref<VKMaterial> instance = this;
        std::vector<VkWriteDescriptorSet>& writeDescriptors = mWriteDescriptors;
        auto& pendingDescriptors = mPendingDescriptors;
        auto& residentDescriptors = mResidentDescriptors;

        Renderer::Submit([instance]() mutable
            {
                auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();

                for (auto&& [binding, descriptor] : instance->mResidentDescriptors)
                {
                    if (descriptor->Type == PendingDescriptorType::Image2D)
                    {
                        Ref<VKImage2D> image = descriptor->Image.As<VKImage2D>();
                        if (descriptor->WDS.pImageInfo && image->GetImageInfo().ImageView != descriptor->WDS.pImageInfo->imageView)
                        {
                            NR_CORE_WARN("Found out of date Image2D descriptor ({0} vs. {1})", (void*)image->GetImageInfo().ImageView, (void*)descriptor->WDS.pImageInfo->imageView);
                            instance->mPendingDescriptors.emplace_back(descriptor);
                        }
                    }
                }

                for (auto& pd : instance->mPendingDescriptors)
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

                    instance->mWriteDescriptors.push_back(pd->WDS);
                }

                if (instance->mWriteDescriptors.size())
                {
                    for (auto& writeDescriptor : instance->mWriteDescriptors)
                        writeDescriptor.dstSet = instance->mDescriptorSet.DescriptorSets[0];

                    NR_CORE_WARN("VKMaterial - Updating {0} descriptor sets", instance->mWriteDescriptors.size());
                    vkUpdateDescriptorSets(vulkanDevice, instance->mWriteDescriptors.size(), instance->mWriteDescriptors.data(), 0, nullptr);
                }

                instance->mPendingDescriptors.clear();
                instance->mWriteDescriptors.clear();
            });
    }

}