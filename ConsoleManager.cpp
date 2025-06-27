// ConsoleManager.cpp
#include "ConsoleManager.h"
#include "MainConsole.h"
#include "MarqueeConsole.h"
#include "SchedulingConsole.h"
//#include "MemorySimulationConsole.h"

// Static member definition
ConsoleManager* ConsoleManager::sharedInstance = nullptr;

ConsoleManager::ConsoleManager() 
    : running(true), currentConsole(nullptr), previousConsole(nullptr)
{
    // Grab Win32 console handle
    this->consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Create each concrete console
    auto mainConsole = std::make_shared<MainConsole>();
    auto marqueeConsole = std::make_shared<MarqueeConsole>();
    auto schedulingConsole = std::make_shared<SchedulingConsole>();
    //auto memoryConsole = std::make_shared<MemorySimulationConsole>();

    // Register them by name
    consoleTable[MAIN_CONSOLE] = mainConsole;
    consoleTable[MARQUEE_CONSOLE] = marqueeConsole;
    consoleTable[SCHEDULING_CONSOLE] = schedulingConsole;
    //consoleTable[MEMORY_CONSOLE] = memoryConsole;

    // Activate the main screen initially
    switchConsole(MAIN_CONSOLE);
}

ConsoleManager* ConsoleManager::getInstance() {
    return sharedInstance;
}

void ConsoleManager::initialize() {
    if (sharedInstance == nullptr) {
        sharedInstance = new ConsoleManager();
    }
}

void ConsoleManager::destroy() {
    if (sharedInstance != nullptr) {
        delete sharedInstance;
        sharedInstance = nullptr;
    }
}

void ConsoleManager::drawConsole() const {
    if (currentConsole != nullptr) {
        currentConsole->display();
    }
}

void ConsoleManager::process() const {
    if (currentConsole != nullptr) {
        currentConsole->process();
    }
}

void ConsoleManager::switchConsole(String consoleName) {
    auto it = consoleTable.find(consoleName);
    if (it != consoleTable.end()) {
        previousConsole = currentConsole;
        currentConsole = it->second;
        if (currentConsole != nullptr) {
            currentConsole->onEnabled();
        }
    }
}

void ConsoleManager::returnToPreviousConsole() {
    if (previousConsole != nullptr) {
        auto temp = currentConsole;
        currentConsole = previousConsole;
        previousConsole = temp;
        if (currentConsole != nullptr) {
            currentConsole->onEnabled();
        }
    }
}

void ConsoleManager::exitApplication() {
    running = false;
}

bool ConsoleManager::isRunning() const {
    return running;
}

HANDLE ConsoleManager::getConsoleHandle() const {
    return consoleHandle;
}

void ConsoleManager::setCursorPosition(int posX, int posY) const {
    //COORD coord = { static_cast<SHORT>(posX), static_cast<SHORT>(posY) };
    //SetConsoleCursorPosition(consoleHandle, coord);
}
