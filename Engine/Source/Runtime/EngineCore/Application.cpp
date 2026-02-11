#include "Application.h"
#include <iostream>
#include <chrono>
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
    std::cout << "Creating window..." << std::endl;
    m_Window = new Window("CreationArtEngine", 800, 600);
    std::cout << "Window created successfully!" << std::endl;
}

void Application::InitializeEngine()
{
    std::cout << "Initializing engine..." << std::endl;
    m_Engine = std::make_unique<GameEngine>();
    
    std::cout << "Initializing engine with window..." << std::endl;
    m_Engine->Initialize(m_Window);
    std::cout << "Engine initialized successfully!" << std::endl;
}

void Application::MainLoop()
{
    while (!m_Window->closed()) 
    {
        CurrentFrameTime = static_cast<float>(glfwGetTime());
        DeltaTime = CurrentFrameTime - LastFrameTime;
        ElapsedTime = CurrentFrameTime;

        m_Window->Update(); 
        m_Engine->Render(DeltaTime);

        LastFrameTime = CurrentFrameTime;

    }
}

void Application::Cleanup()
{
    m_Engine.reset();
    delete m_Window;
}