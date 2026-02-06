#pragma once

#include <memory>
#include "Window.h"
#include "GameEngine.h"
#include "Runtime/EngineCore/RHI/RHIManager.h"

class Application
{
public:
	Application();
	virtual ~Application();

    virtual void Run();
	void MainLoop();
	void Cleanup();

protected:
	bool bIsApplicationRunning = true;

private:
	//Window
	void InitializeWindow();
	void InitializeEngine();

private:
	Window* m_Window;
	std::unique_ptr<GameEngine> m_Engine;
};
