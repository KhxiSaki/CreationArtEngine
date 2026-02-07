#include "Runtime/EngineCore/RHI/IRHIContext.h"

std::unique_ptr<IRHIContext> RHIContextFactory::CreateRHIContext(RHIType type)
{
    switch (type)
    {
    case RHIType::Vulkan:
        return nullptr;
    case RHIType::DirectX12:
        return nullptr;
    case RHIType::None:
    default:
        return nullptr;
    }
}

std::vector<RHIType> RHIContextFactory::GetAvailableRHIs()
{
    std::vector<RHIType> availableRHIs;
    
    if (IsRHIAvailable(RHIType::Vulkan))
        availableRHIs.push_back(RHIType::Vulkan);
    
    if (IsRHIAvailable(RHIType::DirectX12))
        availableRHIs.push_back(RHIType::DirectX12);
    
    return availableRHIs;
}

const char* RHIContextFactory::GetRHIName(RHIType type)
{
    switch (type)
    {
    case RHIType::Vulkan:
        return "Vulkan";
    case RHIType::DirectX12:
        return "DirectX 12";
    case RHIType::None:
    default:
        return "None";
    }
}

bool RHIContextFactory::IsRHIAvailable(RHIType type)
{
    switch (type)
    {
    case RHIType::Vulkan:
        return false;
    case RHIType::DirectX12:
        return false;
    case RHIType::None:
    default:
        return false;
    }
}