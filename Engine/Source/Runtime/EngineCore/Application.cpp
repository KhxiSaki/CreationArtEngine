#include "Runtime/EngineCore/Application.h"
#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>

Application::Application()
{
}

Application::~Application()
{
}

void Application::Run()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "RealityEngine", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

}
