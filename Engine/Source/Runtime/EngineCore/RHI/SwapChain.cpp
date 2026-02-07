#include "SwapChain.h"
#include <iostream>

SwapChain::SwapChain(VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR swapChain,
                     std::vector<VkImage> images, std::vector<VkImageView> imageViews,
                     VkFormat imageFormat, VkExtent2D extent)
    : m_Device(device), m_Surface(surface), m_SwapChain(swapChain),
    m_Images(images), m_ImageViews(imageViews),
    m_ImageFormat(imageFormat), m_Extent(extent) 
{
	std::cout << "SwapChain created with " << m_Images.size() << " images and " << m_ImageViews.size() << " image views" << std::endl;
	std::cout << "SwapChain extent: " << m_Extent.width << "x" << m_Extent.height << std::endl;
}

SwapChain::~SwapChain() 
{
    std::cout << "SwapChain destructor called, destroying " << m_ImageViews.size() << " image views" << std::endl;

    for (auto imageView : m_ImageViews) 
    {
        std::cout << "Destroying image view: " << static_cast<void*>(imageView) << std::endl;
        vkDestroyImageView(m_Device, imageView, nullptr);
    }

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    }
	std::cout << "SwapChain destroyed" << std::endl;
}


SwapChain::SwapChain(SwapChain&& other) noexcept
    : m_Device(other.m_Device), m_Surface(other.m_Surface), m_SwapChain(other.m_SwapChain),
    m_Images(std::move(other.m_Images)), m_ImageViews(std::move(other.m_ImageViews)),
    m_ImageFormat(other.m_ImageFormat), m_Extent(other.m_Extent) 
{
    other.m_SwapChain = VK_NULL_HANDLE;
}

SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
    if (this != &other) 
    {
        m_Device = other.m_Device;
        m_Surface = other.m_Surface;
        m_SwapChain = other.m_SwapChain;
        m_Images = std::move(other.m_Images);
        m_ImageViews = std::move(other.m_ImageViews);
        m_ImageFormat = other.m_ImageFormat;
        m_Extent = other.m_Extent;

        other.m_SwapChain = VK_NULL_HANDLE;
    }
    return *this;
}

VkSwapchainKHR SwapChain::get() const 
{
    return m_SwapChain;
}

const std::vector<VkImage>& SwapChain::getImages() const 
{
    return m_Images;
}

const std::vector<VkImageView>& SwapChain::getImageViews() const 
{
    return m_ImageViews;
}

VkFormat SwapChain::getImageFormat() const 
{
    return m_ImageFormat;
}

VkExtent2D SwapChain::getExtent() const 
{
    return m_Extent;
}

void SwapChain::release()
{
    // Release ownership without destroying the swapchain
    // This is used when the swapchain is passed as oldSwapchain to vkCreateSwapchainKHR
    m_SwapChain = VK_NULL_HANDLE;
}
