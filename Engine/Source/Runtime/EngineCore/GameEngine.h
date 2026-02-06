#pragma once

#include <memory>
#include "Window.h"
#include "RHI/IRHIContext.h"
#include "Runtime/EngineCore/RHI/IRHIContext.h"

class GameEngine
{
public:
	GameEngine();
	~GameEngine();

	void Initialize(Window* window, RHIType preferredRHI = RHIType::Vulkan);
	void Shutdown();
	void Render();
	void OnWindowResize();

	// RHI management
	RHIType GetCurrentRHI() const;
	const char* GetCurrentRHIName() const;
	bool SetRHI(RHIType type);

private:
	Window* m_Window;
	std::unique_ptr<IRHIContext> m_RHIContext;
	RHIType m_CurrentRHI;
};