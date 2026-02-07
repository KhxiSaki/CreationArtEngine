#include "GraphicsPipeline.h"
#include <iostream>

GraphicsPipeline::GraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline)
    : m_Device(device), m_PipelineLayout(pipelineLayout), m_GraphicsPipeline(graphicsPipeline) 
{
	std::cout << "GraphicsPipeline created." << std::endl;
}

GraphicsPipeline::~GraphicsPipeline() 
{
    if (m_GraphicsPipeline != VK_NULL_HANDLE) 
    {
        vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) 
    {
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    }
	std::cout << "GraphicsPipeline destroyed." << std::endl;
}

VkPipelineLayout GraphicsPipeline::getPipelineLayout() const 
{
    return m_PipelineLayout;
}

VkPipeline GraphicsPipeline::get() const 
{
    return m_GraphicsPipeline;
}
