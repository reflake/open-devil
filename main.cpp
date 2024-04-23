#include <SFML/Graphics.hpp>

int main() {

    sf::Window window(
        sf::VideoMode(640, 480),
        "Hello, Devil Hunter!");

    while (window.isOpen()) {

        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
    }

    return 0;
}