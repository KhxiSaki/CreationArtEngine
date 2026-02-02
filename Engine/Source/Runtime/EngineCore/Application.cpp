#include "Runtime/EngineCore/Application.h"
#include <iostream>

Application::Application()
{
}

Application::~Application()
{
}

void Application::Run()
{
	while (bIsApplicationRunning)
	{
		std::cout << "Hello Game Engine";
		return;
	}
}
