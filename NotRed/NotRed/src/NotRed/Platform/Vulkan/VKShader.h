#pragma once

#include "NotRed/Renderer/Shader.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include "Vulkan.h"

namespace NR
{
    class VKShader : public Shader
    {
    public:
        struct UniformBuffer
        {
            VmaAllocation MemoryAlloc = nullptr;
            VkBuffer Buffer;
            VkDescriptorBufferInfo Descriptor;
            uint32_t Size = 0;
            uint32_t BindingPoint = 0;
            std::string Name;
            VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        };


        struct StorageBuffer
        {
            VmaAllocation MemoryAlloc = nullptr;
            VkBuffer Buffer;
            VkDescriptorBufferInfo Descriptor;
            uint32_t Size = 0;
            uint32_t BindingPoint = 0;
            std::string Name;
            VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        };

        struct ImageSampler
        {
            std::string Name;
            uint32_t BindingPoint = 0;
            uint32_t DescriptorSet = 0;
            VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        };

        struct PushConstantRange
        {
            VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
            uint32_t Offset = 0;
            uint32_t Size = 0;
        };

        struct ShaderDescriptorSet
        {
            std::unordered_map<uint32_t, UniformBuffer*> UniformBuffers;
            std::unordered_map<uint32_t, StorageBuffer*> StorageBuffers;
            std::unordered_map<uint32_t, ImageSampler> ImageSamplers;
            std::unordered_map<uint32_t, ImageSampler> StorageImages;

            std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;

            operator bool() const { return !(UniformBuffers.empty() && ImageSamplers.empty() && StorageImages.empty() && StorageBuffers.empty()); }
        };

        struct ShaderMaterialDescriptorSet
        {
            VkDescriptorPool Pool = nullptr;
            std::vector<VkDescriptorSet> DescriptorSets;
        };

    public:
        VKShader(const std::string& path, bool forceCompile);
        ~VKShader() override;

        void Reload(bool forceCompile = false) override;

        size_t GetHash() const override;

        const std::string& GetName() const override { return  mName; }
        const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const override { return mBuffers; }
        const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const override;

        void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

        // Vulkan-specific
        const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return mPipelineShaderStageCreateInfos; }

        VkDescriptorSet GetDescriptorSet() { return mDescriptorSet; }
        VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t set) { return mDescriptorSetLayouts.at(set); }
        std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts();

        UniformBuffer& GetUniformBuffer(uint32_t binding = 0, uint32_t set = 0) { NR_CORE_ASSERT(mShaderDescriptorSets.at(set).UniformBuffers.size() > binding); return *mShaderDescriptorSets.at(set).UniformBuffers.at(binding); }
        uint32_t GetUniformBufferCount(uint32_t set = 0)
        {
            if (mShaderDescriptorSets.size() < set)
            {
                return 0;
            }

            return (uint32_t)mShaderDescriptorSets[set].UniformBuffers.size();
        }

        const std::vector<ShaderDescriptorSet>& GetShaderDescriptorSets() const { return mShaderDescriptorSets; }
        bool HasDescriptorSet(uint32_t set) const { return mTypeCounts.find(set) != mTypeCounts.end(); }

        const std::vector<PushConstantRange>& GetPushConstantRanges() const { return mPushConstantRanges; }

        ShaderMaterialDescriptorSet AllocateDescriptorSet(uint32_t set = 0);
        ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set = 0);
        ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set, uint32_t numberOfSets);
        const VkWriteDescriptorSet* GetDescriptorSet(const std::string& name, uint32_t set = 0) const;

        static void ClearUniformBuffers();

    private:
        void ParseFile(const std::string& filepath, std::string& output, bool isCompute = false) const;
        void CompileOrGetVulkanBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile);
        void LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
        void Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData);
        void ReflectAllShaderStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);

        void CreateDescriptors();

        void AllocateUniformBuffer(UniformBuffer& dst);
        void AllocateStorageBuffer(StorageBuffer& dst);

    private:
        std::vector<VkPipelineShaderStageCreateInfo> mPipelineShaderStageCreateInfos;
        std::unordered_map<VkShaderStageFlagBits, std::string> mShaderSource;
        std::string mAssetPath;
        std::string mName;

        std::vector<ShaderDescriptorSet> mShaderDescriptorSets;

        std::vector<PushConstantRange> mPushConstantRanges;
        std::unordered_map<std::string, ShaderResourceDeclaration> mResources;

        std::unordered_map<std::string, ShaderBuffer> mBuffers;

        std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
        VkDescriptorSet mDescriptorSet;

        std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> mTypeCounts;
    };
}