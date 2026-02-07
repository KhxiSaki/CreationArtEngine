#include "PhysicalDevice.h"
#include <stdexcept>
#include <set>
#include <iostream>

PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
    const std::vector<const char*>& requiredExtensions,
    const VkPhysicalDeviceFeatures& requiredFeatures,
    const VkPhysicalDeviceVulkan11Features* vulkan11Features,
    const VkPhysicalDeviceVulkan12Features* vulkan12Features,
    const VkPhysicalDeviceVulkan13Features* vulkan13Features)
    : m_Instance(instance), m_Surface(surface),
    m_RequiredExtensions(requiredExtensions),
    m_RequiredFeatures(requiredFeatures)
{
    if (vulkan11Features)
    {
        m_Vulkan11Features = *vulkan11Features;
        m_UseVulkan11Features = true;
    }

    if (vulkan12Features)
    {
        m_Vulkan12Features = *vulkan12Features;
        m_UseVulkan12Features = true;
    }

    if (vulkan13Features)
    {
        m_Vulkan13Features = *vulkan13Features;
        m_UseVulkan13Features = true;
    }

    pickPhysicalDevice();
	std::cout << "PhysicalDevice created." << std::endl;
}

// Move constructor
PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept
    : m_Instance(other.m_Instance),
    m_Surface(other.m_Surface),
    m_PhysicalDevice(other.m_PhysicalDevice),
    m_QueueFamilyIndices(std::move(other.m_QueueFamilyIndices)),
    m_SwapChainSupportDetails(std::move(other.m_SwapChainSupportDetails)),
    m_RequiredExtensions(std::move(other.m_RequiredExtensions)),
	m_RequiredFeatures(other.m_RequiredFeatures),
	m_Vulkan11Features(other.m_Vulkan11Features),
	m_Vulkan12Features(other.m_Vulkan12Features),
	m_Vulkan13Features(other.m_Vulkan13Features),
	m_UseVulkan11Features(other.m_UseVulkan11Features),
	m_UseVulkan12Features(other.m_UseVulkan12Features),
	m_UseVulkan13Features(other.m_UseVulkan13Features)
{
    other.m_PhysicalDevice = VK_NULL_HANDLE;
}

// Move assignment operator
PhysicalDevice& PhysicalDevice::operator=(PhysicalDevice&& other) noexcept
{
    if (this != &other)
    {
        m_Instance = other.m_Instance;
        m_Surface = other.m_Surface;
        m_PhysicalDevice = other.m_PhysicalDevice;
        m_QueueFamilyIndices = std::move(other.m_QueueFamilyIndices);
        m_SwapChainSupportDetails = std::move(other.m_SwapChainSupportDetails);
        m_RequiredExtensions = std::move(other.m_RequiredExtensions);
        m_RequiredFeatures = other.m_RequiredFeatures;
		m_Vulkan11Features = other.m_Vulkan11Features;
		m_Vulkan12Features = other.m_Vulkan12Features;
		m_Vulkan13Features = other.m_Vulkan13Features;
		m_UseVulkan11Features = other.m_UseVulkan11Features;
		m_UseVulkan12Features = other.m_UseVulkan12Features;
		m_UseVulkan13Features = other.m_UseVulkan13Features;

        other.m_PhysicalDevice = VK_NULL_HANDLE;
    }
    return *this;
}

VkPhysicalDevice PhysicalDevice::get() const
{
    return m_PhysicalDevice;
}

void PhysicalDevice::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
	std::cout << "Found " << deviceCount << " devices" << std::endl;

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "Checking device: " << deviceProperties.deviceName << std::endl;

        if (isDeviceSuitable(device))
        {
            std::cout << "Found suitable device: " << deviceProperties.deviceName << std::endl;
            m_PhysicalDevice = device;
            m_QueueFamilyIndices = findQueueFamilies(device);
            m_SwapChainSupportDetails = querySwapChainSupport();
            break;
        }
    }
    if (m_PhysicalDevice != VK_NULL_HANDLE && !checkFeatureSupport(m_PhysicalDevice))
    {
        throw std::runtime_error("Required features are not supported by the selected physical device.");
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

bool PhysicalDevice::checkFeatureSupport(VkPhysicalDevice device)
{
    VkPhysicalDeviceFeatures2 supportedFeatures2{};
    supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceVulkan11Features supportedVulkan11Features{};
    supportedVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    VkPhysicalDeviceVulkan12Features supportedVulkan12Features{};
    supportedVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceVulkan13Features supportedVulkan13Features{};
    supportedVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    // Chain the features
	supportedFeatures2.pNext = &supportedVulkan11Features;
	supportedVulkan11Features.pNext = &supportedVulkan12Features;
	supportedVulkan12Features.pNext = &supportedVulkan13Features;

    vkGetPhysicalDeviceFeatures2(device, &supportedFeatures2);

    // Check Vulkan 1.0 features
    if (m_RequiredFeatures.samplerAnisotropy && !supportedFeatures2.features.samplerAnisotropy)
    {
        return false;
    }

    // Check Vulkan 1.3 features (especially dynamic rendering)
    if (m_UseVulkan13Features)
    {
        if (!supportedVulkan13Features.dynamicRendering)
        {
            std::cout << "Device does not support dynamic rendering!" << std::endl;
            return false;
        }
        
        if (!supportedVulkan13Features.synchronization2)
        {
            std::cout << "Device does not support synchronization2!" << std::endl;
            return false;
        }
    }
 
    // Check Vulkan 1.1 features
    if (m_UseVulkan11Features)
    {
        // ... check other Vulkan 1.1 features
    }

    // Check Vulkan 1.2 features
    if (m_UseVulkan12Features)
    {
        if (m_Vulkan12Features.runtimeDescriptorArray && !supportedVulkan12Features.runtimeDescriptorArray)
        {
            return false;
        }
        // ... check other Vulkan 1.2 features
    }

    // Check Vulkan 1.3 features
    if (m_UseVulkan13Features)
    {
		if (m_Vulkan13Features.synchronization2 && !supportedVulkan13Features.synchronization2)
		{
			return false;
		}
		if (m_Vulkan13Features.dynamicRendering && !supportedVulkan13Features.dynamicRendering)
		{
			return false;
		}
        // ... check Vulkan 1.3 features
    }

    return true;
}

bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device)
{
    auto indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    bool storageImageSupported = false;

    std::cout << "  - Graphics queue family: " << (indices.graphicsFamily.has_value() ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "  - Present queue family: " << (indices.presentFamily.has_value() ? "FOUND" : "NOT FOUND") << std::endl;
    std::cout << "  - Extensions supported: " << (extensionsSupported ? "YES" : "NO") << std::endl;

    if (extensionsSupported)
    {
        auto swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

        // Check if swapchain supports storage image usage
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &capabilities);
        storageImageSupported = (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT);

        // If you want to log this information for debugging:
        if (storageImageSupported) {
            std::cout << "Device supports storage images for swapchain" << std::endl;
        }
        else {
            std::cerr << "Device DOES NOT support storage images for swapchain" << std::endl;
        }
        
        std::cout << "  - Swap chain adequate: " << (swapChainAdequate ? "YES" : "NO") << std::endl;
        std::cout << "  - Storage image supported: " << (storageImageSupported ? "YES" : "NO") << std::endl;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    bool anisotropySupported = !m_RequiredFeatures.samplerAnisotropy || supportedFeatures.samplerAnisotropy;
    
    std::cout << "  - Is discrete GPU: " << (isDiscreteGPU ? "YES" : "NO") << std::endl;
    std::cout << "  - Required anisotropy: " << (m_RequiredFeatures.samplerAnisotropy ? "YES" : "NO") << std::endl;
    std::cout << "  - Supported anisotropy: " << (supportedFeatures.samplerAnisotropy ? "YES" : "NO") << std::endl;
    std::cout << "  - Anisotropy supported: " << (anisotropySupported ? "YES" : "NO") << std::endl;

    bool suitable = indices.isComplete()
        && extensionsSupported
        && swapChainAdequate
        && anisotropySupported
        && isDiscreteGPU
        && storageImageSupported; // Add storage image support as a requirement
        
    std::cout << "  - Device is suitable: " << (suitable ? "YES" : "NO") << std::endl;

    return suitable;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }
        i++;
    }

    return indices;
}

bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_RequiredExtensions.begin(), m_RequiredExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

PhysicalDevice::SwapChainSupportDetails PhysicalDevice::querySwapChainSupport() const
{
    return querySwapChainSupport(m_PhysicalDevice);
}

PhysicalDevice::SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool PhysicalDevice::QueueFamilyIndices::isComplete() const
{
    return graphicsFamily.has_value() && presentFamily.has_value();
}

const PhysicalDevice::QueueFamilyIndices& PhysicalDevice::getQueueFamilyIndices() const
{
    return m_QueueFamilyIndices;
}

const VkPhysicalDeviceFeatures& PhysicalDevice::getFeatures() const
{
    return m_RequiredFeatures;
}

const std::vector<const char*>& PhysicalDevice::getExtensions() const
{
    return m_RequiredExtensions;
}

const VkPhysicalDeviceVulkan11Features& PhysicalDevice::getVulkan11Features() const
{
	return m_Vulkan11Features;
}

const VkPhysicalDeviceVulkan12Features& PhysicalDevice::getVulkan12Features() const
{
	return m_Vulkan12Features;
}

const VkPhysicalDeviceVulkan13Features& PhysicalDevice::getVulkan13Features() const
{
	return m_Vulkan13Features;
}

