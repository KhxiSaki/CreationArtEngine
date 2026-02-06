#include "DirectXContext.h"

DirectXContext::DirectXContext()
{
}

DirectXContext::~DirectXContext()
{
    Shutdown();
}

void DirectXContext::Initialize(Window* window)
{
    m_Window = window;
    LogNotImplemented();
    std::cout << "DirectX 12: Initialize() - Not implemented yet" << std::endl;
    m_Initialized = true;
}

void DirectXContext::Shutdown()
{
    if (m_Initialized)
    {
        LogNotImplemented();
        std::cout << "DirectX 12: Shutdown() - Not implemented yet" << std::endl;
        m_Initialized = false;
    }
}

void DirectXContext::Render()
{
    if (m_Initialized)
    {
        LogNotImplemented();
        std::cout << "DirectX 12: Render() - Not implemented yet" << std::endl;
    }
}

void DirectXContext::OnWindowResize()
{
    if (m_Initialized)
    {
        LogNotImplemented();
        std::cout << "DirectX 12: OnWindowResize() - Not implemented yet" << std::endl;
    }
}

bool DirectXContext::IsAvailable()
{
    // TODO: Check for DirectX 12 availability
    // For now, return false to indicate it's not implemented
    return false;
}

void DirectXContext::LogNotImplemented() const
{
    static bool warned = false;
    if (!warned)
    {
        std::cout << "DirectX 12 RHI is not implemented yet. Please implement DirectX 12 support." << std::endl;
        warned = true;
    }
}