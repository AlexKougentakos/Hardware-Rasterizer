#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"

using namespace dae;

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 800;
	const uint32_t height = 480;
	const std::string windowTitle{"DirectX - Alexandros Kougentakos - 2DAE07"};

	SDL_Window* pWindow = SDL_CreateWindow(
		windowTitle.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	bool isUsingDX = true;
	bool isShowingFPS = true;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				//Test for a key
				pRenderer->HandleInput(e);
				if (e.key.keysym.scancode == SDL_SCANCODE_F1)
				{
					isUsingDX = !isUsingDX;
					pRenderer->SetRasterizerModel(isUsingDX);
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F11)
				{
					if (isShowingFPS)
						std::cout << "\033[1;33mFPS FPS Display (On Window Title) is paused!\033[0m" << std::endl;
					else std::cout << "\033[1;33mFPS FPS Display (On Window Title) is resumed!\033[0m" << std::endl;
					isShowingFPS = !isShowingFPS;
				}
					
				break;
			default: ;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		if (isUsingDX)
			pRenderer->RenderDirectX();
		else 
			pRenderer->RenderSoftware();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			printTimer = 0.f;
			if (isShowingFPS)
				SDL_SetWindowTitle(pWindow, (std::stringstream{} << windowTitle.c_str() << " || dFPS: " << std::to_string(pTimer->GetFPS())).str().c_str());
			else SDL_SetWindowTitle(pWindow, (std::stringstream{} << windowTitle.c_str() << " || dFPS: Paused!").str().c_str());
			//std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);
	return 0;


}