#include <SDL2/SDL.h>

#pragma once

class Application
{
public:
    Application(const char *title) : title(title) {}

    bool init();
    bool isQuit();
    void pollEvents();
    void release();

private:

    const char *title;
    bool running = true;
    SDL_Window *window;
};