#include <iostream>

#include "application.hpp"

#define printSdlError(msg) msg << " SDL_Error: " << SDL_GetError() << std::endl;

bool Application::init() {

    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {

        std::cout << printSdlError( "SDL could not initialize!" );
        return false;
    }
    else {

        window = SDL_CreateWindow( title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN );

        if ( window == NULL ) {

            std::cout << printSdlError( "Window could not be created!" );
            return false;
        }
    }

    return true;
}

bool Application::isQuit() {

    return !running;
}

void Application::pollEvents() {

    SDL_Event e;

    while ( SDL_PollEvent( &e ))
    {
        switch ( e.type ) {
            case SDL_QUIT:
                running = false;
                break;
        }
    }
}

void Application::release() {

    SDL_DestroyWindow( window );
    window = NULL;

    SDL_Quit();
}