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

#include "imgui_internal.h"

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



    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();// Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_Window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkInstance;
    init_info.PhysicalDevice = vkPhysicalDevice;
    init_info.Device = vkDevice;
    init_info.QueueFamily = queueFamily;
    init_info.Queue = vkGraphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_DescriptorPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.Allocator = nullptr;
    
    // Enable dynamic rendering
    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.RenderPass = VK_NULL_HANDLE; // Ignored with dynamic rendering
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Setup dynamic rendering pipeline info
    VkFormat swapchainFormat = m_SwapChain->getImageFormat();
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {};
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainFormat;
    
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
    // Wait for device to be idle before cleanup
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

    CleanupVulkan();
}

void ImGuiLayer::OnUpdate(float deltaTime)
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
	
    // Create main dockspace
    static bool opt_fullscreen_persistant = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    bool opt_fullscreen = opt_fullscreen_persistant;
    ImGuiViewport* viewport;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        // Create settings
        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
        {
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
            ImGuiID dock_id_left = 0;
            ImGuiID dock_id_main = dockspace_id;
            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.20f, &dock_id_left, &dock_id_main);
            ImGuiID dock_id_left_top = 0;
            ImGuiID dock_id_left_bottom = 0;
            ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.50f, &dock_id_left_top, &dock_id_left_bottom);
            ImGui::DockBuilderDockWindow("Game", dock_id_main);
            ImGui::DockBuilderDockWindow("Properties", dock_id_left_top);
            ImGui::DockBuilderDockWindow("Scene", dock_id_left_bottom);
            ImGui::DockBuilderFinish(dockspace_id);
        }

        }

    // Menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Options"))
        {
            if (ImGui::MenuItem("Demo Window", nullptr, m_ShowDemoWindow))
                m_ShowDemoWindow = !m_ShowDemoWindow;
            if (ImGui::MenuItem("Another Window", nullptr, m_ShowAnotherWindow))
                m_ShowAnotherWindow = !m_ShowAnotherWindow;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
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
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        // Record ImGui draw data directly to the provided command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
    }
}



void ImGuiLayer::CleanupVulkan()
{
    if (m_Device && m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device->get(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}



void ImGuiLayer::OnWindowResize(int width, int height)
{
    // ImGui doesn't need to do anything special on resize since it's using the main renderer's swapchain
    // The display size will be updated automatically in OnUpdate via ImGui_ImplGlfw_NewFrame()
}