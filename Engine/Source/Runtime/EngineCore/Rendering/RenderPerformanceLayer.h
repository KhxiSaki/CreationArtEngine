#pragma once

#include "Layer.h"
#include <vector>
#include <string>
#include <deque>

class Renderer;
class Device;
class PhysicalDevice;

class RenderPerformanceLayer : public Layer
{
public:
    RenderPerformanceLayer();
    ~RenderPerformanceLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(VkCommandBuffer commandBuffer) override;

    void SetRendererContext(Renderer* renderer);

private:
    void QueryGPUInfo();
    void UpdateFrameTiming(float deltaTime);

    Renderer* m_Renderer = nullptr;
    PhysicalDevice* m_PhysicalDevice = nullptr;
    
    // GPU Information
    std::string m_VendorName;
    std::string m_DeviceName;
    std::string m_DriverVersion;
    std::string m_VulkanVersion;
    uint32_t m_VendorID = 0;
    uint32_t m_DeviceID = 0;
    
    // Frame timing data
    static constexpr size_t MaxFrameHistory = 120; // 2 seconds at 60fps
    std::deque<float> m_FrameTimes;
    float m_CurrentFPS = 0.0f;
    float m_AverageFPS = 0.0f;
    float m_MinFPS = 0.0f;
    float m_MaxFPS = 0.0f;
    float m_CurrentFrameTime = 0.0f;
    float m_AverageFrameTime = 0.0f;
    
    // UI State
    bool m_ShowWindow = true;
    float m_UpdateInterval = 0.5f; // Update stats every 0.5 seconds
    float m_TimeSinceLastUpdate = 0.0f;
};
