#include "ConsoleManager.h"
#include "MainConsole.h"
#include "ProcessConsole.h"
#include "Config.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <fstream>

MainConsole::MainConsole() : AConsole(MAIN_CONSOLE) {
    isSystemInitialized = false;
    nextProcessId = 1;
    // Don't create scheduler here - wait until config is loaded
}

void MainConsole::onEnabled() {
    // Called when switching to this console - show full interface
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
    std::getline(std::cin, input);
    
    if (!input.empty()) {
        bool shouldExit = handleCommand(input);
        if (shouldExit) {
            ConsoleManager::getInstance()->exitApplication();
            return;
        }
    }
    
    // Show prompt for next command
    std::cout << "> ";
    std::cout.flush();
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
    else if (cmd == "vmstat") {
        showMemoryStatus();
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

String MainConsole::toLower(const String& str) {
    String result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void MainConsole::initializeSystem() {
    std::cout << "Initializing system from config.txt..." << std::endl;
    
    // Load configuration from config.txt
    bool configLoaded = Config::loadFromFile("config.txt");
    if (!configLoaded) {
        std::cout << "\033[33mUsing default configuration values.\033[0m" << std::endl;
    }
    
    // Create scheduler AFTER config is loaded
    scheduler = std::make_unique<CPUScheduler>();
    
    isSystemInitialized = true;
    std::cout << "\033[32mSystem initialized successfully!\033[0m" << std::endl;
}

void MainConsole::exitSystem() {
    std::cout << "Exiting CSOPESY..." << std::endl;
}

void MainConsole::clearConsole() {
    // Only clear when user explicitly types "clear" command
    display();
}

void MainConsole::handleScreenCommand(const std::vector<String>& args) {
    if (args.size() < 2) {
        showErrorMessage("Missing screen target. Usage: screen [-s|-r|-ls] <name>");
        return;
    }

    if (args[1] == "-ls") {
        listProcesses();
    }
    else if (args[1] == "-s") {
        if (args.size() < 3) {
            showErrorMessage("Missing process name. Usage: screen -s <process>");
            return;
        }

        String processName = args[2];
        // Create the process if it doesn’t exist
        if (!scheduler->hasProcess(processName)) {
            createProcess(processName);  // <-- your existing method that handles creation
        }

        auto process = scheduler->getProcess(processName);
        String consoleName = "PROCESS_" + processName;

        // Only create and register if not already registered
        if (!ConsoleManager::getInstance()->hasConsole(consoleName)) {
            auto console = std::make_shared<ProcessConsole>(*scheduler, process);
            ConsoleManager::getInstance()->registerConsole(consoleName, console);
        }

        ConsoleManager::getInstance()->switchConsole(consoleName);

    }
    else if (args[1] == "-r") {
        if (args.size() < 3) {
            showErrorMessage("Missing process name. Usage: screen -r <process>");
            return;
        }

        String processName = args[2];
        String consoleName = "PROCESS_" + processName;

        // If already registered, just switch
        if (ConsoleManager::getInstance()->hasConsole(consoleName)) {
            ConsoleManager::getInstance()->switchConsole(consoleName);
            return;
        }

        // If not registered, but the process exists, create and register it now
        if (scheduler->hasProcess(processName)) {
            auto process = scheduler->getProcess(processName);
            auto console = std::make_shared<ProcessConsole>(*scheduler, process);
            ConsoleManager::getInstance()->registerConsole(consoleName, console);
            ConsoleManager::getInstance()->switchConsole(consoleName);
        }
        else {
            showErrorMessage("No existing process or screen for: " + processName);
        }
    }
}



void MainConsole::startScheduler() {
    if (!scheduler) {
        showUninitializedError();
        return;
    }
    if (!scheduler->isRunning()) {
        scheduler->start();
        scheduler->startProcessGeneration();
        std::cout << "\033[32mScheduler started!\033[0m" << std::endl;
    } else {
        std::cout << "\033[33mScheduler is already running.\033[0m" << std::endl;
    }
}

void MainConsole::stopScheduler() {
    if (!scheduler) {
        showUninitializedError();
        return;
    }
    if (scheduler->isRunning()) {
        scheduler->stop();
        std::cout << "\033[33mScheduler stopping... (finishing existing processes)\033[0m" << std::endl;
    } else {
        std::cout << "\033[33mScheduler is not running.\033[0m" << std::endl;
    }
}

void MainConsole::generateReport() {
    if (!scheduler) {
        showUninitializedError();
        return;
    }
    
    std::cout << "Generating CPU utilization report..." << std::endl;
    
    std::ofstream reportFile("csopesy-log.txt");
    if (!reportFile.is_open()) {
        showErrorMessage("Could not create csopesy-log.txt");
        return;
    }
    
    // Get real CPU utilization from scheduler
    double cpuUtil = scheduler->getCpuUtilization();
    int coresUsed = scheduler->getCoresUsed();
    int coresAvailable = scheduler->getCoresAvailable();
    
    // Write report header
    reportFile << "CPU utilization: " << std::fixed << std::setprecision(0) << cpuUtil << "%" << std::endl;
    reportFile << "Cores used: " << coresUsed << std::endl;
    reportFile << "Cores available: " << coresAvailable << std::endl;
    reportFile << "--------------------------------------" << std::endl;
    
    // Get processes by status from scheduler (report generation)
    auto runningProcesses = scheduler->getProcessesByStatus(ProcessStatus::Running);
    auto waitingProcesses = scheduler->getProcessesByStatus(ProcessStatus::Waiting);
    auto sleepingProcesses = scheduler->getProcessesByStatus(ProcessStatus::Sleeping);
    auto finishedProcesses = scheduler->getProcessesByStatus(ProcessStatus::Finished);
    
    // Write running processes
    reportFile << "\nRunning processes:" << std::endl;
    for (const String& processName : runningProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process && process->getAssignedCore() >= 0) {  // Safety check: only show processes with valid cores
            int currentLine = process->getTotalInstructions() - process->getRemainingInstructions();
            reportFile << processName << "\t(" << process->getCreationTime() << ")\t"
                      << "Core:" << process->getAssignedCore() << "\t"
                      << currentLine << " / " << process->getTotalInstructions() << std::endl;
        }
    }
    
    // Write waiting processes
    reportFile << "\nWaiting processes:" << std::endl;
    for (const String& processName : waitingProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process) {
            int currentLine = process->getTotalInstructions() - process->getRemainingInstructions();
            reportFile << processName << "\t(" << process->getCreationTime() << ")\t"
                      << currentLine << " / " << process->getTotalInstructions() << std::endl;
        }
    }
    
    // Write sleeping processes (they're also waiting but sleeping)
    for (const String& processName : sleepingProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process) {
            int currentLine = process->getTotalInstructions() - process->getRemainingInstructions();
            reportFile << processName << "\t(" << process->getCreationTime() << ")\t"
                      << currentLine << " / " << process->getTotalInstructions() << " (sleeping)" << std::endl;
        }
    }
    
    // Write finished processes
    reportFile << "\nFinished processes:" << std::endl;
    for (const String& processName : finishedProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process) {
            reportFile << processName << "\t(" << process->getCreationTime() << ")\t"
                      << "Finished\t" << process->getTotalInstructions() 
                      << " / " << process->getTotalInstructions() << std::endl;
        }
    }
    
    reportFile << "--------------------------------------" << std::endl;
    reportFile.close();
    
    std::cout << "\033[32mReport generated: csopesy-log.txt\033[0m" << std::endl;
}

void MainConsole::showMemoryStatus() {
    if (!scheduler) {
        showUninitializedError();
        return;
    }
    
    scheduler->printMemoryStatus();
}

void MainConsole::createProcess(const String& processName) {
    if (!scheduler) {
        showUninitializedError();
        return;
    }
    
    // Check if process already exists
    if (scheduler->hasProcess(processName)) {
        showErrorMessage("Process " + processName + " already exists.");
        return;
    }
    
    std::cout << "Creating process: " << processName << std::endl;
    
    // Use config values for instruction count
    int minIns = Config::getMinIns();
    int maxIns = Config::getMaxIns();
    int numInstructions = minIns + (rand() % (maxIns - minIns + 1));
    
    auto newProcess = std::make_shared<Process>(processName, nextProcessId++, numInstructions);
    
    // Add to scheduler (which now handles everything)
    scheduler->addProcess(newProcess);
    
    std::cout << "\033[32mProcess " << processName << " created successfully!\033[0m" << std::endl;
    std::cout << "Instructions: " << numInstructions << " | ID: " << newProcess->getId() << std::endl;
}

void MainConsole::listProcesses() {
    if (!scheduler) {
        showUninitializedError();
        return;
    }
    
    // Get real CPU utilization from scheduler
    double cpuUtil = scheduler->getCpuUtilization();
    int coresUsed = scheduler->getCoresUsed();
    int coresAvailable = scheduler->getCoresAvailable();
    
    std::cout << "CPU utilization: " << std::fixed << std::setprecision(0) << cpuUtil << "%" << std::endl;
    std::cout << "Cores used: " << coresUsed << std::endl;
    std::cout << "Cores available: " << coresAvailable << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    
    // Get processes by status from scheduler (listProcesses)
    auto runningProcesses = scheduler->getProcessesByStatus(ProcessStatus::Running);
    auto waitingProcesses = scheduler->getProcessesByStatus(ProcessStatus::Waiting);
    auto sleepingProcesses = scheduler->getProcessesByStatus(ProcessStatus::Sleeping);
    auto finishedProcesses = scheduler->getProcessesByStatus(ProcessStatus::Finished);
    
    std::cout << "\nRunning processes:" << std::endl;
    for (const String& processName : runningProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process && process->getAssignedCore() >= 0) {  // Safety check: only show processes with valid cores
            int currentLine = process->getTotalInstructions() - process->getRemainingInstructions();
            std::cout << processName << "\t(" << process->getCreationTime() << ")\t"
                     << "Core:" << process->getAssignedCore() << "\t"
                     << currentLine << " / " << process->getTotalInstructions() << std::endl;
        }
    }
    
    std::cout << "\nWaiting processes:" << std::endl;
    for (const String& processName : waitingProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process) {
            int currentLine = process->getTotalInstructions() - process->getRemainingInstructions();
            std::cout << processName << "\t(" << process->getCreationTime() << ")\t"
                     << currentLine << " / " << process->getTotalInstructions() << std::endl;
        }
    }
    
    // Show sleeping processes (they're also waiting but sleeping)
    for (const String& processName : sleepingProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process) {
            int currentLine = process->getTotalInstructions() - process->getRemainingInstructions();
            std::cout << processName << "\t(" << process->getCreationTime() << ")\t"
                     << currentLine << " / " << process->getTotalInstructions() << " (sleeping)" << std::endl;
        }
    }
    
    std::cout << "\nFinished processes:" << std::endl;
    for (const String& processName : finishedProcesses) {
        auto process = scheduler->getProcess(processName);
        if (process) {
            std::cout << processName << "\t(" << process->getCreationTime() << ")\t"
                     << "Finished\t" << process->getTotalInstructions() 
                     << " / " << process->getTotalInstructions() << std::endl;
        }
    }
    
    std::cout << "--------------------------------------" << std::endl;
}

void MainConsole::attachToProcess(const String& processName) {
    if (!scheduler) {
        showUninitializedError();
        return;
    }

    if (!scheduler->hasProcess(processName)) {
        showErrorMessage("Process " + processName + " not found.");
        return;
    }

    auto process = scheduler->getProcess(processName);
    if (process->getStatus() == ProcessStatus::Finished) {
        showErrorMessage("Process " + processName + " already finished.");
        return;
    }

    String consoleName = "PROCESS_" + processName;
    auto processConsole = std::make_shared<ProcessConsole>(*scheduler, process);
    ConsoleManager::getInstance()->registerConsole(consoleName, processConsole);
    ConsoleManager::getInstance()->switchConsole(consoleName);
}


void MainConsole::showWelcomeMessage() {
    std::cout << "\033[32mHello, Welcome to CSOPESY commandline!\033[0m" << std::endl;
    std::cout << "\nDevelopers: \n Ambrosio, Lorenzo Aivin F. \n Larraquel, Reign Elaiza D.\n" << std::endl;
    std::cout << "Last Updated: 06-28-2025\n" << std::endl;
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
    std::cout << "> ";
    std::cout.flush();  // Ensure prompt appears immediately
}

void MainConsole::showErrorMessage(const String& error) {
    std::cout << "\033[31m" << error << "\033[0m" << std::endl;
}

void MainConsole::showUninitializedError() {
    std::cout << "\033[31mError: System not initialized. Please run 'initialize' first.\033[0m" << std::endl;
} 