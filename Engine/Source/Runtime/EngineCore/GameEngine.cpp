#include "GameEngine.h"
#include "RHI/Renderer.h"
#include <iostream>

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
    m_Renderer->initialize();
    
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
        m_Renderer->drawFrame();
    }
}

void GameEngine::OnWindowResize()
{
    if (m_Renderer)
    {
        m_Renderer->onWindowResize();
    }
}

bool GameEngine::IsInitialized() const
{
    return m_Renderer != nullptr;
}