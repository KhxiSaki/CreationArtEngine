#include "ImGuiManager.h"
#include "Device.h"
#include <iostream>

ImGuiManager::ImGuiManager()
{
}

ImGuiManager::~ImGuiManager()
{
    Shutdown();
}

void ImGuiManager::Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, 
                             uint32_t queueFamily, VkQueue queue, VkRenderPass renderPass, 
                             uint32_t minImageCount, uint32_t imageCount, 
                             GLFWwindow* window, VkPipelineCache pipelineCache)
{
    if (m_Initialized)
        return;

    m_Instance = instance;
    m_PhysicalDevice = physicalDevice;
    m_Device = device;
    m_QueueFamily = queueFamily;
    m_Queue = queue;
    m_RenderPass = renderPass;
    m_MinImageCount = minImageCount;
    m_ImageCount = imageCount;
    m_Window = window;
    m_PipelineCache = pipelineCache;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    m_Context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Create descriptor pool
    CreateDescriptorPool();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_Window, true);
    
    // Load Vulkan functions (needed when IMGUI_IMPL_VULKAN_NO_PROTOTYPES is defined)
    ImGui_ImplVulkan_LoadFunctions(0, [](const char* function_name, void* user_data) {
        return vkGetInstanceProcAddr(*static_cast<VkInstance*>(user_data), function_name);
    }, &m_Instance);
    
    // Create a simple render pass for ImGui
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM; // Common ImGui format
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_ImGuiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ImGui render pass!");
    }

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Instance;
    init_info.PhysicalDevice = m_PhysicalDevice;
    init_info.Device = m_Device;
    init_info.QueueFamily = m_QueueFamily;
    init_info.Queue = m_Queue;
    init_info.PipelineCache = m_PipelineCache;
    init_info.DescriptorPool = m_DescriptorPool;
    init_info.MinImageCount = m_MinImageCount;
    init_info.ImageCount = m_ImageCount;
    init_info.Allocator = nullptr;
    // Use the ImGui render pass we just created
    init_info.PipelineInfoMain.RenderPass = m_ImGuiRenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = [](VkResult err) {
        if (err == VK_SUCCESS) return;
        std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;
        if (err < 0) abort();
    };

    ImGui_ImplVulkan_Init(&init_info);

// Font upload will be handled by renderer

    m_Initialized = true;
    std::cout << "ImGui Manager initialized successfully!" << std::endl;
}

void ImGuiManager::UploadFonts(VkCommandBuffer commandBuffer)
{
    if (!m_Initialized)
        return;

    // Font upload is now automatic - ImGui_ImplVulkan_CreateFontsTexture() is called automatically on first NewFrame()
}

void ImGuiManager::NewFrame()
{
    if (!m_Initialized)
        return;

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Demo UI - you can remove this and create your own UI
    if (m_ShowDemoWindow)
        ImGui::ShowDemoWindow(&m_ShowDemoWindow);

    // Simple window
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

    if (m_ShowAnotherWindow)
    {
        ImGui::Begin("Another Window", &m_ShowAnotherWindow);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            m_ShowAnotherWindow = false;
        ImGui::End();
    }
}

void ImGuiManager::Render(VkCommandBuffer commandBuffer)
{
    if (!m_Initialized)
        return;

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;
    
    // Record dear imgui primitives into command buffer
    // Use default pipeline parameter (VK_NULL_HANDLE)
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImGuiManager::Shutdown()
{
    if (!m_Initialized)
        return;

    if (m_Device)
        vkDeviceWaitIdle(m_Device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    
    if (m_Context)
    {
        ImGui::DestroyContext(m_Context);
        m_Context = nullptr;
    }

    if (m_DescriptorPool)
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
    
    if (m_ImGuiRenderPass)
    {
        vkDestroyRenderPass(m_Device, m_ImGuiRenderPass, nullptr);
        m_ImGuiRenderPass = VK_NULL_HANDLE;
    }

    m_Initialized = false;
    std::cout << "ImGui Manager shutdown complete!" << std::endl;
}

void ImGuiManager::CreateDescriptorPool()
{
    // Create a larger descriptor pool for ImGui (copied from ImGui demo)
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkResult result = vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_DescriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ImGui descriptor pool!");
    }
}