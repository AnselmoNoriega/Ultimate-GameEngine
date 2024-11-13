#include "nrpch.h"
#include "VKShader.h"

#include <filesystem>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv-tools/libspirv.h>

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Core/Hash.h"

#include "VKContext.h"
#include "VKRenderer.h"

namespace NR
{
    namespace Utils
    {
        static const char* GetCacheDirectory()
        {
            return "Resources/Cache/Shader/Vulkan";
        }

        static void CreateCacheDirectoryIfNeeded()
        {
            std::string cacheDirectory = GetCacheDirectory();
            if (!std::filesystem::exists(cacheDirectory))
            {
                std::filesystem::create_directories(cacheDirectory);
            }
        }

        static ShaderUniformType SPIRTypeToShaderUniformType(spirv_cross::SPIRType type)
        {
            switch (type.basetype)
            {
            case spirv_cross::SPIRType::Boolean:  return ShaderUniformType::Bool;
            case spirv_cross::SPIRType::UInt:     return ShaderUniformType::UInt;
            case spirv_cross::SPIRType::Int:
                if (type.vecsize == 1)            return ShaderUniformType::Int;
                if (type.vecsize == 2)            return ShaderUniformType::IVec2;
                if (type.vecsize == 3)            return ShaderUniformType::IVec3;
                if (type.vecsize == 4)            return ShaderUniformType::IVec4;
            case spirv_cross::SPIRType::Float:
            {
                if (type.columns == 3)            return ShaderUniformType::Mat3;
                if (type.columns == 4)            return ShaderUniformType::Mat4;

                if (type.vecsize == 1)            return ShaderUniformType::Float;
                if (type.vecsize == 2)            return ShaderUniformType::Vec2;
                if (type.vecsize == 3)            return ShaderUniformType::Vec3;
                if (type.vecsize == 4)            return ShaderUniformType::Vec4;
                break;
            }
            default:
                NR_CORE_ASSERT(false, "Unknown type!");
                return ShaderUniformType::None;
            }
        }
    }

    static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VKShader::UniformBuffer*>> sUniformBuffers; // set -> binding point -> buffer
    static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VKShader::StorageBuffer*>> sStorageBuffers; // set -> binding point -> buffer

    VKShader::VKShader(const std::string& path, bool forceCompile)
        : mAssetPath(path)
    {
        size_t fileName = path.find_last_of("/\\");
        mName = fileName != std::string::npos ? path.substr(fileName + 1) : path;

        Reload(forceCompile);
    }

    VKShader::~VKShader()
    {
    }

    void VKShader::ClearUniformBuffers()
    {
        sUniformBuffers.clear();
        sStorageBuffers.clear();
    }

    /*TODO: Check if this is better than a method
    static std::string ReadShaderFromFile(const std::string& filepath)
    {
        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            result.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&result[0], result.size());
        }
        else
        {
            NR_CORE_VERIFY(false, "Could not load shader!");
        }
        in.close();
        return result;
    }*/

    void VKShader::Reload(bool forceCompile)
    {
        Ref<VKShader> instance = this;
        Renderer::Submit([instance, forceCompile]() mutable
            {
                // Clear old shader
                instance->mShaderDescriptorSets.clear();
                instance->mResources.clear();
                instance->mPushConstantRanges.clear();
                instance->mPipelineShaderStageCreateInfos.clear();
                instance->mDescriptorSetLayouts.clear();
                instance->mShaderSource.clear();
                instance->mBuffers.clear();
                instance->mTypeCounts.clear();

                Utils::CreateCacheDirectoryIfNeeded();

                // Vertex and Fragment for now
                std::string compute = "";
                std::string path = instance->mAssetPath + "/" + instance->mName + ".comp";
                instance->ParseFile(path, compute, true);

                if (compute == "")
                {
                    path = instance->mAssetPath + "/" + instance->mName + ".vert";
                    std::string vert = "";
                    instance->ParseFile(path, vert);
                    instance->mShaderSource.insert({ VK_SHADER_STAGE_VERTEX_BIT, vert });
                    uint32_t hashV = Hash::GenerateFNVHash(vert.c_str());

                    path = instance->mAssetPath + "/" + instance->mName + ".frag";
                    std::string frag = "";
                    instance->ParseFile(path, frag);
                    instance->mShaderSource.insert({ VK_SHADER_STAGE_FRAGMENT_BIT, frag });
                    uint32_t hashI = Hash::GenerateFNVHash(frag.c_str());
                }
                else
                {
                    instance->mShaderSource.insert({ VK_SHADER_STAGE_COMPUTE_BIT, compute });
                    uint32_t hash = Hash::GenerateFNVHash(compute.c_str());
                }

                std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
                instance->CompileOrGetVulkanBinary(shaderData, forceCompile);
                instance->LoadAndCreateShaders(shaderData);
                instance->ReflectAllShaderStages(shaderData);
                instance->CreateDescriptors();

                Renderer::ShaderReloaded(instance->GetHash());
            });
    }

    size_t VKShader::GetHash() const
    {
        return std::hash<std::string>{}(mAssetPath);
    }

    void VKShader::LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
    {
        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
        mPipelineShaderStageCreateInfos.clear();
        for (auto [stage, data] : shaderData)
        {
            NR_CORE_ASSERT(data.size());

            VkShaderModuleCreateInfo moduleCreateInfo{};
            moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo.codeSize = data.size() * sizeof(uint32_t);
            moduleCreateInfo.pCode = data.data();

            VkShaderModule shaderModule;
            VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

            VkPipelineShaderStageCreateInfo& shaderStage = mPipelineShaderStageCreateInfos.emplace_back();
            shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStage.stage = stage;
            shaderStage.module = shaderModule;
            shaderStage.pName = "main";
        }
    }

    void VKShader::ReflectAllShaderStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
    {
        mResources.clear();

        for (auto [stage, data] : shaderData)
        {
            Reflect(stage, data);
        }
    }

    void VKShader::Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData)
    {
        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        NR_CORE_TRACE("===========================");
        NR_CORE_TRACE(" Vulkan Shader Reflection");
        NR_CORE_TRACE(" {0}", mAssetPath);
        NR_CORE_TRACE("===========================");

        spirv_cross::Compiler compiler(shaderData);
        auto resources = compiler.get_shader_resources();

        NR_CORE_TRACE("Uniform Buffers:");
        for (const auto& resource : resources.uniform_buffers)
        {
            const auto& name = resource.name;
            auto& bufferType = compiler.get_type(resource.base_type_id);
            int memberCount = (uint32_t)bufferType.member_types.size();

            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

            if (descriptorSet >= mShaderDescriptorSets.size())
            {
                mShaderDescriptorSets.resize(descriptorSet + 1);
            }

            ShaderDescriptorSet& shaderDescriptorSet = mShaderDescriptorSets[descriptorSet];
            if (sUniformBuffers[descriptorSet].find(binding) == sUniformBuffers[descriptorSet].end())
            {
                UniformBuffer* uniformBuffer = new UniformBuffer();
                uniformBuffer->BindingPoint = binding;
                uniformBuffer->Size = size;
                uniformBuffer->Name = name;
                uniformBuffer->ShaderStage = VK_SHADER_STAGE_ALL;
                sUniformBuffers.at(descriptorSet)[binding] = uniformBuffer;
            }
            else
            {
                UniformBuffer* uniformBuffer = sUniformBuffers.at(descriptorSet).at(binding);
                if (size > uniformBuffer->Size)
                {
                    uniformBuffer->Size = size;
                }
            }

            shaderDescriptorSet.UniformBuffers[binding] = sUniformBuffers.at(descriptorSet).at(binding);

            NR_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
            NR_CORE_TRACE("  Member Count: {0}", memberCount);
            NR_CORE_TRACE("  Size: {0}", size);
            NR_CORE_TRACE("-------------------");
        }

        NR_CORE_TRACE("Storage Buffers:");
        for (const auto& resource : resources.storage_buffers)
        {
            const auto& name = resource.name;
            auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t memberCount = (uint32_t)bufferType.member_types.size();

            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);
            
            if (descriptorSet >= mShaderDescriptorSets.size())
            {
                mShaderDescriptorSets.resize(descriptorSet + 1);
            }

            ShaderDescriptorSet& shaderDescriptorSet = mShaderDescriptorSets[descriptorSet];
            
            if (sStorageBuffers[descriptorSet].find(binding) == sStorageBuffers[descriptorSet].end())
            {
                StorageBuffer* storageBuffer = new StorageBuffer();
                storageBuffer->BindingPoint = binding;
                storageBuffer->Size = size;
                storageBuffer->Name = name;
                storageBuffer->ShaderStage = VK_SHADER_STAGE_ALL;
                sStorageBuffers.at(descriptorSet)[binding] = storageBuffer;
            }
            else
            {
                StorageBuffer* storageBuffer = sStorageBuffers.at(descriptorSet).at(binding);
                if (size > storageBuffer->Size)
                {
                    storageBuffer->Size = size;
                }
            }
            shaderDescriptorSet.StorageBuffers[binding] = sStorageBuffers.at(descriptorSet).at(binding);
            NR_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
            NR_CORE_TRACE("  Member Count: {0}", memberCount);
            NR_CORE_TRACE("  Size: {0}", size);
            NR_CORE_TRACE("-------------------");
        }

        NR_CORE_TRACE("Push Constant Buffers:");
        for (const auto& resource : resources.push_constant_buffers)
        {
            const auto& bufferName = resource.name;
            auto& bufferType = compiler.get_type(resource.base_type_id);
            auto bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
            uint32_t memberCount = uint32_t(bufferType.member_types.size());
            uint32_t bufferOffset = 0;

            if (mPushConstantRanges.size())
            {
                bufferOffset = mPushConstantRanges.back().Offset + mPushConstantRanges.back().Size;
            }

            auto& pushConstantRange = mPushConstantRanges.emplace_back();
            pushConstantRange.ShaderStage = shaderStage;
            pushConstantRange.Size = bufferSize;
            pushConstantRange.Offset = bufferOffset;

            // Skip empty push constant buffers - these are for the renderer only
            if (bufferName.empty() || bufferName == "uRenderer")
            {
                continue;
            }

            ShaderBuffer& buffer = mBuffers[bufferName];
            buffer.Name = bufferName;
            buffer.Size = bufferSize - bufferOffset;

            NR_CORE_TRACE("  Name: {0}", bufferName);
            NR_CORE_TRACE("  Member Count: {0}", memberCount);
            NR_CORE_TRACE("  Size: {0}", bufferSize);

            for (uint32_t i = 0; i < memberCount; ++i)
            {
                auto type = compiler.get_type(bufferType.member_types[i]);
                const auto& memberName = compiler.get_member_name(bufferType.self, i);
                auto size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, i);
                auto offset = compiler.type_struct_member_offset(bufferType, i) - bufferOffset;

                std::string uniformName = fmt::format("{}.{}", bufferName, memberName);
                buffer.Uniforms[uniformName] = ShaderUniform(uniformName, Utils::SPIRTypeToShaderUniformType(type), size, offset);
            }
        }

        NR_CORE_TRACE("Sampled Images:");
        for (const auto& resource : resources.sampled_images)
        {
            const auto& name = resource.name;
            auto& type = compiler.get_type(resource.base_type_id);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t dimension = type.image.dim;

            if (descriptorSet >= mShaderDescriptorSets.size())
            {
                mShaderDescriptorSets.resize(descriptorSet + 1);
            }

            ShaderDescriptorSet& shaderDescriptorSet = mShaderDescriptorSets[descriptorSet];
            auto& imageSampler = shaderDescriptorSet.ImageSamplers[binding];
            imageSampler.BindingPoint = binding;
            imageSampler.DescriptorSet = descriptorSet;
            imageSampler.Name = name;
            imageSampler.ShaderStage = shaderStage;

            mResources[name] = ShaderResourceDeclaration(name, binding, 1);

            NR_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
        }

        NR_CORE_TRACE("Storage Images:");
        for (const auto& resource : resources.storage_images)
        {
            const auto& name = resource.name;
            auto& type = compiler.get_type(resource.base_type_id);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t dimension = type.image.dim;

            if (descriptorSet >= mShaderDescriptorSets.size())
                mShaderDescriptorSets.resize(descriptorSet + 1);

            ShaderDescriptorSet& shaderDescriptorSet = mShaderDescriptorSets[descriptorSet];
            auto& imageSampler = shaderDescriptorSet.StorageImages[binding];
            imageSampler.BindingPoint = binding;
            imageSampler.DescriptorSet = descriptorSet;
            imageSampler.Name = name;
            imageSampler.ShaderStage = shaderStage;

            mResources[name] = ShaderResourceDeclaration(name, binding, 1);

            NR_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
        }

        NR_CORE_TRACE("===========================");
    }

    void VKShader::CreateDescriptors()
    {
        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        // Descriptor Pool---------------------------------------------------

        mTypeCounts.clear();
        for (uint32_t set = 0; set < mShaderDescriptorSets.size(); ++set)
        {
            auto& shaderDescriptorSet = mShaderDescriptorSets[set];

            if (shaderDescriptorSet.UniformBuffers.size())
            {
                VkDescriptorPoolSize& typeCount = mTypeCounts[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.UniformBuffers.size());
            }
            if (shaderDescriptorSet.StorageBuffers.size())
            {
                VkDescriptorPoolSize& typeCount = mTypeCounts[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.StorageBuffers.size());
            }
            if (shaderDescriptorSet.ImageSamplers.size())
            {
                VkDescriptorPoolSize& typeCount = mTypeCounts[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.ImageSamplers.size());
            }
            if (shaderDescriptorSet.StorageImages.size())
            {
                VkDescriptorPoolSize& typeCount = mTypeCounts[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                typeCount.descriptorCount = (uint32_t)(shaderDescriptorSet.StorageImages.size());
            }

            // Descriptor Set Layout-------------------------------------------------------

            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            for (auto& [binding, uniformBuffer] : shaderDescriptorSet.UniformBuffers)
            {
                VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                layoutBinding.descriptorCount = 1;
                layoutBinding.stageFlags = uniformBuffer->ShaderStage;
                layoutBinding.pImmutableSamplers = nullptr;
                layoutBinding.binding = binding;

                VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[uniformBuffer->Name];
                set = {};
                set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                set.descriptorType = layoutBinding.descriptorType;
                set.descriptorCount = 1;
                set.dstBinding = layoutBinding.binding;
            }

            for (auto& [binding, strorageBuffer] : shaderDescriptorSet.StorageBuffers)
            {
                VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                layoutBinding.descriptorCount = 1;
                layoutBinding.stageFlags = strorageBuffer->ShaderStage;
                layoutBinding.pImmutableSamplers = nullptr;
                layoutBinding.binding = binding;

                VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[strorageBuffer->Name];
                set = {};
                set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                set.descriptorType = layoutBinding.descriptorType;
                set.descriptorCount = 1;
                set.dstBinding = layoutBinding.binding;
            }

            for (auto& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
            {
                auto& layoutBinding = layoutBindings.emplace_back();
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                layoutBinding.descriptorCount = 1;
                layoutBinding.stageFlags = imageSampler.ShaderStage;
                layoutBinding.pImmutableSamplers = nullptr;
                layoutBinding.binding = binding;

                NR_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present!");
                NR_CORE_ASSERT(shaderDescriptorSet.StorageBuffers.find(binding) == shaderDescriptorSet.StorageBuffers.end(), "Binding is already present!");

                VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[imageSampler.Name];
                set = {};
                set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                set.descriptorType = layoutBinding.descriptorType;
                set.descriptorCount = 1;
                set.dstBinding = layoutBinding.binding;
            }

            for (auto& [bindingAndSet, imageSampler] : shaderDescriptorSet.StorageImages)
            {
                auto& layoutBinding = layoutBindings.emplace_back();
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                layoutBinding.descriptorCount = 1;
                layoutBinding.stageFlags = imageSampler.ShaderStage;
                layoutBinding.pImmutableSamplers = nullptr;

                uint32_t binding = bindingAndSet & 0xffffffff;
                layoutBinding.binding = binding;

                NR_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present!");
                NR_CORE_ASSERT(shaderDescriptorSet.StorageBuffers.find(binding) == shaderDescriptorSet.StorageBuffers.end(), "Binding is already present!");
                NR_CORE_ASSERT(shaderDescriptorSet.ImageSamplers.find(binding) == shaderDescriptorSet.ImageSamplers.end(), "Binding is already present!");

                VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[imageSampler.Name];
                set = {};
                set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                set.descriptorType = layoutBinding.descriptorType;
                set.descriptorCount = 1;
                set.dstBinding = layoutBinding.binding;
            }

            VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
            descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayout.pNext = nullptr;
            descriptorLayout.bindingCount = (uint32_t)(layoutBindings.size());
            descriptorLayout.pBindings = layoutBindings.data();

            NR_CORE_INFO("Creating descriptor set {0} with {1} ubo's, {2} ssbo's, {3} samplers and {4} storage images", set,
                shaderDescriptorSet.UniformBuffers.size(),
                shaderDescriptorSet.StorageBuffers.size(),
                shaderDescriptorSet.ImageSamplers.size(),
                shaderDescriptorSet.StorageImages.size());

            if (set >= mDescriptorSetLayouts.size())
            {
                mDescriptorSetLayouts.resize((size_t)(set + 1));
            }
            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &mDescriptorSetLayouts[set]));
        }
    }

    VKShader::ShaderMaterialDescriptorSet VKShader::CreateDescriptorSets(uint32_t set)
    {
        ShaderMaterialDescriptorSet result;

        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        NR_CORE_ASSERT(mTypeCounts.find(set) != mTypeCounts.end());

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = (uint32_t)mTypeCounts.at(set).size();
        descriptorPoolInfo.pPoolSizes = mTypeCounts.at(set).data();
        descriptorPoolInfo.maxSets = 1;

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &result.Pool));

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = result.Pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &mDescriptorSetLayouts[set];

        result.DescriptorSets.emplace_back();
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, result.DescriptorSets.data()));
        return result;
    }

    VKShader::ShaderMaterialDescriptorSet VKShader::CreateDescriptorSets(uint32_t set, uint32_t numberOfSets)
    {
        ShaderMaterialDescriptorSet result;

        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> poolSizes;
        for (uint32_t set = 0; set < mShaderDescriptorSets.size(); set++)
        {
            auto& shaderDescriptorSet = mShaderDescriptorSets[set];
            if (!shaderDescriptorSet)
            {
                continue;
            }

            if (shaderDescriptorSet.UniformBuffers.size())
            {
                VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.UniformBuffers.size() * numberOfSets;
            }
            if (shaderDescriptorSet.StorageBuffers.size())
            {
                VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageBuffers.size() * numberOfSets;
            }
            if (shaderDescriptorSet.ImageSamplers.size())
            {
                VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.ImageSamplers.size() * numberOfSets;
            }
            if (shaderDescriptorSet.StorageImages.size())
            {
                VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
                typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageImages.size() * numberOfSets;
            }
        }

        NR_CORE_ASSERT(poolSizes.find(set) != poolSizes.end());

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext = nullptr;
        descriptorPoolInfo.poolSizeCount = (uint32_t)poolSizes.at(set).size();
        descriptorPoolInfo.pPoolSizes = poolSizes.at(set).data();
        descriptorPoolInfo.maxSets = numberOfSets;

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &result.Pool));

        result.DescriptorSets.resize(numberOfSets);

        for (uint32_t i = 0; i < numberOfSets; ++i)
        {
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = result.Pool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &mDescriptorSetLayouts[set];

            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result.DescriptorSets[i]));
        }
        return result;
    }

    VKShader::ShaderMaterialDescriptorSet VKShader::AllocateDescriptorSet(uint32_t set)
    {
        NR_CORE_ASSERT(set < mDescriptorSetLayouts.size());

        ShaderMaterialDescriptorSet result;
        if (mShaderDescriptorSets.empty())
        {
            return result;
        }

        result.Pool = nullptr;

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &mDescriptorSetLayouts[set];
        VkDescriptorSet descriptorSet = VKRenderer::RT_AllocateDescriptorSet(allocInfo);

        NR_CORE_ASSERT(descriptorSet);
        result.DescriptorSets.push_back(descriptorSet);

        return result;
    }

    const VkWriteDescriptorSet* VKShader::GetDescriptorSet(const std::string& name, uint32_t set) const
    {
        NR_CORE_ASSERT(set < mShaderDescriptorSets.size());
        NR_CORE_ASSERT(mShaderDescriptorSets.at(set));

        if (mShaderDescriptorSets.at(set).WriteDescriptorSets.find(name) == mShaderDescriptorSets.at(set).WriteDescriptorSets.end())
        {
            NR_CORE_WARN("Shader {0} does not contain requested descriptor set {1}", mName, name);
            return nullptr;
        }
        return &mShaderDescriptorSets.at(set).WriteDescriptorSets.at(name);
    }

    std::vector<VkDescriptorSetLayout> VKShader::GetAllDescriptorSetLayouts()
    {
        std::vector<VkDescriptorSetLayout> result;
        result.reserve(mDescriptorSetLayouts.size());
        for (auto& layout : mDescriptorSetLayouts)
        {
            result.emplace_back(layout);
        }

        return result;
    }

    static const char* VkShaderStageCachedFileExtension(VkShaderStageFlagBits stage)
    {
        switch (stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:    return ".cached_vulkan.vert";
        case VK_SHADER_STAGE_FRAGMENT_BIT:  return ".cached_vulkan.frag";
        case VK_SHADER_STAGE_COMPUTE_BIT:   return ".cached_vulkan.comp";
        }
        NR_CORE_ASSERT(false);
        return "";
    }

    static shaderc_shader_kind VkShaderStageToShaderC(VkShaderStageFlagBits stage)
    {
        switch (stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:    return shaderc_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT:  return shaderc_fragment_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT:   return shaderc_compute_shader;
        }
        NR_CORE_ASSERT(false);
        return (shaderc_shader_kind)0;
    }

    std::string GetVKShaderFileExtension(uint32_t stage, std::string& assetPath)
    {
        switch (stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:    return std::string(assetPath) + ".vert";
        case VK_SHADER_STAGE_FRAGMENT_BIT:  return std::string(assetPath) + ".frag";
        case VK_SHADER_STAGE_COMPUTE_BIT:   return std::string(assetPath) + ".comp";
        default:
            NR_CORE_ASSERT(false);
            return "";
        }
    }

    void VKShader::CompileOrGetVulkanBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile)
    {
        std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
        for (auto [stage, source] : mShaderSource)
        {
            auto extension = VkShaderStageCachedFileExtension(stage);
            std::string fullShaderPath = mAssetPath + "/" + mName;
            std::filesystem::path shaderPath = GetVKShaderFileExtension(stage, fullShaderPath);
            if (!forceCompile)
            {
                auto path = cacheDirectory / (shaderPath.filename().string() + extension);
                std::string cachedFilePath = path.string();

                FILE* f;
                errno_t err = fopen_s(&f, cachedFilePath.c_str(), "rb");
                if (!err)
                {
                    fseek(f, 0, SEEK_END);
                    uint64_t size = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    outputBinary[stage] = std::vector<uint32_t>(size / sizeof(uint32_t));
                    fread(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
                    fclose(f);
                }
            }

            if (outputBinary[stage].size() == 0)
            {
                shaderc::Compiler compiler;
                shaderc::CompileOptions options;
                options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
                options.SetWarningsAsErrors();
                options.SetGenerateDebugInfo();

                const bool optimize = false;
                if (optimize)
                {
                    options.SetOptimizationLevel(shaderc_optimization_level_performance);
                }

                // Compile shader
                {
                    auto& shaderSource = mShaderSource.at(stage);
                    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                        shaderSource,
                        VkShaderStageToShaderC(stage),
                        shaderPath.string().c_str(),
                        options);

                    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
                    {
                        NR_CORE_ERROR(module.GetErrorMessage());
                        NR_CORE_ASSERT(false);
                    }

                    const uint8_t* begin = (const uint8_t*)module.cbegin();
                    const uint8_t* end = (const uint8_t*)module.cend();
                    const ptrdiff_t size = end - begin;

                    outputBinary[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
                }

                // Cache compiled shader
                {
                    auto path = cacheDirectory / (shaderPath.filename().string() + extension);
                    std::string cachedFilePath = path.string();

                    FILE* f;
                    fopen_s(&f, cachedFilePath.c_str(), "wb");
                    fwrite(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
                    fclose(f);
                }
            }
        }
    }

    void VKShader::ParseFile(const std::string& filepath, std::string& output, bool isCompute) const
    {
        std::ifstream in(filepath, std::ios::in | std::ios::binary);

        if (in)
        {
            std::string fileContent((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

            if (fileContent.size() >= 3 && fileContent[0] == '\xEF' && fileContent[1] == '\xBB' && fileContent[2] == '\xBF')
            {
                fileContent.erase(0, 3);
            }

            output = fileContent;
        }
        else if (!isCompute)
        {
            NR_CORE_ASSERT(false, "Could not open file");
        }
        in.close();
    }

    const std::unordered_map<std::string, ShaderResourceDeclaration>& VKShader::GetResources() const
    {
        return mResources;
    }

    void VKShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
    {
    }
}