#pragma once

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <fstream>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Runtime/EngineCore/Window.h"
#include "Runtime/EngineCore/RHI/IRHIContext.h"

class VulkanContext : public IRHIContext
{
public:
    VulkanContext();
    ~VulkanContext();

    // IRHIContext interface
    void Initialize(Window* window) override;
    void Shutdown() override;
    void Render() override;
    void OnWindowResize() override;
    
    bool IsInitialized() const override { return m_Initialized; }
    RHIType GetType() const override { return RHIType::Vulkan; }
    const char* GetName() const override { return "Vulkan"; }

    // Vulkan-specific static methods
    static bool IsAvailable();

private:
    void InitializeVulkan();
    void Cleanup();

    // Vulkan initialization methods
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateGraphicsPipeline();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void drawFrame();
    void CleanupSwapChain();
    void RecreateSwapChain();
    void recordCommandBuffer(uint32_t imageIndex);
    void transition_image_layout(uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask);

    // Helper functions
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& code) const;
    std::vector<char> ReadFile(const std::string& filename) const;
    std::vector<const char*> getRequiredExtensions();

    static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, 
        vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*);

    // Vulkan components
    vk::raii::Context  m_VulkanContext;
    vk::raii::Instance VulkanInstance = nullptr;
    vk::raii::DebugUtilsMessengerEXT VulkanDebugMessenger = nullptr;
    vk::raii::PhysicalDevice VulkanPhysicalDevice = nullptr;
    vk::raii::Device VulkanLogicalDevice = nullptr;
    vk::raii::Queue VulkanGraphicsQueue = nullptr;
    vk::raii::SurfaceKHR VulkanSurface = nullptr;
    vk::raii::SwapchainKHR VulkanSwapChain = nullptr;
    std::vector<vk::Image> VulkanSwapChainImages;
    vk::SurfaceFormatKHR VulkanSwapChainSurfaceFormat;
    vk::Extent2D VulkanSwapChainExtent;
    std::vector<vk::raii::ImageView> VulkanSwapChainImageViews;
    uint32_t queueIndex = ~0;
    uint32_t frameIndex = 0;

    vk::raii::PipelineLayout VulkanPipelineLayout = nullptr;
    vk::raii::Pipeline VulkanGraphicsPipeline = nullptr;
    vk::raii::CommandPool VulkanCommandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> VulkanCommandBuffers;
    std::vector<vk::raii::Semaphore> VulkanPresentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> VulkanRenderFinishedSemaphores;
    std::vector<vk::raii::Fence> VulkanDrawFences;

    std::vector<const char*> VulkanRequiredDeviceExtension = { vk::KHRSwapchainExtensionName };

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};