#include "ProcessConsole.h"
#include "ConsoleManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

ProcessConsole::ProcessConsole(CPUScheduler& scheduler, std::shared_ptr<Process> process)
    : AConsole(process ? ("PROCESS_" + process->getName()) : "PROCESS_CONSOLE"),
    scheduler(scheduler), attachedProcess(process) {
}

void ProcessConsole::onEnabled() {
    display();

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
    // Don’t show header immediately
}

void ProcessConsole::process() {
    // Interactive handled in onEnabled()
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
        return false;
    }
}

void ProcessConsole::showProcessInfo() {
    std::cout << "\n";
    if (attachedProcess) {
        attachedProcess->printProcess();

        if (attachedProcess->getStatus() == ProcessStatus::Finished) {
            std::cout << "\nFinished!\n";
        }
    }
    else {
        std::cout << "No process attached.\n";
    }
    std::cout << "\n";
}

void ProcessConsole::exitToMain() {
    std::cout << "Returning to main console...\n";
}

void ProcessConsole::showProcessHeader() {
    if (attachedProcess) {
        std::cout << "=== Process Screen: " << attachedProcess->getName() << " ===\n";
    }
    else {
        std::cout << "=== PROCESS MANAGEMENT CONSOLE ===\n";
    }
}

void ProcessConsole::showErrorMessage(const String& error) {
    std::cout << "\033[31m" << error << "\033[0m\n";
    std::cout << "> ";
}
