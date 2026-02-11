#pragma once

#include "Runtime/EngineCore/Window.h"
#include <memory>
#include <string>

class IRHIContext
{
public:
    virtual ~IRHIContext() = default;

    virtual void Initialize(Window* window) = 0;
    virtual void Shutdown() = 0;
    virtual void Render(float DeltaTime) = 0;
    virtual void OnWindowResize() = 0;
    
    virtual bool IsInitialized() const = 0;
protected:
    Window* m_Window = nullptr;
    bool m_Initialized = false;
};