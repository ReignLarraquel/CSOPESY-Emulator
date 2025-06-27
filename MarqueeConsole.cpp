#include "MarqueeConsole.h"
#include <iostream>
#include "ConsoleManager.h"

MarqueeConsole::MarqueeConsole() : AConsole(MARQUEE_CONSOLE) {
}

void MarqueeConsole::onEnabled() {
    display();
}

void MarqueeConsole::display() {
    system("cls");
    std::cout << "=== MARQUEE CONSOLE ===" << std::endl;
    std::cout << "This console is not yet implemented." << std::endl;
    std::cout << "Type 'exit' to return to main console." << std::endl;
    std::cout << "> ";
}

void MarqueeConsole::process() {
    String input;
    std::getline(std::cin, input);
    
    if (input == "exit") {
        // Return to main console - this will be handled by ConsoleManager
        std::cout << "Returning to main console..." << std::endl;
    } else {
        std::cout << "Command not recognized. Type 'exit' to return." << std::endl;
    }
} 