#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "backends/imgui_impl_vulkan.h"

class Window;
class Device;
class SwapChain;
class CommandPool;
class Buffer;

class ImGuiManager
{
public:
    ImGuiManager();
    ~ImGuiManager();

    void Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, 
                   uint32_t queueFamily, VkQueue queue, VkRenderPass renderPass, 
                   uint32_t minImageCount, uint32_t imageCount, 
                   GLFWwindow* window, VkPipelineCache pipelineCache = VK_NULL_HANDLE);
    
    void CreateRenderPass(VkFormat format);
    
    void NewFrame();
    void Render(VkCommandBuffer commandBuffer);
    void UploadFonts(VkCommandBuffer commandBuffer);
    void Shutdown();

    bool IsInitialized() const { return m_Initialized; }

private:
    void CreateDescriptorPool();
    void SetupRenderPass();

private:
    bool m_Initialized = false;
    
    // Vulkan objects
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    uint32_t m_QueueFamily = 0;
    VkQueue m_Queue = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    GLFWwindow* m_Window = nullptr;
    
    // ImGui properties
    uint32_t m_MinImageCount = 2;
    uint32_t m_ImageCount = 0;
    
    // Render pass for ImGui
    VkRenderPass m_ImGuiRenderPass = VK_NULL_HANDLE;
    
    // ImGui context
    ImGuiContext* m_Context = nullptr;
    
    // Demo data
    bool m_ShowDemoWindow = true;
    bool m_ShowAnotherWindow = false;
    ImVec4 m_ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};