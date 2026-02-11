#pragma once

#include <memory>
#include "Window.h"
#include "GameEngine.h"


class Application
{
public:
	Application();
	virtual ~Application();

    virtual void Run();
	void MainLoop();
	void Cleanup();

	float GetCurrentFrameTime() { return CurrentFrameTime; };
	float GetLastFrameTime() { return LastFrameTime; };
	float GetElapsedTime() { return ElapsedTime; };
protected:
	bool bIsApplicationRunning = true;

private:
	//Window
	void InitializeWindow();
	void InitializeEngine();

private:
	Window* m_Window;
	std::unique_ptr<GameEngine> m_Engine;

	float CurrentFrameTime = 0.0f;
	float LastFrameTime = 0.0f;
	float DeltaTime = 0.0f;
	float ElapsedTime = 0.0f;
};
