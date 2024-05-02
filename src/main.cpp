#include <iostream>

#include "application.hpp"

int main() {

    Application app( "Hello, Devil Hunter!" );

    bool success = app.init();

    std::cout << "Window created successfully" << std::endl;

    while ( !app.isQuit() ) {

        app.pollEvents();
        app.drawFrame();
    }

    // make sure device is idling before releasing any resources
    app.deviceWaitIdle();
    app.release();

    return 0;
}