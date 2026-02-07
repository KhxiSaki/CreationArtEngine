#pragma once

#include <vulkan/vulkan.h>

#include "LayerStack.h"
#include "../Window.h"
#include "Runtime/EngineCore/RHI/CommandPool.h"
#include "Runtime/EngineCore/RHI/Device.h"
#include "Runtime/EngineCore/RHI/Instance.h"
#include "Runtime/EngineCore/RHI/IRHIContext.h"
#include "Runtime/EngineCore/RHI/PhysicalDevice.h"
#include "Runtime/EngineCore/RHI/RenderPass.h"
#include "Runtime/EngineCore/RHI/Surface.h"
#include "Runtime/EngineCore/RHI/SwapChain.h"

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
    
    void UpdateLayers(float deltaTime);

    bool IsInitialized() const override { return m_Initialized; }
    
    LayerStack* GetLayerStack() const { return m_LayerStack.get(); }

// RHI component getters for ImGuiLayer
    Instance* GetInstance() const { return m_Instance.get(); }
    PhysicalDevice* GetPhysicalDevice() const { return m_PhysicalDevice.get(); }
    Device* GetDevice() const { return m_Device.get(); }
    Surface* GetSurface() const { return m_Surface.get(); }
    SwapChain* GetSwapChain() const { return m_SwapChain.get(); }
    RenderPass* GetRenderPass() const { return m_RenderPass.get(); }
    Window* GetWindow() const;
    uint32_t GetQueueFamilyIndex() const { return m_QueueIndex; }

private:
    void InitializeVulkan();
    void Cleanup();


void CreateCommandBuffers();
    void CreateFramebuffers();
    void CreateSyncObjects();
    void drawFrame();
void CleanupSwapChain();
    void CleanupSwapChainResources();
    void RecreateSwapChain();
    void recordCommandBuffer(uint32_t imageIndex);
void transition_image_layout(uint32_t imageIndex, VkImageLayout old_layout, VkImageLayout new_layout,
        VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
        VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask);
    

    // Helper functions
    std::vector<const char*> getRequiredExtensions();

// Window reference
    Window* m_Window = nullptr;

// RHI components
    std::unique_ptr<Instance> m_Instance;
    std::unique_ptr<Surface> m_Surface;
    std::unique_ptr<PhysicalDevice> m_PhysicalDevice;
    std::unique_ptr<Device> m_Device;
    std::unique_ptr<SwapChain> m_SwapChain;
std::unique_ptr<RenderPass> m_RenderPass;
    std::vector<VkFramebuffer> m_Framebuffers;
    std::unique_ptr<CommandPool> m_CommandPool;
    

    
// Layer System
    std::unique_ptr<LayerStack> m_LayerStack;
    
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
