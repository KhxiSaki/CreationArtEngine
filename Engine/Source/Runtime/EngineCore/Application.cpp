#include "Runtime/EngineCore/Application.h"

//TODO: Will probably move this to a window class in the future
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

//TODO: Will move to vulkan specific RHI types
const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

Application::Application()
{
}

Application::~Application()
{
}

void Application::Run()
{
    InitializeWindow();
    InitializeVulkan();
    MainLoop();
    Cleanup();
}

void Application::InitializeWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    Window = glfwCreateWindow(WIDTH, HEIGHT, "RealityEngine", nullptr, nullptr);
}

void Application::InitializeVulkan()
{
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void Application::CreateInstance()
{
    vk::ApplicationInfo appInfo;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine",
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion14;

    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (enableValidationLayers)
    {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = VulkanContext.enumerateInstanceLayerProperties();
    for (auto const& requiredLayer : requiredLayers)
    {
        if (std::ranges::none_of(layerProperties,
            [requiredLayer](auto const& layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; }))
        {
            throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
        }
    }
    // Get the required extensions.
    auto requiredExtensions = getRequiredExtensions();

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = VulkanContext.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions)
    {
        if (std::ranges::none_of(extensionProperties,
            [requiredExtension](auto const& extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
        {
            throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
        }
    }


    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
	
	VulkanInstance = vk::raii::Instance(VulkanContext, createInfo);
}

void Application::SetupDebugMessenger()
{
    if (!enableValidationLayers)
    {
        return;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT;
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
	debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
	debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;
    
    VulkanDebugMessenger = VulkanInstance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void Application::CreateSurface()
{
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*VulkanInstance, Window, nullptr, &_surface) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    VulkanSurface = vk::raii::SurfaceKHR(VulkanInstance, _surface);
}

void Application::PickPhysicalDevice()
{
    std::vector<vk::raii::PhysicalDevice> devices = VulkanInstance.enumeratePhysicalDevices();
    const auto                            devIter = std::ranges::find_if(
        devices,
        [&](auto const& device) {
            // Check if the device supports the Vulkan 1.3 API version
            bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

            // Check if any of the queue families support graphics operations
            auto queueFamilies = device.getQueueFamilyProperties();
            bool supportsGraphics =
                std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

            // Check if all required device extensions are available
            auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
            bool supportsAllRequiredExtensions =
                std::ranges::all_of(VulkanRequiredDeviceExtension,
                    [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
                        return std::ranges::any_of(availableDeviceExtensions,
                            [requiredDeviceExtension](auto const& availableDeviceExtension) { return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
                    });

            auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceVulkan11Features,
                vk::PhysicalDeviceVulkan13Features,
                vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters && features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 && 
                features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
        });
    if (devIter != devices.end())
    {
        VulkanPhysicalDevice = *devIter;
    }
    else
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Application::CreateLogicalDevice()
{
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = VulkanPhysicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            VulkanPhysicalDevice.getSurfaceSupportKHR(qfpIndex, *VulkanSurface))
        {
            // found a queue family that supports both graphics and present
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }
    // query for Vulkan 1.3 features
    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.dynamicRendering = true;
    vulkan13Features.synchronization2 = true;

    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{};
    extendedDynamicStateFeatures.extendedDynamicState = true;

	vk::PhysicalDeviceVulkan11Features vulkan11Features{};
    vulkan11Features.shaderDrawParameters = true;

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain(
        {},                                   // vk::PhysicalDeviceFeatures2
        vulkan11Features,                       // vk::PhysicalDeviceVulkan11Features
        vulkan13Features,                     // vk::PhysicalDeviceVulkan13Features
        extendedDynamicStateFeatures          // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    );

    // create a Device
    float                     queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.queueFamilyIndex = queueIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanRequiredDeviceExtension.size());
    deviceCreateInfo.ppEnabledExtensionNames = VulkanRequiredDeviceExtension.data();

    VulkanLogicalDevice = vk::raii::Device(VulkanPhysicalDevice, deviceCreateInfo);
    VulkanGraphicsQueue = vk::raii::Queue(VulkanLogicalDevice, queueIndex, 0);
}

void Application::CreateSwapChain()
{
    auto surfaceCapabilities = VulkanPhysicalDevice.getSurfaceCapabilitiesKHR(*VulkanSurface);
    VulkanSwapChainExtent = chooseSwapExtent(surfaceCapabilities);
    VulkanSwapChainSurfaceFormat = chooseSwapSurfaceFormat(VulkanPhysicalDevice.getSurfaceFormatsKHR(*VulkanSurface));

    vk::SwapchainCreateInfoKHR swapChainCreateInfo;
    swapChainCreateInfo.surface = *VulkanSurface;
    swapChainCreateInfo.minImageCount = chooseSwapMinImageCount(surfaceCapabilities);
    swapChainCreateInfo.imageFormat = VulkanSwapChainSurfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = VulkanSwapChainSurfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = VulkanSwapChainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainCreateInfo.presentMode = chooseSwapPresentMode(VulkanPhysicalDevice.getSurfacePresentModesKHR(*VulkanSurface));
	swapChainCreateInfo.clipped = true;

    VulkanSwapChain = vk::raii::SwapchainKHR(VulkanLogicalDevice, swapChainCreateInfo);
    VulkanSwapChainImages = VulkanSwapChain.getImages();
}

void Application::CreateImageViews()
{
    assert(VulkanSwapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D,
	imageViewCreateInfo.format = VulkanSwapChainSurfaceFormat.format,
	imageViewCreateInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    for (auto& image : VulkanSwapChainImages)
    {
        imageViewCreateInfo.image = image;
        VulkanSwapChainImageViews.emplace_back(VulkanLogicalDevice, imageViewCreateInfo);
    }
}

void Application::CreateGraphicsPipeline()
{
    vk::raii::ShaderModule VertexShaderModule = CreateShaderModule(ReadFile("E:/CreationArtEngine/Engine/Binaries/Shaders/ShaderType_Vertex.spv"));
    vk::raii::ShaderModule FragmentShaderModule = CreateShaderModule(ReadFile("E:/CreationArtEngine/Engine/Binaries/Shaders/ShaderTypes_Fragment.spv"));

    vk::PipelineShaderStageCreateInfo VertShaderStageInfo;
    VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    VertShaderStageInfo.module = VertexShaderModule;
    VertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo FragShaderStageInfo{};
    FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    FragShaderStageInfo.module = FragmentShaderModule;
    FragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

    // Graphic Pipeline
    vk::PipelineVertexInputStateCreateInfo   vertexInputInfo; //<--- InputAssembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer;//<--- Rasterizer Stages
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.depthBiasSlopeFactor = 1.0f;
    rasterizer.lineWidth = 1.0f;


    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.sampleShadingEnable = vk::False;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = vk::False;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;

    VulkanPipelineLayout = vk::raii::PipelineLayout(VulkanLogicalDevice, pipelineLayoutInfo);

	vk::PipelineRenderingCreateInfo PipelineRenderingCreateInfo;
    PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    PipelineRenderingCreateInfo.pColorAttachmentFormats = &VulkanSwapChainSurfaceFormat.format;

    vk::GraphicsPipelineCreateInfo GraphicPipelineCreateInfo;
    GraphicPipelineCreateInfo.stageCount = 2;
    GraphicPipelineCreateInfo.pStages = ShaderStages;
    GraphicPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    GraphicPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    GraphicPipelineCreateInfo.pViewportState = &viewportState;
    GraphicPipelineCreateInfo.pRasterizationState = &rasterizer;
    GraphicPipelineCreateInfo.pMultisampleState = &multisampling;
    GraphicPipelineCreateInfo.pColorBlendState = &colorBlending;
    GraphicPipelineCreateInfo.pDynamicState = &dynamicState;
    GraphicPipelineCreateInfo.layout = VulkanPipelineLayout;
    GraphicPipelineCreateInfo.renderPass = nullptr; // if we pass something, we dont use dynamic rendering and we have to use traditional renderpass rendering
    GraphicPipelineCreateInfo.pNext = &PipelineRenderingCreateInfo; // Link the pipeline rendering info

    VulkanGraphicsPipeline = vk::raii::Pipeline(VulkanLogicalDevice, nullptr, GraphicPipelineCreateInfo);
}

void Application::CreateCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = queueIndex;
    
    VulkanCommandPool = vk::raii::CommandPool(VulkanLogicalDevice, poolInfo);
}

void Application::CreateCommandBuffers()
{
    VulkanCommandBuffers.clear();

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = VulkanCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    VulkanCommandBuffers = vk::raii::CommandBuffers(VulkanLogicalDevice, allocInfo);
}

void Application::CreateSyncObjects()
{
    assert(VulkanPresentCompleteSemaphores.empty() && VulkanRenderFinishedSemaphores.empty() && VulkanDrawFences.empty());

    for (size_t i = 0; i < VulkanSwapChainImages.size(); i++)
    {
        VulkanRenderFinishedSemaphores.emplace_back(VulkanLogicalDevice, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VulkanPresentCompleteSemaphores.emplace_back(VulkanLogicalDevice, vk::SemaphoreCreateInfo());
        vk::FenceCreateInfo FenceCreateInfo;
        FenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        VulkanDrawFences.emplace_back(VulkanLogicalDevice, FenceCreateInfo);
    }
};

void Application::drawFrame()
{
    auto fenceResult = VulkanLogicalDevice.waitForFences(*VulkanDrawFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to wait for fence!");
    }
    VulkanLogicalDevice.resetFences(*VulkanDrawFences[frameIndex]);

    auto [result, imageIndex] = VulkanSwapChain.acquireNextImage(UINT64_MAX, *VulkanPresentCompleteSemaphores[frameIndex], nullptr);
    
    VulkanCommandBuffers[frameIndex].reset();
	recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	vk::SubmitInfo   submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &*VulkanPresentCompleteSemaphores[frameIndex];
    submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*VulkanCommandBuffers[frameIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &*VulkanRenderFinishedSemaphores[imageIndex];

    VulkanGraphicsQueue.submit(submitInfo, *VulkanDrawFences[frameIndex]);

    vk::PresentInfoKHR presentInfoKHR;
    presentInfoKHR.waitSemaphoreCount = 1;
    presentInfoKHR.pWaitSemaphores = &*VulkanRenderFinishedSemaphores[imageIndex];
    presentInfoKHR.swapchainCount = 1;
    presentInfoKHR.pSwapchains = &*VulkanSwapChain;
    presentInfoKHR.pImageIndices = &imageIndex;


    result = VulkanGraphicsQueue.presentKHR(presentInfoKHR);
    switch (result)
    {
    case vk::Result::eSuccess:
        break;
    case vk::Result::eSuboptimalKHR:
        std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
        break;
    default:
        break;        // an unexpected result is returned!
    }
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

}

void Application::recordCommandBuffer(uint32_t imageIndex)
{
    auto& commandBuffer = VulkanCommandBuffers[frameIndex];

    commandBuffer.begin({});
    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                        // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
    );
    vk::ClearValue              clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

    vk::RenderingAttachmentInfo attachmentInfo;
    attachmentInfo.imageView = VulkanSwapChainImageViews[imageIndex];
    attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        attachmentInfo.clearValue = clearColor;

    vk::RenderingInfo renderingInfo(
        vk::RenderingInfo{}
        .setRenderArea(vk::Rect2D({0, 0}, VulkanSwapChainExtent))
        .setLayerCount(1)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&attachmentInfo)
    );

    commandBuffer.beginRendering(renderingInfo);
    
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *VulkanGraphicsPipeline);
    commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(VulkanSwapChainExtent.width), static_cast<float>(VulkanSwapChainExtent.height), 0.0f, 1.0f));
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), VulkanSwapChainExtent));
    commandBuffer.draw(3, 1, 0, 0);
    
    commandBuffer.endRendering();

    // After rendering, transition the swapchain image to PRESENT_SRC
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
        {},                                                        // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
    );
    commandBuffer.end();
}

void Application::transition_image_layout(
    uint32_t                imageIndex,
    vk::ImageLayout         old_layout,
    vk::ImageLayout         new_layout,
    vk::AccessFlags2        src_access_mask,
    vk::AccessFlags2        dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask)
{
    vk::ImageMemoryBarrier2 barrier(
        vk::ImageMemoryBarrier2{}
        .setSrcStageMask(src_stage_mask)
        .setSrcAccessMask(src_access_mask)
        .setDstStageMask(dst_stage_mask)
        .setDstAccessMask(dst_access_mask)
        .setOldLayout(old_layout)
        .setNewLayout(new_layout)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(VulkanSwapChainImages[imageIndex])
        .setSubresourceRange({
            vk::ImageAspectFlagBits::eColor,
            0, // baseMipLevel
            1, // levelCount
            0, // baseArrayLayer
            1  // layerCount
        })
    );
    vk::DependencyInfo dependency_info(
        vk::DependencyInfo{}
        .setDependencyFlags({})
        .setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier)
    );
    VulkanCommandBuffers[frameIndex].pipelineBarrier2(dependency_info);
}
vk::Extent2D Application::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return capabilities.currentExtent;
    }
    int WindowWidth, WindowHeight;
    glfwGetFramebufferSize(Window, &WindowWidth, &WindowHeight);

    return {
        std::clamp<uint32_t>(WindowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(WindowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
}

void Application::MainLoop()
{
    while (!glfwWindowShouldClose(Window)) {
        glfwPollEvents(); 
    	drawFrame();
    }
    VulkanLogicalDevice.waitIdle();        // wait for device to finish operations before destroying resources
}

void Application::Cleanup()
{
    glfwDestroyWindow(Window);

    glfwTerminate();
}

std::vector<const char*> Application::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}