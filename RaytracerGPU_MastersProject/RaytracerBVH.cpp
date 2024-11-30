
#include "RaytracerBVH.hpp"
#include <array>
#include "Scenes.hpp"

namespace RaytracerBVHRenderer {
	Raytracer::Raytracer() :
		window{ 800, 800, "Compute-based Images" },
		device{ window } {
		this->initVulkan();
	}
	Raytracer::~Raytracer() {
		this->modelToWorldPipeline = nullptr;
		vkDestroyPipelineLayout(this->device.device(), this->modelToWorldPipelineLayout, nullptr);
		this->raytracePipeline = nullptr;
		vkDestroyPipelineLayout(this->device.device(), this->raytracePipelineLayout, nullptr);

		this->graphicsPipeline = nullptr;
		vkDestroyPipelineLayout(this->device.device(), this->graphicsPipelineLayout, nullptr);

		vkDestroyImage(this->device.device(), this->computeImage, nullptr);
		vkDestroyImageView(this->device.device(), this->computeImageView, nullptr);
		vkDestroySampler(this->device.device(), this->fragmentShaderImageSampler, nullptr);
		vkFreeMemory(this->device.device(), this->computeImageMemory, nullptr);

		vkDestroyFence(this->device.device(), this->computeS1Complete, nullptr);
		vkDestroyFence(this->device.device(), this->computeS2Complete, nullptr);

		this->rayUniformBuffer = nullptr; // deconstruct uniformBuffer
		this->fragUniformBuffer = nullptr;
		// scene handles deconstructing ssbos // deconstruct ssbos
		this->modelToWorldDescriptorSetLayout = nullptr;
		this->raytraceDescriptorSetLayout = nullptr; // deconstruct descriptorSetLayout

		this->modelToWorldDescriptorPool = nullptr; // deconstruct descriptorPool
		this->raytraceDescriptorPool = nullptr;
		this->graphicsDescriptorPool = nullptr;
	}

	auto Raytracer::createSwapChain() -> void {
		auto extent = this->window.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = this->window.getExtent();
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(this->device.device());
		if (this->swapChain == nullptr)
			this->swapChain = std::make_unique<SwapChain>(this->device, extent);
		else {
			std::runtime_error("swap chain already created! (can use this for swap chain recreation later caused by window resize)");
		}
	}

	auto Raytracer::createComputeDescriptorSetLayout() -> void {
		this->modelToWorldDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				3,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).build();
		this->enclosingAABBDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				3,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				4,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).build();
		this->generateMortonCodeDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				3,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				4,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				5,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).build();
		this->radixSortDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).build();
		this->constructHLBVHDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				3,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				4,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				5,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).build();
		this->constructAABBDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).build();
		this->raytraceDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				3,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				4,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				5,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).addBinding(
				6,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1
			).build();
	}
	auto Raytracer::createGraphicsDescriptorSetLayout() -> void {
		this->graphicsDescriptorSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1
			)
			.addBinding(
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1
			).build();
	}

	auto Raytracer::createComputePipelineLayout() -> void {
		VkDescriptorSetLayout tempSpace = this->modelToWorldDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo1{};
		pipelineLayoutInfo1.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo1.setLayoutCount = 1;
		pipelineLayoutInfo1.pSetLayouts = &tempSpace;

		if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo1, nullptr, &this->modelToWorldPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline layout!");

		VkDescriptorSetLayout tempEnclosing = this->enclosingAABBDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo7{};
		pipelineLayoutInfo7.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo7.setLayoutCount = 1;
		pipelineLayoutInfo7.pSetLayouts = &tempEnclosing;

		if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo7, nullptr, &this->enclosingAABBPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline layout!");

		VkDescriptorSetLayout tempMortonCode = this->generateMortonCodeDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo3{};
		pipelineLayoutInfo3.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo3.setLayoutCount = 1;
		pipelineLayoutInfo3.pSetLayouts = &tempMortonCode;

		if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo3, nullptr, &this->generateMortonCodePipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline layout!");

		VkDescriptorSetLayout tempRadixSort = this->radixSortDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo4{};
		pipelineLayoutInfo4.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo4.setLayoutCount = 1;
		pipelineLayoutInfo4.pSetLayouts = &tempRadixSort;

		if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo4, nullptr, &this->radixSortPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline layout!");

		VkDescriptorSetLayout tempBVH = this->constructHLBVHDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo5{};
		pipelineLayoutInfo5.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo5.setLayoutCount = 1;
		pipelineLayoutInfo5.pSetLayouts = &tempBVH;

		if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo5, nullptr, &this->constructHLBVHPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline layout!");

		VkDescriptorSetLayout tempAABB = this->constructAABBDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo2{};
		pipelineLayoutInfo2.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo2.setLayoutCount = 1;
		pipelineLayoutInfo2.pSetLayouts = &tempAABB;

		if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo2, nullptr, &this->constructAABBPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline layout!");

		VkDescriptorSetLayout tempRaytrace = this->raytraceDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo pipelineLayoutInfo6{};
		pipelineLayoutInfo6.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo6.setLayoutCount = 1;
		pipelineLayoutInfo6.pSetLayouts = &tempRaytrace;

		if (vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo6, nullptr, &this->raytracePipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline layout!");
	}
	auto Raytracer::createComputePipeline() -> void {
		{
			ComputePipelineConfigInfo pipelineConfig{};
			ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
			pipelineConfig.pipelineLayout = this->modelToWorldPipelineLayout;
			this->modelToWorldPipeline = std::make_unique<ComputePipeline>(
				this->device,
				"shaders/compiled/ModelSpaceToWorldSpace.comp.spv",
				pipelineConfig
			);
		}

		{
			ComputePipelineConfigInfo pipelineConfig{};
			ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
			pipelineConfig.pipelineLayout = this->enclosingAABBPipelineLayout;
			this->enclosingAABBPipeline = std::make_unique<ComputePipeline>(
				this->device,
				"shaders/compiled/GetEnclosingAABB.comp.spv",
				pipelineConfig
			);
		}

		{
			ComputePipelineConfigInfo pipelineConfig{};
			ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
			pipelineConfig.pipelineLayout = this->generateMortonCodePipelineLayout;
			this->generateMortonCodePipeline = std::make_unique<ComputePipeline>(
				this->device,
				"shaders/compiled/GenerateMortonCodesOfPrimitives.comp.spv",
				pipelineConfig
			);
		}

		{
			ComputePipelineConfigInfo pipelineConfig{};
			ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
			pipelineConfig.pipelineLayout = this->radixSortPipelineLayout;
			this->radixSortComputePipeline = std::make_unique<ComputePipeline>(
				this->device,
				"shaders/compiled/RadixSortSimple.comp.spv",
				pipelineConfig
			);
		}

		{
			ComputePipelineConfigInfo pipelineConfig{};
			ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
			pipelineConfig.pipelineLayout = this->constructHLBVHPipelineLayout;
			this->constructHLBVHComputePipeline = std::make_unique<ComputePipeline>(
				this->device,
				"shaders/compiled/ConstructHLBVH.comp.spv",
				pipelineConfig
			);
		}

		{
			ComputePipelineConfigInfo pipelineConfig{};
			ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
			pipelineConfig.pipelineLayout = this->constructAABBPipelineLayout;
			this->constructAABBPipeline = std::make_unique<ComputePipeline>(
				this->device,
				"shaders/compiled/ConstructAABBsOfInternalNodes.comp.spv",
				pipelineConfig
			);
		}

		{
			ComputePipelineConfigInfo pipelineConfig{};
			ComputePipeline::defaultPipelineConfigInfo(pipelineConfig);
			pipelineConfig.pipelineLayout = this->raytracePipelineLayout;
			this->raytracePipeline = std::make_unique<ComputePipeline>(
				this->device,
				"shaders/compiled/raytraceBVH.comp.spv",
				pipelineConfig
			);
		}
	}

	auto Raytracer::createComputeImage() -> void {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = this->swapChain->getSwapChainExtent().width;
		imageInfo.extent.height = this->swapChain->getSwapChainExtent().height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT; //VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // using sfloat to additively store multiple ray colors in same location
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0;

		this->device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			this->computeImage,
			this->computeImageMemory
		);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = this->computeImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(this->device.device(), &viewInfo, nullptr, &this->computeImageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		if (vkCreateSampler(this->device.device(), &samplerInfo, nullptr, &this->fragmentShaderImageSampler) != VK_SUCCESS) {
			std::runtime_error("failed to create image sampler");
		}
	}

	auto Raytracer::createGraphicsPipeline() -> void {
		VkDescriptorSetLayout tempGraphics = this->graphicsDescriptorSetLayout->getDescriptorSetLayout();
		VkPipelineLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.setLayoutCount = 1;
		layoutCreateInfo.pSetLayouts = &tempGraphics;
		if (vkCreatePipelineLayout(this->device.device(), &layoutCreateInfo, nullptr, &this->graphicsPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline layout!");
		GraphicsPipelineConfigInfo pipelineConfig{};
		GraphicsPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = this->swapChain->getRenderPass();
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		pipelineConfig.pipelineLayout = this->graphicsPipelineLayout;
		this->graphicsPipeline = std::make_unique<GraphicsPipeline>(
			this->device,
			"shaders/compiled/SingleTriangleFullScreen.vert.spv",
			"shaders/compiled/SingleTriangleFullScreen.frag.spv",
			pipelineConfig
		);
	}

	auto Raytracer::createScene() -> void {
		std::vector<f32> init(20, 0);

		// Create Staging buffer to upload data to gpu
		Buffer stagingBuffer(
			this->device,
			sizeof(f32),
			scratchSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // will transfer from
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)init.data());

		this->scratchBuffer = std::make_unique<Buffer>(
			this->device,
			sizeof(f32),
			scratchSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // is ssbo and will transfer into
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		this->device.copyBuffer(
			this->device.graphicsQueue(),
			this->device.getGraphicsCommandPool(),
			stagingBuffer.getBuffer(),
			this->scratchBuffer->getBuffer(),
			sizeof(f32) * scratchSize
		);

		this->scene = std::make_unique<RaytraceScene>(this->device);
		cornellBoxScene(this->scene);

		const u32 primCount = this->scene->getTriangleCount() + this->scene->getSphereCount();
		
		this->enclosingAABBBuffer = std::make_unique<Buffer>(
			this->device,
			sizeof(EnclosingAABBBufferObject),
			1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // is ssbo and will transfer into
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		this->mortonPrimitiveBuffer1 = std::make_unique<Buffer>(
			this->device,
			sizeof(SceneTypes::GPU::MortonPrimitive),
			primCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // is ssbo and will transfer into
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		this->mortonPrimitiveBuffer2 = std::make_unique<Buffer>(
			this->device,
			sizeof(SceneTypes::GPU::MortonPrimitive),
			primCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // is ssbo and will transfer into
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		this->HLBVHNodesBuffer = std::make_unique<Buffer>(
			this->device,
			sizeof(SceneTypes::GPU::BVHNode),
			primCount + primCount - 1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // is ssbo and will transfer into
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		this->HLBVHConstructionInfoBuffer = std::make_unique<Buffer>(
			this->device,
			sizeof(u32) * 2,
			primCount + primCount - 1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // is ssbo and will transfer into
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
	}

	auto Raytracer::createUniformBuffers() -> void {
		VkDeviceSize bufferSizeRT = sizeof(RaytracerBVHRenderer::RaytracingUniformBufferObject);
		this->rayUniformBuffer = std::make_unique<Buffer>(
			this->device,
			bufferSizeRT,
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //TODO remove transfer src. there only for debugging
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		this->rayUniformBuffer->map();

		VkDeviceSize bufferSizeFrag = sizeof(RaytracerBVHRenderer::FragmentUniformBufferObject);
		this->fragUniformBuffer = std::make_unique<Buffer>(
			this->device,
			bufferSizeFrag,
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		this->fragUniformBuffer->map();
	}

	auto Raytracer::createComputeDescriptorPool() -> void {
		this->modelToWorldDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3)
			.build();
		this->enclosingAABBDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4)
			.build();
		this->generateMortonCodeDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5)
			.build();
		this->radixSortDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)
			.build();
		this->constructHLBVHDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5)
			.build();
		this->constructAABBDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2)
			.build();
		this->raytraceDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6)
			.build();
	}

	auto Raytracer::createComputeDescriptorSets() -> void {
		this->modelToWorldDescriptorSets.resize(1);
		this->constructAABBDescriptorSets.resize(1);
		this->generateMortonCodeDescriptorSets.resize(1);
		this->radixSortDescriptorSets.resize(1);
		this->constructHLBVHDescriptorSets.resize(1);
		this->raytraceDescriptorSets.resize(1);
		this->enclosingAABBDescriptorSets.resize(1);
		auto uboBufferInfo = this->rayUniformBuffer->descriptorInfo();
		auto ssboEnclosingAABBBufferInfo = this->enclosingAABBBuffer->descriptorInfo();
		auto ssboModelBufferInfo = this->scene->getModelBuffer()->descriptorInfo();
		auto ssboTriangleBufferInfo = this->scene->getTriangleBuffer()->descriptorInfo();
		auto ssboSphereBufferInfo = this->scene->getSphereBuffer()->descriptorInfo();
		auto ssboMaterialBufferInfo = this->scene->getMaterialBuffer()->descriptorInfo();
		auto ssboScratchBufferInfo = this->scratchBuffer->descriptorInfo();
		auto ssboMortonBufferInfo1 = this->mortonPrimitiveBuffer1->descriptorInfo();
		auto ssboMortonBufferInfo2 = this->mortonPrimitiveBuffer2->descriptorInfo();
		auto ssboBVHNodeInfo = this->HLBVHNodesBuffer->descriptorInfo();
		auto ssboBVHConstructionInfoInfo = this->HLBVHConstructionInfoBuffer->descriptorInfo();

		VkDescriptorImageInfo descImageInfo{};
		descImageInfo.sampler = nullptr;
		descImageInfo.imageView = this->computeImageView;
		descImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		DescriptorWriter(*this->modelToWorldDescriptorSetLayout, *this->modelToWorldDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeBuffer(1, &ssboModelBufferInfo)
			.writeBuffer(2, &ssboTriangleBufferInfo)
			.writeBuffer(3, &ssboSphereBufferInfo)
			.build(this->modelToWorldDescriptorSets[0]);
		DescriptorWriter(*this->enclosingAABBDescriptorSetLayout, *this->enclosingAABBDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeBuffer(1, &ssboEnclosingAABBBufferInfo)
			.writeBuffer(2, &ssboTriangleBufferInfo)
			.writeBuffer(3, &ssboSphereBufferInfo)
			.writeBuffer(4, &ssboScratchBufferInfo)
			.build(this->enclosingAABBDescriptorSets[0]);
		DescriptorWriter(*this->generateMortonCodeDescriptorSetLayout, *this->generateMortonCodeDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeBuffer(1, &ssboEnclosingAABBBufferInfo)
			.writeBuffer(2, &ssboTriangleBufferInfo)
			.writeBuffer(3, &ssboSphereBufferInfo)
			.writeBuffer(4, &ssboMortonBufferInfo1)
			.writeBuffer(5, &ssboScratchBufferInfo)
			.build(this->generateMortonCodeDescriptorSets[0]);
		DescriptorWriter(*this->radixSortDescriptorSetLayout, *this->radixSortDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeBuffer(1, &ssboMortonBufferInfo1)
			.writeBuffer(2, &ssboMortonBufferInfo2)
			.build(this->radixSortDescriptorSets[0]);
		DescriptorWriter(*this->constructHLBVHDescriptorSetLayout, *this->constructHLBVHDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeBuffer(1, &ssboTriangleBufferInfo)
			.writeBuffer(2, &ssboSphereBufferInfo)
			.writeBuffer(3, &ssboMortonBufferInfo1)
			.writeBuffer(4, &ssboBVHNodeInfo)
			.writeBuffer(5, &ssboBVHConstructionInfoInfo)
			.build(this->constructHLBVHDescriptorSets[0]);
		DescriptorWriter(*this->constructAABBDescriptorSetLayout, *this->constructAABBDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeBuffer(1, &ssboBVHNodeInfo)
			.writeBuffer(2, &ssboBVHConstructionInfoInfo)
			.build(this->constructAABBDescriptorSets[0]);
		DescriptorWriter(*this->raytraceDescriptorSetLayout, *this->raytraceDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeImage(1, &descImageInfo)
			.writeBuffer(2, &ssboTriangleBufferInfo)
			.writeBuffer(3, &ssboSphereBufferInfo)
			.writeBuffer(4, &ssboMaterialBufferInfo)
			.writeBuffer(5, &ssboBVHNodeInfo)
			.writeBuffer(6, &ssboScratchBufferInfo)
			.build(this->raytraceDescriptorSets[0]);
	}
	auto Raytracer::createGraphicsDescriptorPool() -> void {
		this->graphicsDescriptorPool = DescriptorPool::Builder(this->device)
			.setMaxSets(1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1)
			.build();
	}
	auto Raytracer::createGraphicsDescriptorSets() -> void {
		this->graphicsDescriptorSets.resize(1);
		auto uboBufferInfo = this->fragUniformBuffer->descriptorInfo();

		VkDescriptorImageInfo descImageInfo{};
		descImageInfo.sampler = this->fragmentShaderImageSampler;
		descImageInfo.imageView = this->computeImageView;
		descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		DescriptorWriter(*this->graphicsDescriptorSetLayout, *this->graphicsDescriptorPool)
			.writeBuffer(0, &uboBufferInfo)
			.writeImage(1, &descImageInfo)
			.build(this->graphicsDescriptorSets[0]);
	}

	auto Raytracer::createComputeCommandBuffers() -> void {
		this->computeS1CommandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
		this->computeS2CommandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = this->device.getComputeCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<u32>(SwapChain::MAX_FRAMES_IN_FLIGHT);

		if (vkAllocateCommandBuffers(this->device.device(), &allocInfo, this->computeS1CommandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate compute Command Buffers!");

		if (vkAllocateCommandBuffers(this->device.device(), &allocInfo, this->computeS2CommandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate compute Command Buffers!");
	}
	auto Raytracer::createGraphicsCommandBuffers() -> void {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = this->device.getGraphicsCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(this->device.device(), &allocInfo, &this->graphicsCommandBuffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate compute Command Buffers!");
	}
	auto Raytracer::recordComputeS1CommandBuffer(VkCommandBuffer commandBuffer, u32 currImageIndex) -> void {
		static bool firstRun = true;
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;
		range.baseArrayLayer = 0;
		range.baseMipLevel = 0;

		VkImageMemoryBarrier reset;
		reset.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		reset.pNext = nullptr;
		reset.srcAccessMask = 0;
		reset.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		reset.oldLayout = firstRun ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		reset.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		reset.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		reset.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		reset.image = this->computeImage;
		reset.subresourceRange = range;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // src stage
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // dst stage
			0, // no dependencies
			0, nullptr, // no memory barriers
			0, nullptr, // no buffer memory barriers
			1, &reset // 1 imageMemoryBarrier
		);

		VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
		vkCmdClearColorImage(
			commandBuffer, this->computeImage,
			VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range
		);

		this->modelToWorldPipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			this->modelToWorldPipelineLayout,
			0,
			1,
			&this->modelToWorldDescriptorSets[0],
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer, ((this->scene->getTriangleCount() + this->scene->getSphereCount()) / 32) + 1, 1, 1);

		VkMemoryBarrier waitForWorldSpaceTransformation;
		waitForWorldSpaceTransformation.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		waitForWorldSpaceTransformation.pNext = nullptr;
		waitForWorldSpaceTransformation.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		waitForWorldSpaceTransformation.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

		std::array<VkBufferMemoryBarrier, 2> barriers;
		barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[0].pNext = nullptr;
		barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[0].buffer = this->scene->getTriangleBuffer()->getBuffer();
		barriers[0].offset = 0;
		barriers[0].size = VK_WHOLE_SIZE;

		barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[1].pNext = nullptr;
		barriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[1].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].buffer = this->scene->getSphereBuffer()->getBuffer();
		barriers[1].offset = 0;
		barriers[1].size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0,
			nullptr,
			2,
			barriers.data(),
			0,
			nullptr
		);

		this->enclosingAABBPipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			this->enclosingAABBPipelineLayout,
			0,
			1,
			&this->enclosingAABBDescriptorSets[0],
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer, 1, 1, 1);

		VkBufferMemoryBarrier enclosingAABBBarrier;
		enclosingAABBBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		enclosingAABBBarrier.pNext = nullptr;
		enclosingAABBBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		enclosingAABBBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		enclosingAABBBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		enclosingAABBBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		enclosingAABBBarrier.buffer = this->mortonPrimitiveBuffer1->getBuffer();
		enclosingAABBBarrier.offset = 0;
		enclosingAABBBarrier.size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0,
			nullptr,
			1,
			&enclosingAABBBarrier,
			0,
			nullptr
		);

		this->generateMortonCodePipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			this->generateMortonCodePipelineLayout,
			0,
			1,
			&this->generateMortonCodeDescriptorSets[0],
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer, ((this->scene->getTriangleCount() + this->scene->getSphereCount()) / 32) + 1, 1, 1);

		VkBufferMemoryBarrier mortonCodeBarrier;
		mortonCodeBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		mortonCodeBarrier.pNext = nullptr;
		mortonCodeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		mortonCodeBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		mortonCodeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		mortonCodeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		mortonCodeBarrier.buffer = this->mortonPrimitiveBuffer1->getBuffer();
		mortonCodeBarrier.offset = 0;
		mortonCodeBarrier.size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0,
			nullptr,
			1,
			&mortonCodeBarrier,
			0,
			nullptr
		);

		this->radixSortComputePipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			this->radixSortPipelineLayout,
			0,
			1,
			&this->radixSortDescriptorSets[0],
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer, 1, 1, 1); // only 1 workgroup
		// see RadixSortSimple for reason

		VkBufferMemoryBarrier sortingBarrier;
		sortingBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		sortingBarrier.pNext = nullptr;
		sortingBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		sortingBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		sortingBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		sortingBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		sortingBarrier.buffer = this->mortonPrimitiveBuffer1->getBuffer();
		sortingBarrier.offset = 0;
		sortingBarrier.size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0,
			nullptr,
			1,
			&sortingBarrier,
			0,
			nullptr
		);

		this->constructHLBVHComputePipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			this->constructHLBVHPipelineLayout,
			0,
			1,
			&this->constructHLBVHDescriptorSets[0],
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer, ((this->scene->getTriangleCount() + this->scene->getSphereCount()) / 256) + 1, 1, 1);

		VkBufferMemoryBarrier bvhBarrier;
		bvhBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bvhBarrier.pNext = nullptr;
		bvhBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bvhBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bvhBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bvhBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bvhBarrier.buffer = this->mortonPrimitiveBuffer1->getBuffer();
		bvhBarrier.offset = 0;
		bvhBarrier.size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0,
			nullptr,
			1,
			&bvhBarrier,
			0,
			nullptr
		);

		this->constructAABBPipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			this->constructAABBPipelineLayout,
			0,
			1,
			&this->constructAABBDescriptorSets[0],
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer, ((this->scene->getTriangleCount() + this->scene->getSphereCount()) / 32) + 1, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
		firstRun = false;
	}
	auto Raytracer::recordComputeS2CommandBuffer(VkCommandBuffer commandBuffer, u32 currImageIndex) -> void {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;
		range.baseArrayLayer = 0;
		range.baseMipLevel = 0;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		VkExtent2D imageSize = this->swapChain->getSwapChainExtent();
		this->raytracePipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			this->raytracePipelineLayout,
			0,
			1,
			&this->raytraceDescriptorSets[0],
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer, (imageSize.width / 32) + 1, (imageSize.height / 32) + 1, 1); // assume once cause doesn't make much sense to go below that
		// and need barrier between each dispatch but not before or after all
		for (auto i = 1; i < this->scene->getRaysPerPixel(); i++) {
			VkImageMemoryBarrier waitForLastTraceSet; // wait for each previous set of rays to get done before starting the next
			waitForLastTraceSet.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			waitForLastTraceSet.pNext = nullptr;
			waitForLastTraceSet.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			waitForLastTraceSet.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			waitForLastTraceSet.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			waitForLastTraceSet.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			waitForLastTraceSet.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			waitForLastTraceSet.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			waitForLastTraceSet.image = this->computeImage;
			waitForLastTraceSet.subresourceRange = range;
			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // src stage
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // dst stage
				0, // no dependencies
				0, nullptr, // no memory barriers
				0, nullptr, // no buffer memory barriers
				1, &waitForLastTraceSet // 1 imageMemoryBarrier
			);

			vkCmdDispatch(commandBuffer, (imageSize.width / 32) + 1, (imageSize.height / 32) + 1, 1);
		}

		// TODO sync2: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#dispatch-writes-into-a-storage-image-draw-samples-that-image-in-a-fragment-shader
		VkImageMemoryBarrier computeToPresent;
		computeToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		computeToPresent.pNext = nullptr;
		computeToPresent.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		computeToPresent.dstAccessMask = 0;
		computeToPresent.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		computeToPresent.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		computeToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		computeToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		computeToPresent.image = this->computeImage;
		computeToPresent.subresourceRange = range;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // src stage
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dst stage
			0, // no dependencies
			0, nullptr, // no memory barriers
			0, nullptr, // no buffer memory barriers
			1, &computeToPresent // 1 imageMemoryBarrier
		);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	auto Raytracer::recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, u32 currImageIndex) -> void {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording graphics command buffer!");
		}

		this->graphicsPipeline->bind(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			this->graphicsPipelineLayout,
			0,
			1,
			&this->graphicsDescriptorSets[0],
			0,
			nullptr
		);

		this->beginRenderPass(commandBuffer, currImageIndex);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0); // vertex shader just needs 3 vertex indexes to make space for fragment shader, so lie and say there's 3 to do even though there's none sent

		this->endRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	auto Raytracer::beginRenderPass(VkCommandBuffer commandBuffer, u32 currImageIndex) -> void {
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = this->swapChain->getRenderPass();
		renderPassInfo.framebuffer = this->swapChain->getFrameBuffer(currImageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = this->swapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(
			commandBuffer,
			&renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE
		);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(this->swapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(this->swapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, this->swapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}
	auto Raytracer::endRenderPass(VkCommandBuffer commandBuffer) -> void {
		vkCmdEndRenderPass(commandBuffer);
	}
	auto Raytracer::getNextImageIndex() -> u32 {
		u32 nextImageIndex;
		if (
			this->swapChain->acquireNextImage(&nextImageIndex) != VK_SUCCESS
		)
			throw std::runtime_error("failed to acquire next image!");
		return nextImageIndex;
	}
};
