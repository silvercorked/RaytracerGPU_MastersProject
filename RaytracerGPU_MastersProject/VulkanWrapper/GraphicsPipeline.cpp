
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

module VulkanWrap:GraphicsPipeline;

GraphicsPipeline::GraphicsPipeline(
	Device& device,
	const std::string& vertFilepath,
	const std::string& fragFilepath,
	const GraphicsPipelineConfigInfo& config
) :
	Pipeline{ device, 2 } // 2 shader modules
{
	this->createGraphicsPipeline(vertFilepath, fragFilepath, config);
}
GraphicsPipeline::~GraphicsPipeline() {}

auto GraphicsPipeline::createGraphicsPipeline(
	const std::string& vertFilepath,
	const std::string& fragFilepath,
	const GraphicsPipelineConfigInfo& config
) -> void {
	auto vertCode = this->readFile(vertFilepath);
	auto fragCode = this->readFile(fragFilepath);

	assert(
		config.pipelineLayout != VK_NULL_HANDLE &&
		"Cannot create graphics GraphicsPipeline:: no pipelineLayout provided in configInfo"
	);
	assert(
		config.renderPass != VK_NULL_HANDLE &&
		"Cannot create graphics GraphicsPipeline:: no renderPass provided in configInfo"
	);
	this->createShaderModule(vertCode, &this->shaderModules[0]);
	this->createShaderModule(fragCode, &this->shaderModules[1]);

	VkPipelineShaderStageCreateInfo shaderStages[2];

	// setup vertex shader stage
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;							// for vertex shader
	shaderStages[0].module = this->getVertShaderModule();						// shader module
	shaderStages[0].pName = "main";												// name of entry function in shader
	shaderStages[0].flags = 0;													// unused
	shaderStages[0].pNext = nullptr;											// for customizing shader functionality, unused								
	shaderStages[0].pSpecializationInfo = nullptr;								// for customizing shader functionality, unused	

	// setup fragment shader stage
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;						// for fragment shader
	shaderStages[1].module = this->getFragShaderModule();						// shader module
	shaderStages[1].pName = "main";												// name of entry function in shader
	shaderStages[1].flags = 0;													// unused
	shaderStages[1].pNext = nullptr;											// for customizing shader functionality, unused	
	shaderStages[1].pSpecializationInfo = nullptr;								// for customizing shader functionality, unused	

	auto& bindingDescriptions = config.bindingDescriptions;
	auto& attributeDescriptions = config.attributeDescriptions;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if (bindingDescriptions.size() == 0 && attributeDescriptions.size() == 0) { // assume trying to make fullscreen single triangle
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
	}
	else {
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	}

	// create pipeline object and connect to config
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &config.inputAssemblyInfo;
	pipelineInfo.pViewportState = &config.viewportInfo;
	pipelineInfo.pRasterizationState = &config.rasterizationInfo;
	pipelineInfo.pMultisampleState = &config.multisampleInfo;
	pipelineInfo.pColorBlendState = &config.colorBlendInfo;
	pipelineInfo.pDepthStencilState = &config.depthStencilInfo;
	pipelineInfo.pDynamicState = &config.dynamicStateInfo;

	pipelineInfo.layout = config.pipelineLayout;
	pipelineInfo.renderPass = config.renderPass;
	pipelineInfo.subpass = config.subpass;

	pipelineInfo.basePipelineIndex = -1;				// used for performance, allows creation of pipeline by derivation of
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// an existing pipeline on the gpu

	if (
		vkCreateGraphicsPipelines(
			this->device.device(),
			VK_NULL_HANDLE,				// pipeline cache, can have better performance
			1,							// single pipeline
			&pipelineInfo,				// 
			nullptr,					// no allocation callbacks
			&this->pipeline				// handle for this graphics pipeline
		) != VK_SUCCESS
		) {
		throw std::runtime_error("failed to create graphics pipeline");
	}
}
auto GraphicsPipeline::bind(VkCommandBuffer commandBuffer) -> void {
	vkCmdBindPipeline(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,		// VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_BIND_POINT_COMPUTE, VK_PIPELINE_BIND_POINT_RAY_TRACING
		this->pipeline
	);
}
auto GraphicsPipeline::defaultPipelineConfigInfo(GraphicsPipelineConfigInfo& configInfo) -> void {
	configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;						// group every 3 verticies into triangle (triangle strip is a notable option)
	configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;										// allows inclusion of seperator when using triangle strip to break strip

	configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	configInfo.viewportInfo.viewportCount = 1;												// can have multiple viewports & scissors
	configInfo.viewportInfo.pViewports = nullptr;
	configInfo.viewportInfo.scissorCount = 1;												// but only have one for now
	configInfo.viewportInfo.pScissors = nullptr;

	configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;			// if enabled, clamps all Z values to 0-1 (ie, something with -z will be visible)
	configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;	// discards all primitives before rasterization, but we want those
	configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;	// what to draw? just corners, edges? we want fill
	configInfo.rasterizationInfo.lineWidth = 1.0f;
	configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;			// which face of triangle to ignore? front? back? ignore none? usually want to do back-face culling for performance
	configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;	// winding order (ie declaring normal dir, as 3 points don't tells which face is forward)
	configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;			// 
	configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
	configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
	configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

	configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;					// at each fragment, how many times to sample?
	configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// set higher than 1 => MSAA (multi-sample anti-aliasing)?
	configInfo.multisampleInfo.minSampleShading = 1.0f;
	configInfo.multisampleInfo.pSampleMask = nullptr;
	configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

	configInfo.colorBlendAttachement.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	configInfo.colorBlendAttachement.blendEnable = VK_FALSE; // if color already in buffer, how do they mix? false => overwrite, need for transparancy
	configInfo.colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	configInfo.colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	configInfo.colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD; // additive blending
	configInfo.colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	configInfo.colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	configInfo.colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD; // additive alpha blending

	configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	configInfo.colorBlendInfo.attachmentCount = 1;
	configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachement;
	configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

	configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;		// store depth values
	configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;	// how to compare 2 depth values. pick smallest in this case => closest visible, behind overwritten in buffer
	configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;		// avoids drawing stuff entirely behind closer stuff, but also doesn't draw the portions of things that are behind stuff (ie, a cloud behind a mountain, part of cloud might be visible, only because the mountain doesn't cover it)
	configInfo.depthStencilInfo.minDepthBounds = 0.0f;
	configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
	configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.front = {};
	configInfo.depthStencilInfo.back = {};

	// configures pipeline to expend a dynamic viewport and dynamic scissor later
	configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
	configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
	configInfo.dynamicStateInfo.flags = 0;

	//configInfo.bindingDescriptions = Model::Vertex::getBindingDescriptions();
	//configInfo.attributeDescriptions = Model::Vertex::getAttributeDescriptions();
}
auto GraphicsPipeline::enableAlphaBlending(GraphicsPipelineConfigInfo& configInfo) -> void {
	configInfo.colorBlendAttachement.blendEnable = VK_TRUE;
	// src is output from frag shader
	// dst is whatever exists in the color attachment (such as something already rendered)
	configInfo.colorBlendAttachement.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// color.rgb = (src.alpha x scr.rgb) + ((1 - src.alpha) x dst.rgb)
	// this works if rendering from farthest to closest (order irrelavent for solids, but needed for transparents)
	// there are cases where this doesn't work. See order-independnt transparency (https://en.wikipedia.org/wiki/Order-independent_transparency) for ideas for improvement here
	configInfo.colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	configInfo.colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	configInfo.colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD; // additive blending
	configInfo.colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;	// last three aren't super important for now
	configInfo.colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	configInfo.colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD; // additive alpha blending
}
