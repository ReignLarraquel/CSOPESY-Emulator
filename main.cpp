#include <iostream>
#include "TypedefRepo.h"
#include "ConsoleManager.h"

int main()
{
    ConsoleManager::initialize();
    ConsoleManager* consoleManager = ConsoleManager::getInstance();

    consoleManager->switchConsole(MAIN_CONSOLE);

    // Main console loop - display and process input
    while (consoleManager->isRunning())
    {
        consoleManager->drawConsole();
        consoleManager->process();
    }

    // Clean up
    ConsoleManager::destroy();

    return 0;
}