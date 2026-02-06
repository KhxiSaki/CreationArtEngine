#pragma once

#include "Runtime/EngineCore/RHI/IRHIContext.h"
#include <string>
#include <vector>

struct RHIConfig
{
    RHIType preferredRHI = RHIType::Vulkan;
    RHIType fallbackRHI = RHIType::None;
    bool enableAutoFallback = true;
    std::string configFilePath = "Engine/RHIConfig.json";
};

class RHIManager
{
public:
    static RHIManager& GetInstance();
    
    // Configuration management
    void LoadConfig();
    void SaveConfig();
    void SetConfig(const RHIConfig& config);
    const RHIConfig& GetConfig() const { return m_Config; }
    
    // RHI selection and management
    RHIType GetPreferredRHI() const;
    RHIType GetFallbackRHI() const;
    void SetPreferredRHI(RHIType type);
    void SetFallbackRHI(RHIType type);
    
    // Available RHI information
    std::vector<RHIType> GetAvailableRHIs() const;
    std::vector<std::string> GetAvailableRHINames() const;
    bool IsRHIAvailable(RHIType type) const;
    
    // Editor interface
    std::string GetRHIStatus() const;
    void PrintRHIInfo() const;

private:
    RHIManager() = default;
    ~RHIManager() = default;
    RHIManager(const RHIManager&) = delete;
    RHIManager& operator=(const RHIManager&) = delete;
    
    RHIConfig m_Config;
    bool m_ConfigLoaded = false;
};