#include "process.h"
#include "Config.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <random>
#include <algorithm>


    // Constructor: initializes all fields and generates random instructions
    Process::Process(const std::string& processName, int processId, int numInstructions)
        : name(processName), id(processId), totalInstructions(numInstructions), 
          remainingInstructions(numInstructions), status(ProcessStatus::Waiting), 
          assignedCore(-1), currentInstructionIndex(0), sleepCyclesRemaining(0) {
        creationTime = getTimestamp();
        // Reserve space for potential FOR_END instructions within the requested instruction count
        int maxForLoops = 3; // Maximum nesting level
        int reservedForEndInstructions = maxForLoops;
        int actualInstructionsToGenerate = numInstructions - reservedForEndInstructions;
        generateRandomInstructions(actualInstructionsToGenerate);
        
        // If we didn't use all reserved FOR_END slots, fill remaining with PRINT instructions
        int unusedSlots = numInstructions - static_cast<int>(instructions.size());
        for (int i = 0; i < unusedSlots; i++) {
            Instruction instr(InstructionType::PRINT);
            instr.arg1 = "Hello world from " + name + "!";
            instructions.push_back(instr);
        }
    }

    void Process::printProcess() const {
        std::cout << "Process name: " << name << "\n";
        std::cout << "ID: " << id << "\n";

        std::cout << "Logs:\n";
        displayLogs();

        if (hasFinished()) {
            std::cout << "\nFinished!\n";
        }
        else {
            std::cout << "\nCurrent instruction line: "
                << (totalInstructions - remainingInstructions) << "\n"
                << "Lines of code: "
                << totalInstructions << "\n";
        }
        std::cout << std::endl;
    }

    // Get a timestamp string in format (MM/DD/YYYY hh:mm:ssAM/PM)
    std::string Process::getTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm local;
        
#ifdef _WIN32
        localtime_s(&local, &t);
#else
        localtime_r(&t, &local);
#endif
        std::ostringstream oss;
        oss << '('
            << std::setw(2) << std::setfill('0') << (local.tm_mon + 1) << '/'
            << std::setw(2) << std::setfill('0') << local.tm_mday << '/'
            << (local.tm_year + 1900) << ' '
            << std::setw(2) << std::setfill('0')
            << ((local.tm_hour % 12 == 0) ? 12 : local.tm_hour % 12)
            << ':'
            << std::setw(2) << std::setfill('0') << local.tm_min << ':'
            << std::setw(2) << std::setfill('0') << local.tm_sec
            << (local.tm_hour < 12 ? "AM" : "PM")
            << ')';
        return oss.str();
    }

    // Displays all collected log entries
    void Process::displayLogs() const {
        for (const auto& entry : logs) {
            std::cout << entry << "\n";
        }
    }

    // Main instruction execution method
    void Process::executeInstruction() {
        if (remainingInstructions <= 0 || currentInstructionIndex >= instructions.size()) {
            return;
        }

        // Apply delay-per-exec as busy-waiting (process stays on CPU)
        int delay = Config::getDelayPerExec();
        if (delay > 0) {
            // Busy-wait for specified number of CPU cycles
            auto start = std::chrono::steady_clock::now();
            auto target = start + std::chrono::milliseconds(delay * 1); // delay * tick_duration
            while (std::chrono::steady_clock::now() < target) {
                // Busy wait - process remains on CPU
            }
        }

        // Handle SLEEP cycles - this should only be called when process is Running
        // If process is Sleeping, it shouldn't be scheduled until sleep ends
        if (sleepCyclesRemaining > 0) {
            sleepCyclesRemaining--;
            if (sleepCyclesRemaining == 0) {
                // Sleep finished, return to Waiting status
                status = ProcessStatus::Waiting;
            }
            return; // Don't execute any instruction during sleep
        }

        // Execute current instruction
        const Instruction& currentInstr = instructions[currentInstructionIndex];
        
        switch (currentInstr.type) {
            case InstructionType::PRINT:
                executePrintInstruction(currentInstr);
                break;
            case InstructionType::DECLARE:
                executeDeclareInstruction(currentInstr);
                break;
            case InstructionType::ADD:
                executeAddInstruction(currentInstr);
                break;
            case InstructionType::SUBTRACT:
                executeSubtractInstruction(currentInstr);
                break;
            case InstructionType::SLEEP:
                executeSleepInstruction(currentInstr);
                break;
            case InstructionType::FOR_START:
                executeForStartInstruction(currentInstr);
                break;
            case InstructionType::FOR_END:
                executeForEndInstruction(currentInstr);
                break;
        }

        // Move to next instruction and update counters
        currentInstructionIndex++;
        --remainingInstructions;
        
        // Auto-update status when finished
        if (remainingInstructions == 0 || currentInstructionIndex >= instructions.size()) {
            status = ProcessStatus::Finished;
        }
    }

    void Process::generateRandomInstructions(int numInstructions) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> instrTypeDist(0, 6); // 7 instruction types
        std::uniform_int_distribution<> valueDist(1, 100);
        std::uniform_int_distribution<> forRepeatsDist(2, 5);
        std::uniform_int_distribution<> sleepDist(1, 10);
        
        instructions.clear();
        instructions.reserve(totalInstructions); // Reserve full space including potential FOR_ENDs
        
        int forNestingLevel = 0;
        std::vector<int> forStartPositions;
        
        for (int i = 0; i < numInstructions && instructions.size() < totalInstructions; i++) {
            InstructionType instrType = static_cast<InstructionType>(instrTypeDist(gen));
            
            // Limit FOR loop nesting to 3 levels and prevent new FOR loops if close to instruction limit
            if (instrType == InstructionType::FOR_START && 
                (forNestingLevel >= 3 || instructions.size() >= totalInstructions - forStartPositions.size())) {
                instrType = InstructionType::PRINT; // Default to PRINT instead
            }
            
            Instruction instr(instrType);
            
            switch (instrType) {
                case InstructionType::PRINT: {
                    // Generate PRINT with variable or simple message
                    if (variables.empty() || valueDist(gen) % 2 == 0) {
                        instr.arg1 = "Hello world from " + name + "!";
                    } else {
                        // Print a variable
                        auto varIt = variables.begin();
                        std::advance(varIt, valueDist(gen) % variables.size());
                        instr.arg1 = "Value from: " + varIt->first;
                    }
                    break;
                }
                case InstructionType::DECLARE: {
                    instr.arg1 = generateRandomVariableName();
                    instr.value = valueDist(gen);
                    break;
                }
                case InstructionType::ADD:
                case InstructionType::SUBTRACT: {
                    instr.arg1 = generateRandomVariableName(); // Result variable
                    instr.arg2 = generateRandomVariableName(); // First operand
                    if (valueDist(gen) % 2 == 0) {
                        instr.arg3 = generateRandomVariableName(); // Variable operand
                    } else {
                        instr.value = valueDist(gen); // Numeric operand
                    }
                    break;
                }
                case InstructionType::SLEEP: {
                    instr.value = sleepDist(gen);
                    break;
                }
                case InstructionType::FOR_START: {
                    instr.value = forRepeatsDist(gen);
                    instr.forLevel = forNestingLevel;
                    forStartPositions.push_back(i);
                    forNestingLevel++;
                    break;
                }
                case InstructionType::FOR_END: {
                    if (!forStartPositions.empty()) {
                        forNestingLevel--;
                        instr.forLevel = forNestingLevel;
                        forStartPositions.pop_back();
                    } else {
                        // No matching FOR_START, convert to PRINT
                        instr.type = InstructionType::PRINT;
                        instr.arg1 = "Hello world from " + name + "!";
                    }
                    break;
                }
            }
            
            instructions.push_back(instr);
        }
        
        // Close any unclosed FOR loops, but only if we have space
        while (!forStartPositions.empty() && instructions.size() < totalInstructions) {
            Instruction endInstr(InstructionType::FOR_END);
            endInstr.forLevel = --forNestingLevel;
            instructions.push_back(endInstr);
            forStartPositions.pop_back();
            // Don't increment totalInstructions or remainingInstructions anymore
        }
        
        // Fill any remaining slots with PRINT instructions
        while (instructions.size() < totalInstructions) {
            Instruction instr(InstructionType::PRINT);
            instr.arg1 = "Hello world from " + name + "!";
            instructions.push_back(instr);
        }
    }

    void Process::executePrintInstruction(const Instruction& instr) {
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore << " ";
        
        if (instr.arg1.find("Value from:") == 0) {
            // Extract variable name and print its value
            std::string varName = instr.arg1.substr(12); // Remove "Value from: "
            uint16_t value = getVariableValue(varName);
            entry << "\"" << instr.arg1 << " " << value << "\"";
        } else {
            entry << "\"" << instr.arg1 << "\"";
        }
        
        logs.push_back(entry.str());
    }

    void Process::executeDeclareInstruction(const Instruction& instr) {
        setVariableValue(instr.arg1, instr.value);
        
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore 
              << " DECLARE " << instr.arg1 << " = " << instr.value;
        logs.push_back(entry.str());
    }

    void Process::executeAddInstruction(const Instruction& instr) {
        uint16_t operand1 = getVariableValue(instr.arg2);
        uint16_t operand2 = instr.arg3.empty() ? instr.value : getVariableValue(instr.arg3);
        
        uint16_t result = operand1 + operand2;
        setVariableValue(instr.arg1, result);
        
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore 
              << " " << instr.arg1 << " = " << operand1 << " + " << operand2 << " = " << result;
        logs.push_back(entry.str());
    }

    void Process::executeSubtractInstruction(const Instruction& instr) {
        uint16_t operand1 = getVariableValue(instr.arg2);
        uint16_t operand2 = instr.arg3.empty() ? instr.value : getVariableValue(instr.arg3);
        
        // Clamp to prevent underflow (uint16 range)
        uint16_t result = (operand1 >= operand2) ? (operand1 - operand2) : 0;
        setVariableValue(instr.arg1, result);
        
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore 
              << " " << instr.arg1 << " = " << operand1 << " - " << operand2 << " = " << result;
        logs.push_back(entry.str());
    }

    void Process::executeSleepInstruction(const Instruction& instr) {
        sleepCyclesRemaining = instr.value;
        status = ProcessStatus::Sleeping;  // Process will relinquish CPU
        
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore 
              << " SLEEP " << instr.value << " cycles";
        logs.push_back(entry.str());
    }

    void Process::executeForStartInstruction(const Instruction& instr) {
        forLoopStack.push_back(currentInstructionIndex);
        forCounterStack.push_back(instr.value);
        
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore 
              << " FOR loop start (" << instr.value << " iterations)";
        logs.push_back(entry.str());
    }

    void Process::executeForEndInstruction(const Instruction& instr) {
        if (!forLoopStack.empty() && !forCounterStack.empty()) {
            forCounterStack.back()--;
            
            if (forCounterStack.back() > 0) {
                // Jump back to FOR_START
                currentInstructionIndex = forLoopStack.back();
            } else {
                // Loop finished, clean up
                forLoopStack.pop_back();
                forCounterStack.pop_back();
                
                std::ostringstream entry;
                entry << getTimestamp() << " Core:" << assignedCore 
                      << " FOR loop end";
                logs.push_back(entry.str());
            }
        }
    }

    uint16_t Process::getVariableValue(const std::string& varName) {
        auto it = variables.find(varName);
        if (it != variables.end()) {
            return it->second;
        }
        // Auto-declare with value 0 if not found
        variables[varName] = 0;
        return 0;
    }

    void Process::setVariableValue(const std::string& varName, uint16_t value) {
        // Clamp to uint16 range (0 to 65535)
        variables[varName] = std::min(value, static_cast<uint16_t>(65535));
    }

    std::string Process::generateRandomVariableName() {
        static std::vector<std::string> varNames = {"x", "y", "z", "a", "b", "c", "counter", "temp", "result", "sum"};
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, static_cast<int>(varNames.size()) - 1);
        
        return varNames[dist(gen)];
    }

    // How many instructions are left
    int Process::getRemainingInstructions() const {
        return remainingInstructions;
    }

    // Has the process completed all its work?
    bool Process::hasFinished() const {
        return remainingInstructions == 0 || currentInstructionIndex >= instructions.size();
    }

    // Status and core management
    ProcessStatus Process::getStatus() const {
        return status;
    }

    void Process::setStatus(ProcessStatus newStatus) {
        status = newStatus;
        // Auto-update to Finished status when no instructions remain
        if (remainingInstructions == 0 || currentInstructionIndex >= instructions.size()) {
            status = ProcessStatus::Finished;
        }
    }

    int Process::getAssignedCore() const {
        return assignedCore;
    }

    void Process::setAssignedCore(int coreId) {
        assignedCore = coreId;
    }

    const std::string& Process::getName() const {
        return name;
    }

    int Process::getId() const {
        return id;
    }

    int Process::getTotalInstructions() const {
        return totalInstructions;
    }

    const std::string& Process::getCreationTime() const {
        return creationTime;
    }
