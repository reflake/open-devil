#include <SDL2/SDL.h>

#include "vulkan/engine.hpp"

#pragma once

class Application
{
public:
    Application(const char *title) : title(title) {}

    bool init();
    bool isQuit();
    void pollEvents();
    void release();
    void drawFrame();
    void deviceWaitIdle();

private:

    bool initSDL();
    bool initVulkan();

private:

    const char *title;
    bool running = true;
    SDL_Window *window;
    VulkanEngine vulkanEngine;
};