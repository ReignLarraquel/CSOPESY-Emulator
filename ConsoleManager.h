#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include "TypedefRepo.h"
#include "AConsole.h"

const String MAIN_CONSOLE = "MAIN_CONSOLE";
const String MARQUEE_CONSOLE = "MARQUEE_CONSOLE";
//const String PROCESS_CONSOLE = "PROCESS_CONSOLE";
//const String MEMORY_CONSOLE = "MEMORY_CONSOLE";

class ConsoleManager
{
public:
    typedef std::unordered_map<String, std::shared_ptr<AConsole>> ConsoleTable;

    static ConsoleManager* getInstance();
    static void initialize();
    static void destroy();

    void drawConsole() const;
    void process()     const;
    void switchConsole(String consoleName);
    void returnToPreviousConsole();
    void exitApplication();
    bool isRunning()   const;
    bool hasConsole(const String& consoleName) const;

    
    // Dynamic console registration
    void registerConsole(const String& consoleName, std::shared_ptr<AConsole> console);
    void unregisterConsole(const String& consoleName);

    HANDLE getConsoleHandle() const;
    void   setCursorPosition(int posX, int posY) const;

private:
    ConsoleManager();
    ~ConsoleManager() = default;

    // disable copy/assign
    ConsoleManager(ConsoleManager const&) {}
    ConsoleManager& operator=(ConsoleManager const&) {}

    static ConsoleManager* sharedInstance;

    ConsoleTable                     consoleTable;
    std::shared_ptr<AConsole>        currentConsole;
    std::shared_ptr<AConsole>        previousConsole;
    HANDLE                           consoleHandle;
    bool                             running = true;
};
