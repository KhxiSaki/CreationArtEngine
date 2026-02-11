#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan.h>
#include "Runtime/EngineCore/Layer/Layer.h"

class Renderer;
class Device;
class Instance;
class PhysicalDevice;
class Surface;
class SwapChain;
class RenderPass;

class ImGuiLayer : public Layer
{
public:
    ImGuiLayer();
    ~ImGuiLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(VkCommandBuffer commandBuffer) override;

    void SetRendererContext(Renderer* renderer);
private:
    void SetupVulkan();
    void CleanupVulkan();
    void check_vk_result(VkResult err);

    // RHI components from renderer
    Renderer* m_Renderer = nullptr;
    Instance* m_Instance = nullptr;
    PhysicalDevice* m_PhysicalDevice = nullptr;
    Device* m_Device = nullptr;
    Surface* m_Surface = nullptr;
    SwapChain* m_SwapChain = nullptr;
    RenderPass* m_RenderPass = nullptr;
    
    // Vulkan objects (created by ImGui)
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

    // ImGui state
    bool m_ShowDemoWindow = true;
    bool m_ShowAnotherWindow = false;
    ImVec4 m_ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // GLFW window
    GLFWwindow* m_Window = nullptr;
};
