#include <iostream>
#include <cstdlib>
#include <ctime>
#include "TypedefRepo.h"
#include "ConsoleManager.h"

int main()
{
    // Seed random number generator to prevent predictable sequences
    srand(static_cast<unsigned int>(time(nullptr)));
    
    ConsoleManager::initialize();
    ConsoleManager* consoleManager = ConsoleManager::getInstance();

    consoleManager->switchConsole(MAIN_CONSOLE);

    // Main console loop - process input (display handled by onEnabled)
    while (consoleManager->isRunning())
    {
        consoleManager->process();
    }

    // Clean up
    ConsoleManager::destroy();

    return 0;
}