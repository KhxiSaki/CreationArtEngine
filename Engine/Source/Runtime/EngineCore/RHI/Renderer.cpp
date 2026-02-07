#include "Renderer.h"
#include <stdexcept>
#include <iostream>

Renderer::Renderer(Window* window)
    : m_pWindow(window)
{
    std::cout << "Renderer created." << std::endl;
}

Renderer::~Renderer() 
{
    cleanup();
    std::cout << "Destroying Renderer." << std::endl;
}

void Renderer::initialize() 
{
    std::cout << "Initializing Renderer." << std::endl;
    std::cout << "Renderer initialized." << std::endl;
}

void Renderer::drawFrame() 
{
}

void Renderer::onWindowResize() 
{
}

void Renderer::cleanup() 
{
}