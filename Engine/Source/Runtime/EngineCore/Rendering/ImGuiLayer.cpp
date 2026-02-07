#include "ImGuiLayer.h"
#include "Renderer.h"
#include "Runtime/EngineCore/Window.h"
#include "Runtime/EngineCore/RHI/Instance.h"
#include "Runtime/EngineCore/RHI/PhysicalDevice.h"
#include "Runtime/EngineCore/RHI/Device.h"
#include "Runtime/EngineCore/RHI/Surface.h"
#include "Runtime/EngineCore/RHI/SwapChain.h"
#include "Runtime/EngineCore/RHI/RenderPass.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
#endif

ImGuiLayer::ImGuiLayer()
    : Layer("ImGuiLayer")
{
}

ImGuiLayer::~ImGuiLayer()
{
    OnDetach();
}

void ImGuiLayer::SetRendererContext(Renderer* renderer)
{
    m_Renderer = renderer;
}

void ImGuiLayer::check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;
    if (err < 0)
        std::abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix;
    std::cerr << "[vulkan] Debug report from ObjectType: " << objectType << "\nMessage: " << pMessage << "\n\n" << std::endl;
    return VK_FALSE;
}
#endif

void ImGuiLayer::OnAttach()
{
    if (!m_Renderer)
    {
        throw std::runtime_error("ImGuiLayer requires a valid Renderer context");
    }

// Get RHI components from renderer using our wrapper classes
    m_Instance = m_Renderer->GetInstance();
    m_PhysicalDevice = m_Renderer->GetPhysicalDevice();
    m_Device = m_Renderer->GetDevice();
    m_Surface = m_Renderer->GetSurface();
    m_SwapChain = m_Renderer->GetSwapChain();
    m_RenderPass = m_Renderer->GetRenderPass();
    m_Window = m_Renderer->GetWindow()->getGLFWwindow();

    // Get Vulkan objects from RHI classes
    VkInstance vkInstance = m_Instance->getInstance();
    VkPhysicalDevice vkPhysicalDevice = m_PhysicalDevice->get();
    VkDevice vkDevice = m_Device->get();
    VkQueue vkGraphicsQueue = m_Device->getGraphicsQueue();
    uint32_t queueFamily = m_PhysicalDevice->getQueueFamilyIndices().graphicsFamily.value();
    VkSurfaceKHR vkSurface = m_Surface->get();

    // Load Vulkan functions (needed when IMGUI_IMPL_VULKAN_NO_PROTOTYPES is defined)
    ImGui_ImplVulkan_LoadFunctions(0, [](const char* function_name, void* user_data) {
        return vkGetInstanceProcAddr(*static_cast<VkInstance*>(user_data), function_name);
    }, &vkInstance);

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
        pool_info.pPoolSizes = pool_sizes;
        VkResult err = vkCreateDescriptorPool(vkDevice, &pool_info, nullptr, &m_DescriptorPool);
        check_vk_result(err);
    }

    // Setup Vulkan Window
    SetupVulkanWindow();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_Window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkInstance;
    init_info.PhysicalDevice = vkPhysicalDevice;
    init_info.Device = vkDevice;
    init_info.QueueFamily = queueFamily;
    init_info.Queue = vkGraphicsQueue;
    init_info.PipelineCache = m_PipelineCache;
    init_info.DescriptorPool = m_DescriptorPool;
    init_info.MinImageCount = m_MinImageCount;
    init_info.ImageCount = m_MainWindowData.ImageCount;
    init_info.Allocator = nullptr;
    init_info.PipelineInfoMain.RenderPass = m_MainWindowData.RenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = [](VkResult err) { 
        if (err != VK_SUCCESS) {
            std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;
            if (err < 0) std::abort();
        }
    };
    
    bool init_success = ImGui_ImplVulkan_Init(&init_info);
    if (!init_success) {
        throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
    }


}

void ImGuiLayer::OnDetach()
{
    if (m_Device) {
        VkResult err = vkDeviceWaitIdle(m_Device->get());
        check_vk_result(err);
    }
    
    // Only shutdown if the backend was properly initialized
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    CleanupVulkanWindow();
    CleanupVulkan();
}

void ImGuiLayer::OnUpdate(float deltaTime)
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window
    if (m_ShowDemoWindow)
        ImGui::ShowDemoWindow(&m_ShowDemoWindow);

    // 2. Show a simple window that we create ourselves
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");
        ImGui::Checkbox("Demo Window", &m_ShowDemoWindow);
        ImGui::Checkbox("Another Window", &m_ShowAnotherWindow);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&m_ClearColor);

        if (ImGui::Button("Button"))
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window
    if (m_ShowAnotherWindow)
    {
        ImGui::Begin("Another Window", &m_ShowAnotherWindow);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            m_ShowAnotherWindow = false;
        ImGui::End();
    }
}

void ImGuiLayer::OnRender(VkCommandBuffer commandBuffer)
{
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        m_MainWindowData.ClearValue.color.float32[0] = m_ClearColor.x * m_ClearColor.w;
        m_MainWindowData.ClearValue.color.float32[1] = m_ClearColor.y * m_ClearColor.w;
        m_MainWindowData.ClearValue.color.float32[2] = m_ClearColor.z * m_ClearColor.w;
        m_MainWindowData.ClearValue.color.float32[3] = m_ClearColor.w;
        FrameRender(draw_data);
        FramePresent();
    }
}

void ImGuiLayer::SetupVulkanWindow()
{
    // Get Vulkan objects from RHI classes
    VkPhysicalDevice vkPhysicalDevice = m_PhysicalDevice->get();
    VkSurfaceKHR vkSurface = m_Surface->get();
    uint32_t queueFamily = m_PhysicalDevice->getQueueFamilyIndices().graphicsFamily.value();

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, queueFamily, vkSurface, &res);
    if (res != VK_TRUE)
    {
        std::cerr << "Error no WSI support on physical device 0" << std::endl;
        std::exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    m_MainWindowData.Surface = vkSurface;
    m_MainWindowData.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(vkPhysicalDevice, m_MainWindowData.Surface, requestSurfaceImageFormat, sizeof(requestSurfaceImageFormat)/sizeof(requestSurfaceImageFormat[0]), requestSurfaceColorSpace);

    // Select Present Mode
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
    m_MainWindowData.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(vkPhysicalDevice, m_MainWindowData.Surface, present_modes, sizeof(present_modes)/sizeof(present_modes[0]));

    // Get window size
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);
    m_MainWindowData.Width = width;
    m_MainWindowData.Height = height;

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(m_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(m_Instance->getInstance(), vkPhysicalDevice, m_Device->get(), &m_MainWindowData, queueFamily, nullptr, width, height, m_MinImageCount, 0);
}

void ImGuiLayer::CleanupVulkan()
{
    if (m_Device && m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device->get(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void ImGuiLayer::CleanupVulkanWindow()
{
    if (m_Instance && m_Device) {
        ImGui_ImplVulkanH_DestroyWindow(m_Instance->getInstance(), m_Device->get(), &m_MainWindowData, nullptr);
    }
}

void ImGuiLayer::FrameRender(ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore  = m_MainWindowData.FrameSemaphores[m_MainWindowData.SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = m_MainWindowData.FrameSemaphores[m_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(m_Device->get(), m_MainWindowData.Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &m_MainWindowData.FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        m_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &m_MainWindowData.Frames[m_MainWindowData.FrameIndex];
    {
        err = vkWaitForFences(m_Device->get(), 1, &fd->Fence, VK_TRUE, UINT64_MAX);
        check_vk_result(err);

        err = vkResetFences(m_Device->get(), 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(m_Device->get(), fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = m_MainWindowData.RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = m_MainWindowData.Width;
        info.renderArea.extent.height = m_MainWindowData.Height;
        info.clearValueCount = 1;
        info.pClearValues = &m_MainWindowData.ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(m_Device->getGraphicsQueue(), 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

void ImGuiLayer::FramePresent()
{
    if (m_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = m_MainWindowData.FrameSemaphores[m_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &m_MainWindowData.Swapchain;
    info.pImageIndices = &m_MainWindowData.FrameIndex;
    VkResult err = vkQueuePresentKHR(m_Device->getPresentQueue(), &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        m_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);
    m_MainWindowData.SemaphoreIndex = (m_MainWindowData.SemaphoreIndex + 1) % m_MainWindowData.SemaphoreCount;
}