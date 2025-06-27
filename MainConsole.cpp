#include "ConsoleManager.h"
#include "MainConsole.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

MainConsole::MainConsole() : AConsole(MAIN_CONSOLE) {
    isSystemInitialized = false;
}

void MainConsole::onEnabled() {
    // Called when switching to this console
    display();
}

void MainConsole::display() {
    ConsoleManager::getInstance()->setCursorPosition(0, 0);
    system("cls");
    showLogo();
    showWelcomeMessage();
    showCommandPrompt();
}

void MainConsole::process() {
    String input;
    std::cout << "> ";
    std::getline(std::cin, input);
    
    if (!input.empty()) {
        bool shouldExit = handleCommand(input);
        if (shouldExit) {
            ConsoleManager::getInstance()->exitApplication();
        }
    }
}

bool MainConsole::handleCommand(const String& command) {
    auto args = parseCommand(command);
    if (args.empty()) return false;
    
    String cmd = toLower(args[0]);
    
    if (cmd == "exit") {
        exitSystem();
        return true;
    }
    
    if (cmd == "initialize") {
        initializeSystem();
        return false;
    }
    
    // Block all other commands until initialized
    if (!isSystemInitialized) {
        showUninitializedError();
        return false;
    }
    
    // Handle other commands
    if (cmd == "clear") {
        clearConsole();
    }
    else if (cmd == "screen") {
        handleScreenCommand(args);
    }
    else if (cmd == "scheduler-start") {
        startScheduler();
    }
    else if (cmd == "scheduler-stop") {
        stopScheduler();
    }
    else if (cmd == "report-util") {
        generateReport();
    }
    else {
        showErrorMessage("Unknown command: " + command);
    }
    
    return false;
}

std::vector<String> MainConsole::parseCommand(const String& command) {
    std::vector<String> args;
    std::istringstream iss(command);
    String arg;
    
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    return args;
}

void MainConsole::initializeSystem() {
    std::cout << "Initializing system from config.txt..." << std::endl;
    
    // TODO: Read config.txt and initialize system parameters
    isSystemInitialized = true;
    
    std::cout << "\033[32mSystem initialized successfully!\033[0m" << std::endl;
}

void MainConsole::exitSystem() {
    std::cout << "Exiting CSOPESY..." << std::endl;
}

void MainConsole::clearConsole() {
    display();
}

void MainConsole::handleScreenCommand(const std::vector<String>& args) {
    if (args.size() < 2) {
        showErrorMessage("Usage: screen -s <name> | screen -ls | screen -r <name>");
        return;
    }
    
    String flag = args[1];
    
    if (flag == "-ls") {
        listProcesses();
    }
    else if (flag == "-s" && args.size() >= 3) {
        createProcess(args[2]);
    }
    else if (flag == "-r" && args.size() >= 3) {
        attachToProcess(args[2]);
    }
    else {
        showErrorMessage("Invalid screen command");
    }
}

void MainConsole::startScheduler() {
    std::cout << "Starting process scheduler..." << std::endl;
    // TODO: Implement scheduler starting logic
    std::cout << "\033[32mScheduler started!\033[0m" << std::endl;
}

void MainConsole::stopScheduler() {
    std::cout << "Stopping process scheduler..." << std::endl;
    // TODO: Implement scheduler stopping logic
    std::cout << "\033[32mScheduler stopped!\033[0m" << std::endl;
}

void MainConsole::generateReport() {
    std::cout << "Generating CPU utilization report..." << std::endl;
    // TODO: Implement report generation
    std::cout << "\033[32mReport generated: csopesy-log.txt\033[0m" << std::endl;
}

void MainConsole::createProcess(const String& processName) {
    std::cout << "Creating process: " << processName << std::endl;
    // TODO: Create process and add to scheduler
    std::cout << "\033[32mProcess " << processName << " created successfully!\033[0m" << std::endl;
}

void MainConsole::listProcesses() {
    std::cout << "\n=== Process List ===" << std::endl;
    std::cout << "CPU Utilization: 0%" << std::endl;
    std::cout << "Cores used: 0" << std::endl;
    std::cout << "Cores available: 4" << std::endl;
    std::cout << "\nRunning processes:" << std::endl;
    // TODO: List actual running processes
    std::cout << "No running processes." << std::endl;
    
    std::cout << "\nFinished processes:" << std::endl;
    // TODO: List actual finished processes
    std::cout << "No finished processes." << std::endl;
    std::cout << "===================" << std::endl;
}

void MainConsole::attachToProcess(const String& processName) {
    // TODO: Check if process exists
    std::cout << "Attaching to process: " << processName << std::endl;
    
    // For now, show error since we haven't implemented process consoles yet
    showErrorMessage("Process " + processName + " not found.");
    
    // TODO: Switch to process console when implemented
    // ConsoleManager::getInstance()->switchConsole("PROCESS_" + processName);
}

void MainConsole::showWelcomeMessage() {
    std::cout << "\033[32mHello, Welcome to CSOPESY commandline!\033[0m" << std::endl;
    std::cout << "Type \033[33m'exit'\033[0m to quit, \033[33m'clear'\033[0m to clear the screen" << std::endl;
    
    if (!isSystemInitialized) {
        std::cout << "\033[31mSystem not initialized. Please run 'initialize' first.\033[0m" << std::endl;
    }
    
    std::cout << "Enter a command:" << std::endl;
}

void MainConsole::showLogo() {
    std::cout << "  ___  ____   __   ____  ____  ____  _  _ " << std::endl;
    std::cout << " / __)/ ___) /  \\ (  _ \\(  __)/ ___)( \\/ )" << std::endl;
    std::cout << "( (__ \\___ \\(  O ) ) __/ ) _) \\___ \\ )  / " << std::endl;
    std::cout << " \\___)(____/ \\__/ (__)  (____)(____/(__/  " << std::endl;
    std::cout << std::endl;
}

void MainConsole::showCommandPrompt() {
    // Command prompt is shown in process() method
}

void MainConsole::showErrorMessage(const String& error) {
    std::cout << "\033[31m" << error << "\033[0m" << std::endl;
}

void MainConsole::showUninitializedError() {
    std::cout << "\033[31mError: System not initialized. Please run 'initialize' first.\033[0m" << std::endl;
}

String MainConsole::toLower(const String& str) {
    String result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
} 