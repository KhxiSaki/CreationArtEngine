#pragma once

#include <vulkan/vulkan.h>
#include "../Window.h"
#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "SwapChain.h"
#include "GraphicsPipeline.h"
#include "CommandPool.h"
#include "DeviceBuilder.h"
#include "SwapChainBuilder.h"
#include "GraphicsPipelineBuilder.h"
#include "PhysicalDeviceBuilder.h"
#include "InstanceBuilder.h"
#include <memory>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "Runtime/EngineCore/Window.h"
#include "Runtime/EngineCore/RHI/IRHIContext.h"
#include "Runtime/EngineCore/RHI/Model.h"
#include "Runtime/EngineCore/RHI/Buffer.h"
#include "Runtime/EngineCore/RHI/ImGuiManager.h"

class Renderer : public IRHIContext
{
public:
    Renderer(Window* window);
    ~Renderer();

    // IRHIContext interface
    void Initialize(Window* window) override;
    void Shutdown() override;
    void Render() override;
    void OnWindowResize() override;

    bool IsInitialized() const override { return m_Initialized; }

private:
    void InitializeVulkan();
    void Cleanup();


    void CreateCommandBuffers();
    void CreateSyncObjects();
    void drawFrame();
    void CleanupSwapChain();
    void RecreateSwapChain();
    void recordCommandBuffer(uint32_t imageIndex);
void transition_image_layout(uint32_t imageIndex, VkImageLayout old_layout, VkImageLayout new_layout,
        VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
        VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask);
    void createTriangleVertexBuffer();

    // Helper functions
    std::vector<const char*> getRequiredExtensions();

// RHI components
    std::unique_ptr<Instance> m_Instance;
    std::unique_ptr<Surface> m_Surface;
    std::unique_ptr<PhysicalDevice> m_PhysicalDevice;
    std::unique_ptr<Device> m_Device;
    std::unique_ptr<SwapChain> m_SwapChain;
    std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;
    std::unique_ptr<CommandPool> m_CommandPool;
    
// Simple triangle vertex buffer
    std::unique_ptr<Buffer> m_TriangleVertexBuffer;
    
    // ImGui Manager
    std::unique_ptr<ImGuiManager> m_ImGuiManager;
    
    // Command buffers and synchronization
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkSemaphore> m_PresentCompleteSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_DrawFences;
    
    // State
    uint32_t m_QueueIndex = ~0;
    uint32_t m_FrameIndex = 0;
    VkExtent2D m_SwapChainExtent;
    VkFormat m_SwapChainFormat;

    std::vector<const char*> m_RequiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};