#include "Device.h"
#include "PhysicalDevice.h"
#include <iostream>
#include <vk_mem_alloc.h>

Device::Device(VkDevice device, VkQueue graphicsQueue, VkQueue presentQueue, VkPhysicalDevice physicalDevice, VkInstance instance)
    : m_Device(device), m_GraphicsQueue(graphicsQueue), m_PresentQueue(presentQueue) 
{
    // Create VMA allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    
    if (vmaCreateAllocator(&allocatorInfo, &m_Allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }
    
	std::cout << "Device created successfully." << std::endl;
}

Device::~Device() 
{
    if (m_Allocator != VK_NULL_HANDLE) 
    {
        vmaDestroyAllocator(m_Allocator);
    }
    if (m_Device != VK_NULL_HANDLE) 
    {
        vkDestroyDevice(m_Device, nullptr);
    }
	std::cout << "Device destroyed." << std::endl;
}

VkDevice Device::get() const 
{
    return m_Device;
}

VkQueue Device::getGraphicsQueue() const 
{
    return m_GraphicsQueue;
}

VkQueue Device::getPresentQueue() const 
{
    return m_PresentQueue;
}

VmaAllocator Device::getAllocator() const
{
    return m_Allocator;
}

Device::Device(Device&& other) noexcept 
{
    m_Device = other.m_Device;
    m_GraphicsQueue = other.m_GraphicsQueue;
    m_PresentQueue = other.m_PresentQueue;

    other.m_Device = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) noexcept 
{
    if (this != &other) 
    {
        if (m_Device != VK_NULL_HANDLE) 
        {
            vkDestroyDevice(m_Device, nullptr);
        }
        m_Device = other.m_Device;
        m_GraphicsQueue = other.m_GraphicsQueue;
        m_PresentQueue = other.m_PresentQueue;

        other.m_Device = VK_NULL_HANDLE;
    }
    return *this;
}