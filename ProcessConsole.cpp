#include "ProcessConsole.h"
#include "ConsoleManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

ProcessConsole::ProcessConsole(std::shared_ptr<Process> process) 
    : AConsole(process ? ("PROCESS_" + process->getName()) : "PROCESS_CONSOLE"), 
      attachedProcess(process) {
}

void ProcessConsole::onEnabled() {
    display();
    
    // Run interactive loop
    String input;
    while (true) {
        std::cout << "root:\\> ";
        std::getline(std::cin, input);
        
        if (!input.empty()) {
            bool shouldExit = handleCommand(input);
            if (shouldExit) {
                ConsoleManager::getInstance()->switchConsole(MAIN_CONSOLE);
                break;
            }
        }
    }
}

void ProcessConsole::display() {
    system("cls");
    // Don't show header immediately, will be shown when commands are executed
}

void ProcessConsole::process() {
    // This method is now handled by the interactive loop in onEnabled()
}

bool ProcessConsole::handleCommand(const String& command) {
    String cmd = command;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    if (cmd == "exit") {
        return true;
    }
    else if (cmd == "process-smi" && attachedProcess) {
        showProcessInfo();
        return false;
    }
    else {
        // For any other command, just show a new prompt without error
        return false;
    }
}

void ProcessConsole::showProcessInfo() {
    std::cout << "\n";
    if (attachedProcess) {
        attachedProcess->printProcess();
        
        // Check if process is finished and show "Finished!" message
        if (attachedProcess->getStatus() == ProcessStatus::Finished) {
            std::cout << "\nFinished!\n";
        }
    } else {
        std::cout << "No process attached." << std::endl;
    }
    std::cout << "\n";
}

void ProcessConsole::exitToMain() {
    // This method is kept for potential future use
    // Currently exit is handled directly in the interactive loop
    std::cout << "Returning to main console..." << std::endl;
}

void ProcessConsole::showProcessHeader() {
    if (attachedProcess) {
        std::cout << "=== Process Screen: " << attachedProcess->getName() << " ===" << std::endl;
    } else {
        std::cout << "=== PROCESS MANAGEMENT CONSOLE ===" << std::endl;
    }
}

void ProcessConsole::showErrorMessage(const String& error) {
    std::cout << "\033[31m" << error << "\033[0m" << std::endl;
    std::cout << "> ";
} 