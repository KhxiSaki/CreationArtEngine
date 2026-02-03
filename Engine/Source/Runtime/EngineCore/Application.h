#pragma once

//TODO: Will move this to precompiled header in the future
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <fstream>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Window.h"

class Application
{
public:
	Application();
	virtual ~Application();

    virtual void Run();
	void MainLoop();
	void Cleanup();
private:

	//Window
	void InitializeWindow();

	// Vulkan
	void InitializeVulkan();
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


	//helpers function- vulkan
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
	{
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
		if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
		{
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

	static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
	{
		assert(!availableFormats.empty());
		const auto formatIt = std::ranges::find_if(
			availableFormats,
			[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
		return std::ranges::any_of(availablePresentModes,
			[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
			vk::PresentModeKHR::eMailbox :
			vk::PresentModeKHR::eFifo;
	}

	std::vector<char const*> getRequiredExtensions();

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
	{
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{
			std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
		}

		return vk::False;
	}



	//ShaderManager
	[[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& code) const
	{
		vk::ShaderModuleCreateInfo CreateInfo;
		CreateInfo.codeSize = code.size() * sizeof(char);
		CreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		vk::raii::ShaderModule     ShaderModule{ VulkanLogicalDevice, CreateInfo };

		return ShaderModule;
	}

	//FileManager
	static std::vector<char> ReadFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file!");
		}
		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();
		return buffer;
	}

protected:
	bool bIsApplicationRunning = true;

private:
	Window* m_Window;

	//Vulkan
	vk::raii::Context  VulkanContext;
	vk::raii::Instance VulkanInstance = nullptr;
	vk::raii::DebugUtilsMessengerEXT VulkanDebugMessenger = nullptr;
	vk::raii::PhysicalDevice VulkanPhysicalDevice = nullptr;
	vk::raii::Device VulkanLogicalDevice = nullptr;
	vk::raii::Queue VulkanGraphicsQueue = nullptr;
	vk::raii::SurfaceKHR VulkanSurface = nullptr;
	vk::raii::SwapchainKHR           VulkanSwapChain = nullptr;
	std::vector<vk::Image>           VulkanSwapChainImages;
	vk::SurfaceFormatKHR             VulkanSwapChainSurfaceFormat;
	vk::Extent2D                     VulkanSwapChainExtent;
	std::vector<vk::raii::ImageView> VulkanSwapChainImageViews;
	uint32_t                         queueIndex = ~0;
	uint32_t frameIndex = 0;

	vk::raii::PipelineLayout VulkanPipelineLayout = nullptr;
	vk::raii::Pipeline       VulkanGraphicsPipeline = nullptr;
	vk::raii::CommandPool VulkanCommandPool = nullptr;
	std::vector<vk::raii::CommandBuffer>  VulkanCommandBuffers;
	std::vector<vk::raii::Semaphore> VulkanPresentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> VulkanRenderFinishedSemaphores;
	std::vector<vk::raii::Fence>     VulkanDrawFences;

	std::vector<const char*> VulkanRequiredDeviceExtension = { vk::KHRSwapchainExtensionName };
};
