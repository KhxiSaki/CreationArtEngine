#include "Runtime/EngineCore/RHI/RHIManager.h"
#include "Runtime/EngineCore/RHI/IRHIContext.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Simple JSON-like config for now (can be replaced with proper JSON library later)
RHIManager& RHIManager::GetInstance()
{
    static RHIManager instance;
    if (!instance.m_ConfigLoaded)
    {
        instance.LoadConfig();
    }
    return instance;
}

void RHIManager::LoadConfig()
{
    // For now, use default config
    // TODO: Implement proper JSON loading
    m_Config.preferredRHI = RHIType::Vulkan;
    m_Config.fallbackRHI = RHIType::None;
    m_Config.enableAutoFallback = true;
    m_ConfigLoaded = true;
    
    std::cout << "RHI Manager: Loaded configuration" << std::endl;
}

void RHIManager::SaveConfig()
{
    // TODO: Implement proper JSON saving
    std::cout << "RHI Manager: Saved configuration" << std::endl;
}

void RHIManager::SetConfig(const RHIConfig& config)
{
    m_Config = config;
    SaveConfig();
}

RHIType RHIManager::GetPreferredRHI() const
{
    return m_Config.preferredRHI;
}

RHIType RHIManager::GetFallbackRHI() const
{
    return m_Config.fallbackRHI;
}

void RHIManager::SetPreferredRHI(RHIType type)
{
    m_Config.preferredRHI = type;
    SaveConfig();
}

void RHIManager::SetFallbackRHI(RHIType type)
{
    m_Config.fallbackRHI = type;
    SaveConfig();
}

std::vector<RHIType> RHIManager::GetAvailableRHIs() const
{
    return RHIContextFactory::GetAvailableRHIs();
}

std::vector<std::string> RHIManager::GetAvailableRHINames() const
{
    std::vector<std::string> names;
    auto availableRHIs = GetAvailableRHIs();
    
    for (auto type : availableRHIs)
    {
        names.push_back(RHIContextFactory::GetRHIName(type));
    }
    
    return names;
}

bool RHIManager::IsRHIAvailable(RHIType type) const
{
    return RHIContextFactory::IsRHIAvailable(type);
}

std::string RHIManager::GetRHIStatus() const
{
    std::stringstream ss;
    ss << "RHI Status:\n";
    ss << "  Preferred: " << RHIContextFactory::GetRHIName(m_Config.preferredRHI) << "\n";
    ss << "  Fallback: " << RHIContextFactory::GetRHIName(m_Config.fallbackRHI) << "\n";
    ss << "  Auto-fallback: " << (m_Config.enableAutoFallback ? "Enabled" : "Disabled") << "\n";
    
    ss << "  Available: ";
    auto available = GetAvailableRHINames();
    for (size_t i = 0; i < available.size(); ++i)
    {
        if (i > 0) ss << ", ";
        ss << available[i];
    }
    ss << "\n";
    
    return ss.str();
}

void RHIManager::PrintRHIInfo() const
{
    std::cout << GetRHIStatus() << std::endl;
}