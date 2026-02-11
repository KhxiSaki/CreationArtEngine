#pragma once

#include <memory>
#include "Window.h"

// Forward declaration
class Renderer;
class LayerStack;

class GameEngine
{
public:
	GameEngine();
	~GameEngine();

	void Initialize(Window* window);
	void Shutdown();
	void Render(float DeltaTime);
	void OnWindowResize();

	// Simple status methods
	bool IsInitialized() const;

	// Engine layer stack will hold different layer of engine architecture such as PhysicLayer,RenderLayer,InputLayer etc
	std::unique_ptr<LayerStack> EngineLayerStack;

	void UpdateLayers(float deltaTime);

	LayerStack* GetLayerStack() const { return EngineLayerStack.get(); }

private:
	Window* m_Window;
	Renderer* m_Renderer;
};