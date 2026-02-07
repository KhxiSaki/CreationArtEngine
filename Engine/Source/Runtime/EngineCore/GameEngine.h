#pragma once

#include <memory>
#include "Window.h"

// Forward declaration
class Renderer;

class GameEngine
{
public:
	GameEngine();
	~GameEngine();

	void Initialize(Window* window);
	void Shutdown();
	void Render();
	void OnWindowResize();

	// Simple status methods
	bool IsInitialized() const;

private:
	Window* m_Window;
	Renderer* m_Renderer;
};