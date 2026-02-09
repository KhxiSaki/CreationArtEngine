#include "RenderPerformanceLayer.h"
#include "Renderer.h"
#include "Runtime/EngineCore/RHI/PhysicalDevice.h"
#include "imgui.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

RenderPerformanceLayer::RenderPerformanceLayer()
    : Layer("RenderPerformanceLayer")
{
}

RenderPerformanceLayer::~RenderPerformanceLayer()
{
    OnDetach();
}

void RenderPerformanceLayer::SetRendererContext(Renderer* renderer)
{
    m_Renderer = renderer;
}

void RenderPerformanceLayer::OnAttach()
{
    if (!m_Renderer)
    {
        throw std::runtime_error("RenderPerformanceLayer requires a valid Renderer context");
    }

    m_PhysicalDevice = m_Renderer->GetPhysicalDevice();
    QueryGPUInfo();
}

void RenderPerformanceLayer::OnDetach()
{
    m_FrameTimes.clear();
}

void RenderPerformanceLayer::QueryGPUInfo()
{
    if (!m_PhysicalDevice)
        return;

    VkPhysicalDevice physicalDevice = m_PhysicalDevice->get();
    
    // Get device properties
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    
    // Store basic info
    m_DeviceName = deviceProperties.deviceName;
    m_VendorID = deviceProperties.vendorID;
    m_DeviceID = deviceProperties.deviceID;
    
    // Convert vendor ID to name
    switch (m_VendorID)
    {
        case 0x1002: m_VendorName = "AMD"; break;
        case 0x10DE: m_VendorName = "NVIDIA"; break;
        case 0x8086: m_VendorName = "Intel"; break;
        case 0x13B5: m_VendorName = "ARM"; break;
        case 0x5143: m_VendorName = "Qualcomm"; break;
        case 0x1010: m_VendorName = "ImgTec"; break;
        default: 
        {
            std::stringstream ss;
            ss << "Unknown (0x" << std::hex << std::uppercase << m_VendorID << ")";
            m_VendorName = ss.str();
            break;
        }
    }
    
    // Format driver version (NVIDIA-style)
    uint32_t driverVersion = deviceProperties.driverVersion;
    if (m_VendorID == 0x10DE) // NVIDIA
    {
        std::stringstream ss;
        ss << ((driverVersion >> 22) & 0x3FF) << "."
           << ((driverVersion >> 14) & 0xFF) << "."
           << ((driverVersion >> 6) & 0xFF) << "."
           << (driverVersion & 0x3F);
        m_DriverVersion = ss.str();
    }
    else // Standard version encoding
    {
        std::stringstream ss;
        ss << VK_VERSION_MAJOR(driverVersion) << "."
           << VK_VERSION_MINOR(driverVersion) << "."
           << VK_VERSION_PATCH(driverVersion);
        m_DriverVersion = ss.str();
    }
    
    // Format Vulkan API version
    uint32_t apiVersion = deviceProperties.apiVersion;
    std::stringstream ss;
    ss << VK_VERSION_MAJOR(apiVersion) << "."
       << VK_VERSION_MINOR(apiVersion) << "."
       << VK_VERSION_PATCH(apiVersion);
    m_VulkanVersion = ss.str();
}

void RenderPerformanceLayer::UpdateFrameTiming(float deltaTime)
{
    // Add current frame time to history
    m_FrameTimes.push_back(deltaTime * 1000.0f); // Convert to milliseconds
    
    // Keep only the last N frames
    if (m_FrameTimes.size() > MaxFrameHistory)
    {
        m_FrameTimes.pop_front();
    }
    
    // Update current stats
    m_CurrentFrameTime = deltaTime * 1000.0f;
    m_CurrentFPS = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
    
    // Update accumulated stats periodically
    m_TimeSinceLastUpdate += deltaTime;
    if (m_TimeSinceLastUpdate >= m_UpdateInterval)
    {
        m_TimeSinceLastUpdate = 0.0f;
        
        if (!m_FrameTimes.empty())
        {
            // Calculate average
            float sum = std::accumulate(m_FrameTimes.begin(), m_FrameTimes.end(), 0.0f);
            m_AverageFrameTime = sum / m_FrameTimes.size();
            m_AverageFPS = 1000.0f / m_AverageFrameTime;
            
            // Find min and max
            auto minmax = std::minmax_element(m_FrameTimes.begin(), m_FrameTimes.end());
            m_MinFPS = 1000.0f / *minmax.second; // Min FPS = slowest frame
            m_MaxFPS = 1000.0f / *minmax.first;  // Max FPS = fastest frame
        }
    }
}

void RenderPerformanceLayer::OnUpdate(float deltaTime)
{
    // Use ImGui's built-in timing instead of our own
    ImGuiIO& io = ImGui::GetIO();
    float currentFPS = io.Framerate;
    float currentFrameTime = 1000.0f / io.Framerate;
    
    // Update our timing history for graphs
    UpdateFrameTiming(1.0f / io.Framerate);
    
    // Render ImGui UI during OnUpdate, not OnRender
    if (!m_ShowWindow)
        return;

    ImGui::Begin("Render Performance", &m_ShowWindow, ImGuiWindowFlags_AlwaysAutoResize);
    
    // GPU Information Section
    if (ImGui::CollapsingHeader("GPU Information", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Separator();
        ImGui::Text("Vendor:        %s", m_VendorName.c_str());
        ImGui::Text("Device:        %s", m_DeviceName.c_str());
        ImGui::Text("Driver:        %s", m_DriverVersion.c_str());
        ImGui::Text("Vulkan API:    %s", m_VulkanVersion.c_str());
        ImGui::Spacing();
    }
    
    // Frame Statistics Section
    if (ImGui::CollapsingHeader("Frame Statistics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Separator();
        
        // Current stats using ImGui's real-time data
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Current FPS:   %.1f", io.Framerate);
        ImGui::Text("Frame Time:    %.2f ms", 1000.0f / io.Framerate);
        
        ImGui::Spacing();
        
        // Hazel-style CPU/GPU timing breakdown
        if (m_Renderer) {
            ImGui::Separator();
            ImGui::Text("CPU Render Time: %.2f ms", m_Renderer->GetCPURenderTime());
            ImGui::Text("GPU Render Time: %.2f ms", m_Renderer->GetGPUTime());
            ImGui::Text("Total Frame Time: %.2f ms", 1000.0f / io.Framerate);
        }
        
        ImGui::Spacing();
        
        // Average stats from our history
        ImGui::Text("Average FPS:   %.1f", m_AverageFPS);
        ImGui::Text("Avg Frame:     %.2f ms", m_AverageFrameTime);
        
        ImGui::Spacing();
        
        // Min/Max stats from our history
        ImGui::Text("Min FPS:       %.1f", m_MinFPS);
        ImGui::Text("Max FPS:       %.1f", m_MaxFPS);
        
        ImGui::Spacing();
        
        // Additional ImGui metrics
        ImGui::Text("Vertices:      %d", io.MetricsRenderVertices);
        ImGui::Text("Indices:       %d", io.MetricsRenderIndices);
        ImGui::Text("Windows:       %d", io.MetricsRenderWindows);
    }
    
    // Frame Graph Section
    if (ImGui::CollapsingHeader("Frame Graph", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Separator();
        
        if (!m_FrameTimes.empty())
        {
            // Convert deque to vector for ImGui
            std::vector<float> frameTimesVec(m_FrameTimes.begin(), m_FrameTimes.end());
            
            // Calculate overlay text using real-time data
            ImGuiIO& io = ImGui::GetIO();
            char overlay[64];
            snprintf(overlay, sizeof(overlay), "%.2f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            
            // Find min/max for scaling
            float minFrameTime = *std::min_element(frameTimesVec.begin(), frameTimesVec.end());
            float maxFrameTime = *std::max_element(frameTimesVec.begin(), frameTimesVec.end());
            
            // Draw frame time graph
            ImGui::PlotLines("##FrameTimes", 
                           frameTimesVec.data(), 
                           static_cast<int>(frameTimesVec.size()),
                           0,
                           overlay,
                           0.0f,
                           maxFrameTime * 1.1f, // Add 10% headroom
                           ImVec2(0, 80));
            
            // Add reference lines
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("16.67ms (60 FPS)");
            ImGui::Text("33.33ms (30 FPS)");
            ImGui::EndGroup();
            
            // FPS graph (inverted frame times)
            std::vector<float> fpsHistory;
            fpsHistory.reserve(frameTimesVec.size());
            for (float frameTime : frameTimesVec)
            {
                fpsHistory.push_back(frameTime > 0.0f ? (1000.0f / frameTime) : 0.0f);
            }
            
            snprintf(overlay, sizeof(overlay), "%.1f FPS", io.Framerate);
            ImGui::PlotLines("##FPS", 
                           fpsHistory.data(), 
                           static_cast<int>(fpsHistory.size()),
                           0,
                           overlay,
                           0.0f,
                           m_MaxFPS * 1.1f,
                           ImVec2(0, 80));
        }
        else
        {
            ImGui::Text("Collecting data...");
        }
        
        ImGui::Spacing();
        ImGui::Text("History: %zu frames", m_FrameTimes.size());
    }
    
    ImGui::End();
}

void RenderPerformanceLayer::OnRender(VkCommandBuffer commandBuffer)
{
    // This layer renders ImGui content during OnUpdate(), not during Vulkan rendering
    // The ImGuiLayer will handle all Vulkan command buffer recording for ImGui
}
