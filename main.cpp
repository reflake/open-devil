#include <iostream>

#include "application.hpp"

int main() {

    Application app( "Hello, Devil Hunter!" );

    bool success = app.init();

    std::cout << "Window created successfully" << std::endl;

    while ( !app.isQuit() ) {

        app.pollEvents();
    }

    app.release();

    return 0;
}