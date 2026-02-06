#pragma once

#include "Runtime/EngineCore/RHI/IRHIContext.h"
#include <iostream>

class DirectXContext : public IRHIContext
{
public:
    DirectXContext();
    ~DirectXContext();

    // IRHIContext interface
    void Initialize(Window* window) override;
    void Shutdown() override;
    void Render() override;
    void OnWindowResize() override;
    
    bool IsInitialized() const override { return m_Initialized; }
    RHIType GetType() const override { return RHIType::DirectX12; }
    const char* GetName() const override { return "DirectX 12"; }

    // DirectX-specific static methods
    static bool IsAvailable();

private:
    void LogNotImplemented() const;
};