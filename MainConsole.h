#pragma once
#include "TypedefRepo.h"
#include "AConsole.h"
#include "process.h"
#include "Scheduler.h"
#include <vector>
#include <memory>
#include <map>

class MainConsole : public AConsole {
public:
    MainConsole();
    ~MainConsole() = default;
    
    void onEnabled() override;
    void display() override;
    void process() override;

private:
    // State management
    bool isSystemInitialized;
    
    // Scheduler system (now handles all process management)
    std::unique_ptr<CPUScheduler> scheduler;
    int nextProcessId;
    
    // Command handling
    bool handleCommand(const String& command);
    std::vector<String> parseCommand(const String& command);
    
    
    void initializeSystem();                            // initialize
    void exitSystem();                                  // exit
    void clearConsole();                                // clear
    void handleScreenCommand(const std::vector<String>& args);  // screen
    void startScheduler();                              // scheduler-start
    void stopScheduler();                               // scheduler-stop
    void generateReport();                              // report-util
    
     
    void createProcess(const String& processName);     // screen -s
    void listProcesses();                              // screen -ls
    void attachToProcess(const String& processName);   // screen -r
    
    // Memory management
    void showMemoryStatus();                           // vmstat
    
    // UI Elements
    void showWelcomeMessage();
    void showLogo();
    void showCommandPrompt();
    void showErrorMessage(const String& error);
    void showUninitializedError();
    
    
    String toLower(const String& str);
};
