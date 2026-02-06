#include "Runtime/EngineCore/GameEngine.h"

GameEngine::GameEngine()
    : m_CurrentRHI(RHIType::None)
{
}

GameEngine::~GameEngine()
{
    Shutdown();
}

void GameEngine::Initialize(Window* window, RHIType preferredRHI)
{
    m_Window = window;
    
    // Try to create the preferred RHI context
    m_RHIContext = RHIContextFactory::CreateRHIContext(preferredRHI);
    
    if (!m_RHIContext)
    {
        // Fallback to available RHI
        auto availableRHIs = RHIContextFactory::GetAvailableRHIs();
        if (!availableRHIs.empty())
        {
            m_RHIContext = RHIContextFactory::CreateRHIContext(availableRHIs[0]);
            m_CurrentRHI = availableRHIs[0];
        }
    }
    else
    {
        m_CurrentRHI = preferredRHI;
    }
    
    if (m_RHIContext)
    {
        m_RHIContext->Initialize(window);
        std::cout << "GameEngine initialized with " << m_RHIContext->GetName() << " RHI" << std::endl;
    }
    else
    {
        throw std::runtime_error("Failed to initialize any RHI context!");
    }
}

void GameEngine::Shutdown()
{
    if (m_RHIContext)
    {
        m_RHIContext->Shutdown();
        m_RHIContext.reset();
        m_CurrentRHI = RHIType::None;
    }
}

void GameEngine::Render()
{
    if (m_RHIContext)
    {
        m_RHIContext->Render();
    }
}

void GameEngine::OnWindowResize()
{
    if (m_RHIContext)
    {
        m_RHIContext->OnWindowResize();
    }
}

RHIType GameEngine::GetCurrentRHI() const
{
    return m_CurrentRHI;
}

const char* GameEngine::GetCurrentRHIName() const
{
    return RHIContextFactory::GetRHIName(m_CurrentRHI);
}

bool GameEngine::SetRHI(RHIType type)
{
    if (m_CurrentRHI == type)
        return true;
    
    // Shutdown current RHI
    if (m_RHIContext)
    {
        m_RHIContext->Shutdown();
        m_RHIContext.reset();
    }
    
    // Create new RHI context
    m_RHIContext = RHIContextFactory::CreateRHIContext(type);
    if (m_RHIContext && m_Window)
    {
        m_RHIContext->Initialize(m_Window);
        m_CurrentRHI = type;
        std::cout << "Switched to " << m_RHIContext->GetName() << " RHI" << std::endl;
        return true;
    }
    
    return false;
}