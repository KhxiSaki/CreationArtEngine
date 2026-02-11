#pragma once

#include <string>
#include <memory>

#include "Runtime/EngineCore/RHI/Buffer.h"

// No forward declarations to avoid conflicts with your engine's Vulkan headers

class Layer
{
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender(VkCommandBuffer commandBuffer) = 0;

    const std::string& GetName() const { return m_Name; }
    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }

protected:
    std::string m_Name;
    bool m_Enabled = true;
};
