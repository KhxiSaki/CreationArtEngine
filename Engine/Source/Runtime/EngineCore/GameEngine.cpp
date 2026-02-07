#include "GameEngine.h"
#include <iostream>

#include "Rendering/Renderer.h"

GameEngine::GameEngine()
    : m_Renderer(nullptr)
{
}

GameEngine::~GameEngine()
{
    Shutdown();
}

void GameEngine::Initialize(Window* window)
{
    m_Window = window;
    
    // Initialize the Renderer with the window
    m_Renderer = new Renderer(window);
    
    if (m_Renderer && !m_Renderer->IsInitialized())
    {
        m_Renderer->Initialize(window);
    }

    std::cout << "GameEngine initialized!" << std::endl;
    std::cout << "Renderer initialized successfully!" << std::endl;
}

void GameEngine::Shutdown()
{
    if (m_Renderer)
    {
        delete m_Renderer;
        m_Renderer = nullptr;
    }
}

void GameEngine::Render()
{
    if (m_Renderer)
    {
        m_Renderer->Render();
    }
}

void GameEngine::OnWindowResize()
{
    if (m_Renderer)
    {
        m_Renderer->OnWindowResize();
    }
}

bool GameEngine::IsInitialized() const
{
    return m_Renderer != nullptr;
}