#include "Renderer.h"
#include "InstanceBuilder.h"
#include "PhysicalDeviceBuilder.h"
#include "DeviceBuilder.h"
#include "SwapChainBuilder.h"
#include "GraphicsPipelineBuilder.h"
#include <stdexcept>
#include <iostream>
#include <glm/glm.hpp>

//TODO: Will move to vulkan specific RHI types
const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Simple triangle vertices
const std::vector<Vertex> triangleVertices = {
    {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

Renderer::Renderer(Window* window)
{
    std::cout << "Renderer created." << std::endl;
}

Renderer::~Renderer() 
{
    Shutdown();
    std::cout << "Destroying Renderer." << std::endl;
}

void Renderer::Initialize(Window* window)
{
    m_Window = window;
    InitializeVulkan();
    m_Initialized = true;
}

void Renderer::Shutdown()
{
    if (m_Initialized)
    {
        if (m_Device) {
            vkDeviceWaitIdle(m_Device->get());
        }
        CleanupSwapChain();
        m_Initialized = false;
    }
}

void Renderer::Render()
{
    if (m_Initialized)
    {
        drawFrame();
    }
}

void Renderer::OnWindowResize()
{
    if (m_Initialized)
    {
        RecreateSwapChain();
    }
}

void Renderer::createTriangleVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(triangleVertices[0]) * triangleVertices.size();

    Buffer stagingBuffer(
        m_Device->getAllocator(),
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    void* data = stagingBuffer.map();
    memcpy(data, triangleVertices.data(), static_cast<size_t>(bufferSize));
    stagingBuffer.unmap();
    stagingBuffer.flush();

    m_TriangleVertexBuffer = std::make_unique<Buffer>(
        m_Device->getAllocator(),
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    stagingBuffer.copyTo(m_CommandPool.get(), m_Device->getGraphicsQueue(), m_TriangleVertexBuffer.get());
}

void Renderer::InitializeVulkan()
{
    // Create Instance using the Instance class which handles its own creation
    m_Instance = std::make_unique<Instance>();
    
    // Create Surface
    m_Surface = std::make_unique<Surface>(m_Instance->getInstance(), m_Window->getGLFWwindow());
    
// Pick and create Physical Device with dynamic rendering support
    VkPhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;
    
    m_PhysicalDevice = std::unique_ptr<PhysicalDevice>(PhysicalDeviceBuilder()
        .setInstance(m_Instance->getInstance())
        .setSurface(m_Surface->get())
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .setVulkan13Features(vulkan13Features)
        .build());
    
// Create Logical Device using builder
    const auto& queueIndices = m_PhysicalDevice->getQueueFamilyIndices();
    if (!queueIndices.graphicsFamily.has_value() || !queueIndices.presentFamily.has_value()) {
        throw std::runtime_error("Queue family indices are not available!");
    }
    
m_Device = std::unique_ptr<Device>(DeviceBuilder()
        .setPhysicalDevice(m_PhysicalDevice->get())
        .setInstance(m_Instance->getInstance())
        .setQueueFamilyIndices(queueIndices)
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .setVulkan13Features(vulkan13Features)
        .build());
    
// Create Swap Chain using builder
    m_SwapChain = std::unique_ptr<SwapChain>(SwapChainBuilder()
        .setDevice(m_Device->get())
        .setPhysicalDevice(m_PhysicalDevice->get())
        .setSurface(m_Surface->get())
        .setWidth(m_Window->getWidth())
        .setHeight(m_Window->getHeight())
        .setGraphicsFamilyIndex(queueIndices.graphicsFamily.value())
        .setPresentFamilyIndex(queueIndices.presentFamily.value())
        .build());
    
    // Store swap chain properties
    m_SwapChainExtent = m_SwapChain->getExtent();
    m_SwapChainFormat = m_SwapChain->getImageFormat();
    
// Create Graphics Pipeline using builder with vertex input
    m_GraphicsPipeline = std::unique_ptr<GraphicsPipeline>(GraphicsPipelineBuilder()
        .setDevice(m_Device->get())
        .setSwapChainExtent(m_SwapChainExtent)
        .setShaderPaths("E:/CreationArtEngine/Engine/Binaries/Shaders/ShaderType_Vertex.spv", 
                        "E:/CreationArtEngine/Engine/Binaries/Shaders/ShaderTypes_Fragment.spv")
        .setColorFormats({ m_SwapChainFormat })
        .setAttachmentCount(1)
        .setVertexInputBindingDescription(Vertex::getBindingDescription())
        .setVertexInputAttributeDescriptions(Vertex::getAttributeDescriptions())
        .build());
    
// Create Command Pool
    m_CommandPool = std::make_unique<CommandPool>(m_Device->get(), m_PhysicalDevice->getQueueFamilyIndices().graphicsFamily.value());
    
    // Create triangle vertex buffer
    createTriangleVertexBuffer();
    
    // Create Command Buffers and Sync Objects
    CreateCommandBuffers();
    CreateSyncObjects();
}

void Renderer::CreateCommandBuffers()
{
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool->get();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(m_Device->get(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Renderer::CreateSyncObjects()
{
    m_PresentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(m_SwapChain->getImages().size());
    m_DrawFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create present complete semaphores
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_Device->get(), &semaphoreInfo, nullptr, &m_PresentCompleteSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_Device->get(), &fenceInfo, nullptr, &m_DrawFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    // Create render finished semaphores
    for (size_t i = 0; i < m_SwapChain->getImages().size(); i++) {
        if (vkCreateSemaphore(m_Device->get(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render finished semaphore!");
        }
    }
}

void Renderer::drawFrame()
{
    VkResult fenceResult = vkWaitForFences(m_Device->get(), 1, &m_DrawFences[m_FrameIndex], VK_TRUE, UINT64_MAX);
    if (fenceResult != VK_SUCCESS)
    {
        throw std::runtime_error("failed to wait for fence!");
    }

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Device->get(), m_SwapChain->get(), UINT64_MAX, m_PresentCompleteSemaphores[m_FrameIndex], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_Device->get(), 1, &m_DrawFences[m_FrameIndex]);

    vkResetCommandBuffer(m_CommandBuffers[m_FrameIndex], 0);
    recordCommandBuffer(imageIndex);

    VkPipelineStageFlags waitDestinationStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_PresentCompleteSemaphores[m_FrameIndex];
    submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[m_FrameIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[imageIndex];

    if (vkQueueSubmit(m_Device->getGraphicsQueue(), 1, &submitInfo, m_DrawFences[m_FrameIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapchain = m_SwapChain->get();
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(m_Device->getPresentQueue(), &presentInfo);
    
    // Check for suboptimal or out of date results
    if (result == VK_SUBOPTIMAL_KHR || m_Window->IsResized())
    {
        m_Window->SetResizedFalse();
        RecreateSwapChain();
    }
    else if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_Window->SetResizedFalse();
        RecreateSwapChain();
        return;  // Skip frame index increment since we're recreating
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::CleanupSwapChain()
{
    if (m_Device) {
        vkDeviceWaitIdle(m_Device->get());
    }
    
    // Clean up synchronization objects
    for (auto semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device->get(), semaphore, nullptr);
    }
    m_RenderFinishedSemaphores.clear();
    
    for (auto semaphore : m_PresentCompleteSemaphores) {
        vkDestroySemaphore(m_Device->get(), semaphore, nullptr);
    }
    m_PresentCompleteSemaphores.clear();
    
    for (auto fence : m_DrawFences) {
        vkDestroyFence(m_Device->get(), fence, nullptr);
    }
    m_DrawFences.clear();
    
    // Clean up command buffers
    if (!m_CommandBuffers.empty() && m_CommandPool) {
        vkFreeCommandBuffers(m_Device->get(), m_CommandPool->get(), 
                           static_cast<uint32_t>(m_CommandBuffers.size()), 
                           m_CommandBuffers.data());
    }
    m_CommandBuffers.clear();
    
    // Clean up swap chain
    m_SwapChain.reset();
}

void Renderer::RecreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window->getGLFWwindow(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window->getGLFWwindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_Device->get());

    CleanupSwapChain();
    
// Recreate Swap Chain using builder
    const auto& queueIndices = m_PhysicalDevice->getQueueFamilyIndices();
    m_SwapChain = std::unique_ptr<SwapChain>(SwapChainBuilder()
        .setDevice(m_Device->get())
        .setPhysicalDevice(m_PhysicalDevice->get())
        .setSurface(m_Surface->get())
        .setWidth(width)
        .setHeight(height)
        .setGraphicsFamilyIndex(queueIndices.graphicsFamily.value())
        .setPresentFamilyIndex(queueIndices.presentFamily.value())
        .build());
    
    // Store swap chain properties
    m_SwapChainExtent = m_SwapChain->getExtent();
    m_SwapChainFormat = m_SwapChain->getImageFormat();
    
    // Recreate Graphics Pipeline using builder
    m_GraphicsPipeline = std::unique_ptr<GraphicsPipeline>(GraphicsPipelineBuilder()
        .setDevice(m_Device->get())
        .setSwapChainExtent(m_SwapChainExtent)
        .setShaderPaths("E:/CreationArtEngine/Engine/Binaries/Shaders/ShaderType_Vertex.spv", 
                        "E:/CreationArtEngine/Engine/Binaries/Shaders/ShaderTypes_Fragment.spv")
        .setColorFormats({ m_SwapChainFormat })
        .setAttachmentCount(1)
        .build());
    
    // Recreate Command Buffers and Sync Objects
    CreateCommandBuffers();
    CreateSyncObjects();
}

void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
    VkCommandBuffer commandBuffer = m_CommandBuffers[m_FrameIndex];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        imageIndex,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        0,                                                        // srcAccessMask (no need to wait for previous operations)
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                     // dstAccessMask
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,            // srcStage
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT             // dstStage
    );

VkClearValue clearColor = {};
    clearColor.color.float32[0] = 0.0f; // Dark red background
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 1.0f;

    VkRenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.imageView = m_SwapChain->getImageViews()[imageIndex];
    attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue = clearColor;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = m_SwapChainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &attachmentInfo;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->get());

    // Bind vertex buffer
    VkBuffer vertexBuffers[] = { m_TriangleVertexBuffer->get() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_SwapChainExtent.width);
    viewport.height = static_cast<float>(m_SwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_SwapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRendering(commandBuffer);

    // After rendering, transition the swapchain image to PRESENT_SRC
    transition_image_layout(
        imageIndex,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                        // srcAccessMask
        0,                                                           // dstAccessMask
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,               // srcStage
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT                         // dstStage
    );

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Renderer::transition_image_layout(
    uint32_t imageIndex,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkAccessFlags src_access_mask,
    VkAccessFlags dst_access_mask,
    VkPipelineStageFlags src_stage_mask,
    VkPipelineStageFlags dst_stage_mask)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_SwapChain->getImages()[imageIndex];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstAccessMask = dst_access_mask;

    vkCmdPipelineBarrier(
        m_CommandBuffers[m_FrameIndex],
        src_stage_mask, dst_stage_mask,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

std::vector<const char*> Renderer::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}