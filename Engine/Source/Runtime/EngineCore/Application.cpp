#include "Runtime/EngineCore/Application.h"
#include "Runtime/EngineCore/RHI/RHIManager.h"

Application::Application()
{
}

Application::~Application()
{
}

void Application::Run()
{
    InitializeWindow();
    InitializeEngine();
    MainLoop();
    Cleanup();
}

void Application::InitializeWindow()
{
    m_Window = new Window("CreationArtEngine", 800, 600);
}

void Application::InitializeEngine()
{
    m_Engine = std::make_unique<GameEngine>();
    
    // Get RHI preference from manager
    auto& rhiManager = RHIManager::GetInstance();
    RHIType preferredRHI = rhiManager.GetPreferredRHI();
    
    // Print RHI info for debugging
    rhiManager.PrintRHIInfo();
    
    m_Engine->Initialize(m_Window, preferredRHI);
}

void Application::MainLoop()
{
    while (!m_Window->closed()) {
        m_Window->Update(); 
        m_Engine->Render();
    }
}

void Application::Cleanup()
{
    m_Engine.reset();
    delete m_Window;
}