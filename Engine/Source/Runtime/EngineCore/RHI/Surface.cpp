#include "Surface.h"
#include <stdexcept>
#include <iostream>

Surface::Surface(VkInstance vkInstance, GLFWwindow* window)
    : m_Instance(vkInstance), m_Surface(VK_NULL_HANDLE)
{
    if (glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create window surface!");
    }

	std::cout << "Window surface created: " << static_cast<void*>(m_Surface) << std::endl;
}

Surface::~Surface() 
{
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	std::cout << "Window surface destroyed." << std::endl;
}

VkSurfaceKHR Surface::get() const 
{
    return m_Surface;
}
