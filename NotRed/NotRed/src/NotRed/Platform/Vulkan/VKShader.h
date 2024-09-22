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
            std::string Name;
            VkDeviceMemory Memory = nullptr;
            VmaAllocation MemoryAlloc = nullptr;
            VkBuffer Buffer;
            uint32_t Size = 0;
            VkDescriptorBufferInfo Descriptor;
            uint32_t BindingPoint = 0;
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
            std::unordered_map<uint32_t, ImageSampler> ImageSamplers;
            std::unordered_map<uint32_t, ImageSampler> StorageImages;

            std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;
        };

        struct ShaderMaterialDescriptorSet
        {
            VkDescriptorPool Pool;
            std::vector<VkDescriptorSet> DescriptorSets;
        };

    public:
        VKShader(const std::string& path, bool forceCompile);
        ~VKShader() override;

        void Bind() override;
        void Reload(bool forceCompile = false) override;

        size_t GetHash() const override;

        RendererID GetRendererID() const override;

        void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) override;

        void SetUniform(const std::string& fullname, uint32_t value) override;
        void SetUniform(const std::string& fullname, int value) override;
        void SetUniform(const std::string& fullname, float value) override;
        void SetUniform(const std::string& fullname, const glm::vec2& value) override;
        void SetUniform(const std::string& fullname, const glm::vec3& value) override;
        void SetUniform(const std::string& fullname, const glm::vec4& value) override;
        void SetUniform(const std::string& fullname, const glm::mat3& value) override;
        void SetUniform(const std::string& fullname, const glm::mat4& value) override;

        void SetUInt(const std::string& name, uint32_t value) override;
        void SetInt(const std::string& name, int value) override;
        void SetFloat(const std::string& name, float value) override;
        void SetFloat2(const std::string& name, const glm::vec2& value) override;
        void SetFloat3(const std::string& name, const glm::vec3& value) override;
        void SetMat4(const std::string& name, const glm::mat4& value) override;
        void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;
        void SetIntArray(const std::string& name, int* values, uint32_t size) override;

        const std::string& GetName() const override { return  mName; }
        const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const override { return mBuffers; }
        const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const override;

        void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

        // Vulkan-specific
        const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return mPipelineShaderStageCreateInfos; }

        static void* MapUniformBuffer(uint32_t bindingPoint, uint32_t set = 0);
        static void UnmapUniformBuffer(uint32_t bindingPoint, uint32_t set = 0);

        VkDescriptorSet GetDescriptorSet() { return mDescriptorSet; }
        VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t set) { return mDescriptorSetLayouts.at(set); }
        std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts();

        UniformBuffer& GetUniformBuffer(uint32_t binding = 0, uint32_t set = 0) { NR_CORE_ASSERT(mShaderDescriptorSets.at(set).UniformBuffers.size() > binding); return *mShaderDescriptorSets.at(set).UniformBuffers[binding]; }
        uint32_t GetUniformBufferCount(uint32_t set = 0)
        {
            if (mShaderDescriptorSets.find(set) == mShaderDescriptorSets.end())
            {
                return 0;
            }

            return mShaderDescriptorSets.at(set).UniformBuffers.size();
        }

        const std::unordered_map<uint32_t, ShaderDescriptorSet>& GetShaderDescriptorSets() const { return mShaderDescriptorSets; }

        const std::vector<PushConstantRange>& GetPushConstantRanges() const { return mPushConstantRanges; }

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

    private:
        std::vector<VkPipelineShaderStageCreateInfo> mPipelineShaderStageCreateInfos;
        std::unordered_map<VkShaderStageFlagBits, std::string> mShaderSource;
        std::string mAssetPath;
        std::string mName;

        std::unordered_map<uint32_t, ShaderDescriptorSet> mShaderDescriptorSets;

        std::vector<PushConstantRange> mPushConstantRanges;
        std::unordered_map<std::string, ShaderResourceDeclaration> mResources;

        std::unordered_map<std::string, ShaderBuffer> mBuffers;

        std::unordered_map<uint32_t, VkDescriptorSetLayout> mDescriptorSetLayouts;
        VkDescriptorSet mDescriptorSet;
        VkDescriptorPool mDescriptorPool;

        std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> mTypeCounts;
    };
}