#include "GameEngine.h"
#include <iostream>
#include "Rendering/Layer.h"
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
    
    //Attach every engine layer to initialize
    if (EngineLayerStack) {
        for (auto layer : *EngineLayerStack) {
            layer->OnAttach();
        }
    }

    //@TODO: Move rendering initialization into Rendering layer
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
    // Shutdown layers first - this will clean up ImGui's Vulkan resources
    if (EngineLayerStack) 
    {
        for (auto layer : *EngineLayerStack) 
        {
            layer->OnDetach();
        }
        EngineLayerStack.reset();
    }

    if (m_Renderer)
    {
        delete m_Renderer;
        m_Renderer = nullptr;
    }
}

void GameEngine::Render(float DeltaTime)
{
    if (EngineLayerStack)
    {
    	UpdateLayers(DeltaTime);
    }

    if (m_Renderer)
    {
        m_Renderer->Render(DeltaTime);
    }
}


void GameEngine::UpdateLayers(float deltaTime)
{
    // Update all layers
    for (auto layer : *EngineLayerStack)
    {
        if (layer->IsEnabled())
        {
            layer->OnUpdate(deltaTime);
        }
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