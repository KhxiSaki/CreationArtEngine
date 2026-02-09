#include "Renderer.h"
#include <stdexcept>
#include <iostream>
#include <glm/glm.hpp>

#include "Runtime/EngineCore/RHI/DeviceBuilder.h"
#include "Runtime/EngineCore/RHI/PhysicalDeviceBuilder.h"
#include "Runtime/EngineCore/RHI/SwapChainBuilder.h"
#include "Runtime/EngineCore/Rendering/Layer.h"
#include "Runtime/EngineCore/Rendering/ImGuiLayer.h"

//TODO: Will move to vulkan specific RHI types
const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
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
    if (m_LayerStack) {
        for (auto layer : *m_LayerStack) {
            layer->OnAttach();
        }
    }
    m_Initialized = true;
}

void Renderer::Shutdown()
{
    if (m_Initialized)
    {
        // Wait for device to be idle before cleanup
        if (m_Device) {
            vkDeviceWaitIdle(m_Device->get());
        }
        
        // Shutdown layers first - this will clean up ImGui's Vulkan resources
        if (m_LayerStack) {
            for (auto layer : *m_LayerStack) {
                layer->OnDetach();
            }
            m_LayerStack.reset();
        }
        
        // Clean up swapchain and other resources
        CleanupSwapChainResources();
        
        // Clean up Vulkan objects in correct order
        if (m_CommandPool) {
            m_CommandPool.reset();
        }
        
        if (m_SwapChain) {
            m_SwapChain.reset();
        }
        
        if (m_RenderPass) {
            m_RenderPass.reset();
        }
        
        if (m_Device) {
            m_Device.reset();
        }
        
        if (m_Surface) {
            m_Surface.reset();
        }
        
        if (m_PhysicalDevice) {
            m_PhysicalDevice.reset();
        }
        
        if (m_Instance) {
            m_Instance.reset();
        }
        
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

void Renderer::UpdateLayers(float deltaTime)
{
    if (m_Initialized)
    {
        // Update all layers
        for (auto layer : *m_LayerStack)
        {
            if (layer->IsEnabled())
            {
                layer->OnUpdate(deltaTime);
            }
        }
    }
}

void Renderer::OnWindowResize()
{
    if (m_Initialized)
    {
        RecreateSwapChain();
    }
}



void Renderer::InitializeVulkan()
{
    // Create Instance using the Instance class which handles its own creation
    m_Instance = std::make_unique<Instance>();
    
    // Create Surface
    m_Surface = std::make_unique<Surface>(m_Instance->getInstance(), m_Window->getGLFWwindow());
    
// Pick and create Physical Device
    m_PhysicalDevice = std::unique_ptr<PhysicalDevice>(PhysicalDeviceBuilder()
        .setInstance(m_Instance->getInstance())
        .setSurface(m_Surface->get())
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .build());
    
// Create Logical Device using builder
    const auto& queueIndices = m_PhysicalDevice->getQueueFamilyIndices();
    if (!queueIndices.graphicsFamily.has_value() || !queueIndices.presentFamily.has_value()) {
        throw std::runtime_error("Queue family indices are not available!");
    }
    
// Enable dynamic rendering feature
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;
    
m_Device = std::unique_ptr<Device>(DeviceBuilder()
        .setPhysicalDevice(m_PhysicalDevice->get())
        .setInstance(m_Instance->getInstance())
        .setQueueFamilyIndices(queueIndices)
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .addRequiredExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
        .setVulkan12Features(vulkan12Features)
        .setVulkan13Features(vulkan13Features)
        .build());
    
    // Load dynamic rendering function pointers
    vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(m_Device->get(), "vkCmdBeginRenderingKHR");
    vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(m_Device->get(), "vkCmdEndRenderingKHR");
    
    if (!vkCmdBeginRenderingKHR || !vkCmdEndRenderingKHR) {
        throw std::runtime_error("Failed to load dynamic rendering function pointers!");
    }
    
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
    
// Create Render Pass
    m_RenderPass = std::make_unique<RenderPass>(m_Device->get(), m_SwapChainFormat);
    
// Create Command Pool
    m_CommandPool = std::make_unique<CommandPool>(m_Device->get(), m_PhysicalDevice->getQueueFamilyIndices().graphicsFamily.value());
    
    CreateCommandBuffers();
    CreateSyncObjects();
    
// Initialize Layer Stack
    m_LayerStack = std::make_unique<LayerStack>();
    
    // Create and add ImGui Layer
    auto imguiLayer = new ImGuiLayer();
    imguiLayer->SetRendererContext(this);
    m_LayerStack->PushLayer(imguiLayer);
    
    std::cout << "Layer system initialized with ImGui layer" << std::endl;
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

// Update layers (this will call ImGui NewFrame)
    UpdateLayers(0.016f); // Assume 60 FPS for now

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

void Renderer::CleanupSwapChainResources()
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
    
    // Clean up render pass
    m_RenderPass.reset();
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

// Store old swapchain handle before cleanup
    VkSwapchainKHR oldSwapchain = m_SwapChain ? m_SwapChain->get() : VK_NULL_HANDLE;

    // Clean up only the resources that depend on the swapchain, but keep the swapchain itself
    CleanupSwapChainResources();
    
    // Release the old swapchain without destroying it (it will be destroyed by vkCreateSwapchainKHR)
    if (m_SwapChain)
    {
        m_SwapChain->release();
        m_SwapChain.reset();
    }
    
// Recreate Swap Chain using builder
    const auto& queueIndices = m_PhysicalDevice->getQueueFamilyIndices();
    SwapChainBuilder builder;
    builder.setDevice(m_Device->get())
           .setPhysicalDevice(m_PhysicalDevice->get())
           .setSurface(m_Surface->get())
           .setWidth(width)
           .setHeight(height)
           .setGraphicsFamilyIndex(queueIndices.graphicsFamily.value())
           .setPresentFamilyIndex(queueIndices.presentFamily.value())
           .setOldSwapchain(oldSwapchain);
    
m_SwapChain = std::unique_ptr<SwapChain>(builder.build());
    
    // The old swapchain will be destroyed automatically when m_SwapChain is reset
    // No need to manually destroy it here
    
    // Store swap chain properties
    m_SwapChainExtent = m_SwapChain->getExtent();
    m_SwapChainFormat = m_SwapChain->getImageFormat();

// Recreate Render Pass
    m_RenderPass = std::make_unique<RenderPass>(m_Device->get(), m_SwapChainFormat);
    
    // Notify ImGui layer of the resize
    if (m_LayerStack) {
        for (auto layer : *m_LayerStack) {
            auto imguiLayer = dynamic_cast<ImGuiLayer*>(layer);
            if (imguiLayer) {
                imguiLayer->OnWindowResize(m_SwapChainExtent.width, m_SwapChainExtent.height);
            }
        }
    }
    
// Recreate Command Buffers, Framebuffers and Sync Objects
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

// Transition the swapchain image to color attachment layout
    transition_image_layout(imageIndex, 
        VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_NONE_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    // Set up dynamic rendering
    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = m_SwapChain->getImageViews()[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color.float32[0] = 0.0f; // Dark red background
    colorAttachment.clearValue.color.float32[1] = 0.0f;
    colorAttachment.clearValue.color.float32[2] = 0.0f;
    colorAttachment.clearValue.color.float32[3] = 1.0f;

    VkRenderingInfoKHR renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.renderArea.extent = m_SwapChainExtent;
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;

    this->vkCmdBeginRenderingKHR(commandBuffer, &renderInfo);

    // Set viewport and scissor
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

    // Render all layers
    for (auto layer : *m_LayerStack)
    {
        if (layer->IsEnabled())
        {
            layer->OnRender(commandBuffer);
        }
    }

this->vkCmdEndRenderingKHR(commandBuffer);

    // Transition the swapchain image to present layout
    transition_image_layout(imageIndex,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_NONE_KHR,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

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

Window* Renderer::GetWindow() const
{
    return m_Window;
}