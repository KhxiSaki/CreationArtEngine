#pragma once

#include "Runtime/EngineCore/Window.h"
#include <memory>
#include <string>

enum class RHIType
{
    Vulkan,
    DirectX12,
    None
};

class IRHIContext
{
public:
    virtual ~IRHIContext() = default;

    virtual void Initialize(Window* window) = 0;
    virtual void Shutdown() = 0;
    virtual void Render() = 0;
    virtual void OnWindowResize() = 0;
    
    virtual bool IsInitialized() const = 0;
    virtual RHIType GetType() const = 0;
    virtual const char* GetName() const = 0;

protected:
    Window* m_Window = nullptr;
    bool m_Initialized = false;
};

class RHIContextFactory
{
public:
    static std::unique_ptr<IRHIContext> CreateRHIContext(RHIType type);
    
    static std::vector<RHIType> GetAvailableRHIs();
    static const char* GetRHIName(RHIType type);
    static bool IsRHIAvailable(RHIType type);
};