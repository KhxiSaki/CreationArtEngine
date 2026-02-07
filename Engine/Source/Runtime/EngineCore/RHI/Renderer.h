#pragma once

#include <vulkan/vulkan.h>
#include "../Window.h"
#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "SynchronizationObjects.h"
#include "CommandPool.h"

#include <vector>

class Renderer 
{
public:
    Renderer(Window* window);
    ~Renderer();

    void initialize();
    void drawFrame();
    void cleanup();
    void onWindowResize();

private:
    Window* m_pWindow;
};