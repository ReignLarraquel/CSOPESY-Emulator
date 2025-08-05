#define NOMINMAX
#include "ConsoleManager.h"
#include "MainConsole.h"
#include "ProcessConsole.h"
#include "MemoryManager.h"
#include "Config.h"
#include <algorithm>
#include <iostream>
#include <sstream>
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

String toString(ProcessStatus status) {
    switch (status) {
    case ProcessStatus::Running: return "Running";
    case ProcessStatus::Waiting: return "Waiting";
    case ProcessStatus::Sleeping: return "Sleeping";
    case ProcessStatus::Finished: return "Finished";
    default: return "Unknown";
    }
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
    else if (cmd == "process-smi") {
        showProcessSMI();
    }
    else if (cmd == "vmstat") {
        showMemoryStatus();
    }
    else if (cmd == "backing-store-dump") {
        scheduler->dumpBackingStoreToFile();
        std::cout << "Backing store dumped to csopesy-backing-store.txt\n";
    }
    else {
        showErrorMessage("Unknown command: " + command);
    }
    
    return false;
}

std::vector<String> MainConsole::parseCommand(const String& command) {
    std::vector<String> args;
    String currentArg;
    bool inQuotes = false;
    char quoteChar = '\0';
    

    for (size_t i = 0; i < command.length(); ++i) {
        char c = command[i];
        
        if (!inQuotes && (c == '"' || c == '\'')) {
            inQuotes = true;
            quoteChar = c;
        } else if (inQuotes && c == '\\' && i + 1 < command.length() && command[i + 1] == quoteChar) {
            currentArg += quoteChar;
            i++;
        } else if (inQuotes && c == quoteChar) {
            inQuotes = false;
            quoteChar = '\0';
        } else if (!inQuotes && std::isspace(c)) {
            if (!currentArg.empty()) {
                args.push_back(currentArg);
                currentArg.clear();
            }
        } else {
            currentArg += c;
        }
    }
    
    if (!currentArg.empty()) {
        args.push_back(currentArg);
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

    bool configLoaded = Config::loadFromFile("config.txt");
    if (!configLoaded) {
        std::cout << "\033[33mUsing default configuration values.\033[0m" << std::endl;
    }

    scheduler = std::make_unique<CPUScheduler>();

    int coreCount = Config::getNumCpu();
    coreManager = std::make_shared<CoreManager>(coreCount);

    scheduler->startCpuExecution();

    isSystemInitialized = true;
    std::cout << "\033[32mSystem initialized successfully!\033[0m" << std::endl;
}

void MainConsole::exitSystem() {
    std::cout << "Exiting CSOPESY..." << std::endl;

    if (scheduler) {
        scheduler->stopCpuExecution();
    }
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
    else if (args[1] == "-c") {

        if (args.size() != 4) {
            showErrorMessage("Invalid command. Usage: screen -c <process_name> \"<instructions>\"");
            return;
        }

        String processName = args[2];
        String instructionsStr = args[3];

        int memorySize = Config::getMemPerProc();

        
        if (!instructionsStr.empty() && instructionsStr.front() == '"' && instructionsStr.back() == '"') {
            instructionsStr = instructionsStr.substr(1, instructionsStr.length() - 2);
        }

        // Create process with custom instructions
        try {
            int numInstructions = 50; // Default for custom instructions
            auto newProcess = std::make_shared<Process>(
                processName, scheduler->getNextProcessId(), numInstructions, memorySize, instructionsStr);

            scheduler->addProcess(newProcess);

            std::cout << "\033[32mProcess " << processName << " created with custom instructions!\033[0m\n";
            std::cout << "Memory: " << memorySize << " bytes | ID: " << newProcess->getId() << std::endl;


            std::cout << "Process added to scheduler queue.\033[0m\n";
        }
        catch (const std::exception& e) {
            showErrorMessage(String("Failed to create process: ") + e.what());
        }
    }
    else if (args[1] == "-s") {
        if (args.size() < 4) {
            showErrorMessage("Missing process name or memory size. Usage: screen -s <process_name> <memory_size>");
            return;
        }

        String processName = args[2];
        int memorySize = std::stoi(args[3]);

        if (memorySize < 64 || memorySize > 65536 || (memorySize & (memorySize - 1)) != 0) {
            showErrorMessage("invalid memory allocation");
            return;
        }

        // Create the process if it doesn’t exist
        if (!scheduler->hasProcess(processName)) {
            createProcess(processName, memorySize);
        }

        auto process = scheduler->getProcess(processName);
        String consoleName = "PROCESS_" + processName;

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

        if (ConsoleManager::getInstance()->hasConsole(consoleName)) {
            ConsoleManager::getInstance()->switchConsole(consoleName);
            return;
        }

        if (scheduler->hasProcess(processName)) {
            auto process = scheduler->getProcess(processName);
            
            // Check if process was terminated due to memory access violation
            if (process->wasTerminatedDueToMemoryViolation()) {
                // Extract the time part from the timestamp (format: "(MM/DD/YYYY hh:mm:ssAM/PM)")
                std::string timestamp = process->getMemoryViolationTimestamp();
                size_t timeStart = timestamp.find(' ', timestamp.find(' ') + 1) + 1; // Find second space and add 1
                std::string timeOnly = timestamp.substr(timeStart, 
                    timestamp.find(')', timeStart) - timeStart);
                
                std::ostringstream hexAddr;
                hexAddr << "0x" << std::hex << std::uppercase << process->getMemoryViolationAddress();
                
                showErrorMessage("Process " + processName + 
                    " shut down due to memory access violation error that occurred at " + 
                    timeOnly + ". " + hexAddr.str() + " invalid.");
                return;
            }
            

            // Create and switch to process console (works for both running and finished processes)
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

    scheduler->start();
    std::cout << "\033[32mAutomatic process generation started!\033[0m" << std::endl;
}

void MainConsole::stopScheduler() {
    if (!scheduler) {
        showUninitializedError();
        return;
    }

    scheduler->stop();
    std::cout << "\033[33mAutomatic process generation stopped.\033[0m" << std::endl;
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
    if (!scheduler || !coreManager) {
        showUninitializedError();
        return;
    }

    MemoryManager& mm = scheduler->getMemoryManager();  // Get the one inside the scheduler

    int totalMem = Config::getMaxOverallMem();
    int frameSize = Config::getMemPerFrame();
    int totalPages = mm.getTotalFrames();
    int usedPages = mm.getUsedFrameCount();
    int freePages = mm.getFreeFrameCount();
    int usedMem = usedPages * frameSize;
    int freeMem = freePages * frameSize;

    std::cout << "\n=== VMSTAT ===" << std::endl;

    // Memory
    std::cout << "Memory Info:" << std::endl;
    std::cout << "Total: " << totalMem << " bytes" << std::endl;
    std::cout << "Used : " << usedMem << " bytes" << std::endl;
    std::cout << "Free : " << freeMem << " bytes" << std::endl;

    // Pages
    std::cout << "Pages:" << std::endl;
    std::cout << "Total: " << totalPages << " pages" << std::endl;
    std::cout << "Used : " << usedPages << " pages" << std::endl;
    std::cout << "Free : " << freePages << " pages" << std::endl;
    std::cout << "Page Size: " << frameSize << " bytes\n" << std::endl;

    // CPU
    std::cout << "CPU Ticks:" << std::endl;
    std::cout << "Active : " << coreManager->getActiveTicks() << std::endl;
    std::cout << "Idle   : " << coreManager->getIdleTicks() << std::endl;
    std::cout << "Total  : " << coreManager->getTotalTicks() << std::endl;

    // Paging
    std::cout << "\nPaging:" << std::endl;
    std::cout << "Pages Paged In : " << mm.getPagedInCount() << std::endl;
    std::cout << "Pages Paged Out: " << mm.getPagedOutCount() << std::endl;

    std::cout << "\nProcesses by status:" << std::endl;
    std::cout << "Running : " << scheduler->getProcessesByStatus(ProcessStatus::Running).size() << std::endl;
    std::cout << "Waiting : " << scheduler->getProcessesByStatus(ProcessStatus::Waiting).size() << std::endl;
    std::cout << "Sleeping: " << scheduler->getProcessesByStatus(ProcessStatus::Sleeping).size() << std::endl;
    std::cout << "Finished: " << scheduler->getProcessesByStatus(ProcessStatus::Finished).size() << std::endl;

    std::cout << "===================" << std::endl;
}

void MainConsole::showProcessSMI() {
    if (!scheduler) {
        showUninitializedError();
        return;
    }

    std::map<String, int> usedPagesPerProcess;
    int totalUsedPages = 0;

    for (const String& name : scheduler->getAllProcessNames()) {
        auto proc = scheduler->getProcess(name);
        if (!proc) continue;

        int usedPages = 0;
        for (const auto& [pageNum, entry] : proc->getPageTable()) {
            if (entry.valid) usedPages++;
        }

        if (usedPages > 0) {
            usedPagesPerProcess[name] = usedPages;
            totalUsedPages += usedPages;
        }
    }

    int totalMem = Config::getMaxOverallMem();
    int frameSize = Config::getMemPerFrame();
    int totalFrames = totalMem / frameSize;

    // Cap used pages to the number of physical frames
    int cappedUsedPages = std::min(totalUsedPages, totalFrames);
    int usedMem = cappedUsedPages * frameSize;
    int freeMem = totalMem - usedMem;

    float memUtil = (totalMem == 0) ? 0 : ((float)usedMem / totalMem) * 100;

    int activeCores = scheduler->getCoreManager().getUsedCoreCount();
    int totalCores = scheduler->getCoreManager().getCoreCount();
    float cpuUtil = (totalCores == 0) ? 0 : ((float)activeCores / totalCores) * 100;


    std::cout << "PROCESS-SMI V01.00 Driver Version: 01.00" << std::endl;
    std::cout << "CPU-Util: " << std::fixed << std::setprecision(0) << cpuUtil << "%" << std::endl;
    std::cout << "Memory Usage: " << usedMem << "MiB / " << totalMem << "MiB" << std::endl;
    std::cout << "Memory Util: " << std::fixed << std::setprecision(0) << memUtil << "%" << std::endl;
    std::cout << std::endl;
    std::cout << "Running processes and memory usage:" << std::endl;
    
    for (const auto& [procName, pageCount] : usedPagesPerProcess) {
        int memUsageMB = (pageCount * frameSize) / 1024;
        std::cout << procName << " " << memUsageMB << "MiB" << std::endl;
    }
}


void MainConsole::createProcess(const String& processName, int memorySize) {
    if (!scheduler) {
        showUninitializedError();
        return;
    }

    if (scheduler->hasProcess(processName)) {
        showErrorMessage("Process " + processName + " already exists.");
        return;
    }

    std::cout << "Creating process: " << processName << " with " << memorySize << " bytes of memory\n";

    int minIns = Config::getMinIns();
    int maxIns = Config::getMaxIns();
    int numInstructions = minIns + (rand() % (maxIns - minIns + 1));

    auto newProcess = std::make_shared<Process>(processName, scheduler->getNextProcessId(), numInstructions, memorySize);
    scheduler->addProcess(newProcess);

    std::cout << "\033[32mProcess " << processName << " created successfully!\033[0m\n";
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
    std::cout << "\nDevelopers: \n Ambrosio, Lorenzo Aivin F. \n Larraquel, Reign Elaiza D.\n Cruz, Giovanni Jonathan R.\n" << std::endl;
    std::cout << "Last Updated: 08-05-2025\n" << std::endl;
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