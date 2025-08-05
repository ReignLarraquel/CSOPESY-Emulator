#define NOMINMAX
#include "ProcessConsole.h"
#include "ConsoleManager.h"
#include "process.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>

ProcessConsole::ProcessConsole(CPUScheduler& scheduler, std::shared_ptr<Process> process)
    : AConsole(process ? ("PROCESS_" + process->getName()) : "PROCESS_CONSOLE"),
    scheduler(scheduler), attachedProcess(process) {
}

void ProcessConsole::onEnabled() {
    // Check if process was terminated due to memory access violation before displaying console
    if (attachedProcess && attachedProcess->wasTerminatedDueToMemoryViolation()) {
        // Extract time information
        std::string timestamp = attachedProcess->getMemoryViolationTimestamp();
        size_t timeStart = timestamp.find(' ', timestamp.find(' ') + 1) + 1;
        std::string timeOnly = timestamp.substr(timeStart, timestamp.find(')', timeStart) - timeStart);
        
        // Format the hex address
        std::ostringstream hexAddr;
        hexAddr << "0x" << std::hex << std::uppercase << attachedProcess->getMemoryViolationAddress();
        
        // Show error message with improved visibility
        system("cls"); // Clear screen first for better visibility
        std::cout << "\n\n\033[1;41m  MEMORY ACCESS VIOLATION ERROR  \033[0m\n\n";
        std::cout << "\033[1;31mProcess " << attachedProcess->getName() << 
            " shut down due to memory access violation error\033[0m\n";
        std::cout << "Time of violation: " << timeOnly << "\n";
        std::cout << "Invalid memory address: " << hexAddr.str() << "\n\n";
        std::cout << "Press Enter to return to main console...";
        
        // Wait for user to press Enter before returning to main console
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            
        // Return to main console
        ConsoleManager::getInstance()->switchConsole(MAIN_CONSOLE);
        return;
    }

    // TOBEDELETED: Fixed screen switching - display console and let main loop handle input
    display();
}

void ProcessConsole::display() {
    // TOBEDELETED: Revert - clear screen when switching to process console
    system("cls");
    
    std::cout << "----------------------------------------\n";
    std::cout << "Process Console: " << (attachedProcess ? attachedProcess->getName() : "Unknown") << "\n";
    std::cout << "----------------------------------------\n\n";
    
    // TOBEDELETED: CRITICAL FIX - Automatically show process logs including PRINT output!
    if (attachedProcess) {
        std::cout << "Process Logs:\n";
        attachedProcess->displayLogs();
        std::cout << "\n";
        
        // Show process status
        if (attachedProcess->hasFinished()) {
            std::cout << "Status: FINISHED\n\n";
        } else {
            std::cout << "Status: " 
                     << (attachedProcess->getStatus() == ProcessStatus::Running ? "RUNNING" :
                         attachedProcess->getStatus() == ProcessStatus::Waiting ? "WAITING" : "SLEEPING")
                     << " - Current line: " << (attachedProcess->getTotalInstructions() - attachedProcess->getRemainingInstructions())
                     << " / " << attachedProcess->getTotalInstructions() << "\n\n";
        }
    }
}

void ProcessConsole::process() {
    // TOBEDELETED: Handle input properly for ProcessConsole
    std::cout << "\033[1;32mroot:\\>\033[0m "; // Green and bold prompt
    std::cout.flush();
    
    String input;
    std::getline(std::cin, input);
    
    if (!input.empty()) {
        bool shouldExit = handleCommand(input);
        if (shouldExit) {
            ConsoleManager::getInstance()->switchConsole(MAIN_CONSOLE);
        } else {
            // TOBEDELETED: Refresh display after any command to show new PRINT output
            display();
        }
    }
}

bool ProcessConsole::handleCommand(const String& command) {
    // Parse command into arguments
    std::vector<String> args;
    std::istringstream iss(command);
    String arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    // Extract the first word as the command
    String cmd = args.empty() ? "" : args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "exit") {
        return true;
    }
    else if (cmd == "process-smi" && attachedProcess) {
        showProcessInfo();
        return false;
    }
    else if (cmd == "refresh" || cmd == "logs") {
        // TOBEDELETED: Manual refresh command to see latest PRINT output
        // display() will be called automatically after this returns
        return false;
    }
    // Memory dump command
    else if (cmd == "memory-dump") {
        showProcessMemoryDump();
        return false;
    }
    // Legacy memory read command
    else if (cmd == "memory-read" && args.size() >= 2) {
        std::string addressStr = args[1];
        uint32_t address;
        try {
            address = std::stoul(addressStr, nullptr, 16);
            if (attachedProcess) {
                uint16_t value = attachedProcess->getMemoryValueAt(address);
                std::cout << "Memory at " << addressStr << ": " << value << std::endl;
            }
        }
        catch (...) {
            std::cout << "Invalid address format. Use hex format (e.g., 0x1000)" << std::endl;
        }
        return false;
    }
    // Legacy memory write command
    else if (cmd == "memory-write" && args.size() >= 3) {
        std::string addressStr = args[1];
        std::string valueStr = args[2];
        try {
            uint32_t address = std::stoul(addressStr, nullptr, 16);
            uint16_t value = static_cast<uint16_t>(std::stoul(valueStr));

            if (attachedProcess) {
                bool success = attachedProcess->setMemoryValueAt(address, value);
                if (success) {
                    std::cout << "Wrote value " << value << " to address " << addressStr << std::endl;
                }
                else {
                    std::cout << "Failed to write to address: " << addressStr << std::endl;
                }
            }
        }
        catch (...) {
            std::cout << "Invalid format. Use: memory-write <hex_address> <decimal_value>" << std::endl;
        }
        return false;
    }
    // New READ command: READ <variable_name> <address>
    else if (cmd == "read" && args.size() >= 3) {
        std::string varName = args[1];
        std::string addressStr = args[2];
        uint32_t address;
        try {
            address = std::stoul(addressStr, nullptr, 16);
            if (attachedProcess) {
                uint16_t value = attachedProcess->getMemoryValueAt(address);

                // Store the value in the variable - using the public method instead
                attachedProcess->setVariable(varName, value);
                std::cout << "READ " << varName << " = " << value << " from " << addressStr << std::endl;
            }
        }
        catch (...) {
            std::cout << "Invalid address format. Use hex format (e.g., 0x1000)" << std::endl;
        }
        return false;
    }
    // New WRITE command: WRITE <address> <value>
    else if (cmd == "write" && args.size() >= 3) {
        std::string addressStr = args[1];
        std::string valueStr = args[2];
        try {
            uint32_t address = std::stoul(addressStr, nullptr, 16);
            uint16_t value = static_cast<uint16_t>(std::stoul(valueStr));

            if (attachedProcess) {
                bool success = attachedProcess->setMemoryValueAt(address, value);
                if (success) {
                    std::cout << "WRITE " << value << " to " << addressStr << std::endl;
                }
                else {
                    std::cout << "Failed to write to address: " << addressStr << std::endl;
                    
                    // Add these lines to properly set the violation flags when write fails
                    if (!attachedProcess->isValidMemoryAccess(address)) {
                        // Set memory violation flags in the Process object
                        attachedProcess->markAsMemoryViolation(address);
                        
                        // Show error message with improved visibility
                        system("cls"); // Clear screen first for better visibility
                        std::cout << "\n\n\033[1;41m  MEMORY ACCESS VIOLATION ERROR  \033[0m\n\n";
                        std::cout << "\033[1;31mProcess " << attachedProcess->getName() << 
                            " shut down due to memory access violation error\033[0m\n";
                            
                        // Format the hex address
                        std::ostringstream hexAddr;
                        hexAddr << "0x" << std::hex << std::uppercase << address;
                        
                        std::string timestamp = attachedProcess->getTimestamp();
                        size_t timeStart = timestamp.find(' ', timestamp.find(' ') + 1) + 1;
                        std::string timeOnly = timestamp.substr(timeStart, timestamp.find(')', timeStart) - timeStart);
                        
                        std::cout << "Time of violation: " << timeOnly << "\n";
                        std::cout << "Invalid memory address: " << hexAddr.str() << "\n\n";
                        std::cout << "Press Enter to return to main console...";
                        
                        // Wait for user to press Enter before returning to main console
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        
                        // Exit the console
                        return true;
                    }
                }
            }
        }
        catch (...) {
            std::cout << "Invalid format. Use: WRITE <hex_address> <decimal_value>" << std::endl;
        }
        return false;
    }

    return false;
}

void ProcessConsole::showProcessMemoryDump() {
    if (!attachedProcess) return;

    std::cout << "\n=== Process Memory Dump ===\n";

    // Get memory dump from process
    auto memoryDump = attachedProcess->getMemoryDump();

    if (memoryDump.empty()) {
        std::cout << "No memory values currently stored.\n";
        return;
    }

    std::cout << "Address      | Value\n";
    std::cout << "-------------|--------\n";

    // Sort addresses for display
    std::vector<uint32_t> addresses;
    for (const auto& [addr, _] : memoryDump) {
        addresses.push_back(addr);
    }
    std::sort(addresses.begin(), addresses.end());

    for (uint32_t addr : addresses) {
        std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " | " << std::dec << memoryDump.at(addr) << "\n";
    }
    std::cout << "\n";
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