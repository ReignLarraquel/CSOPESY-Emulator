#include "process.h"
#include "Config.h"
#include "MemoryManager.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <random>
#include <algorithm>


    // Constructor: initializes all fields and generates random instructions
Process::Process(const std::string& name, int id, int numInstructions, int memorySize,
    const std::string& customInstructions)
    : name(name), id(id), totalInstructions(numInstructions),
    remainingInstructions(numInstructions), status(ProcessStatus::Waiting),
    assignedCore(-1), currentInstructionIndex(0), sleepCyclesRemaining(0),
    memoryRequirement(memorySize), nextVariableAddress(SYMBOL_TABLE_START) { // TOBEDELETED: Initialize symbol table address counter

    creationTime = getTimestamp();

    // Initialize page table
    int numPages = memoryRequirement / Config::getMemPerFrame();
    for (int i = 0; i < numPages; ++i) {
        pageTable[i] = { -1, false, false };
    }

    if (!customInstructions.empty()) {
        try {
            // Parse and set custom instructions
            setCustomInstructions(customInstructions);
        }
        catch (const std::exception& e) {
            std::ostringstream entry;
            entry << getTimestamp() << " Core:" << assignedCore
                << " ERROR: Invalid instructions: " << e.what();
            logs.push_back(entry.str());

            // Fall back to random instructions if custom instructions failed
            generateRandomInstructions(numInstructions);
        }
    }
    else {
        // Generate random instructions as usual
        int maxForLoops = 3;
        int reservedForEndInstructions = maxForLoops;
        int actualInstructionsToGenerate = numInstructions - reservedForEndInstructions;
        generateRandomInstructions(actualInstructionsToGenerate);
    }

    // Fill any remaining slots with PRINT instructions if needed
    int unusedSlots = numInstructions - static_cast<int>(instructions.size());
    for (int i = 0; i < unusedSlots; i++) {
        Instruction instr(InstructionType::PRINT);
        instr.arg1 = "Hello world from " + name + "!";
        instructions.push_back(instr);
    }
}

    int Process::getMemoryRequirement() const {
        return memoryRequirement;
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
        if (remainingInstructions <= 0 || currentInstructionIndex >= static_cast<int>(instructions.size())) { // TOBEDELETED: Fix C4018 warning
            return;
        }

        // Apply CPU delay if configured
        int delay = Config::getDelayPerExec();
        if (delay > 0) {
            auto start = std::chrono::steady_clock::now();
            auto target = start + std::chrono::milliseconds(delay);
            while (std::chrono::steady_clock::now() < target) {
                // Busy wait
            }
        }

        // Handle sleep logic
        if (sleepCyclesRemaining > 0) {
            sleepCyclesRemaining--;
            if (sleepCyclesRemaining == 0) {
                status = ProcessStatus::Waiting;
            }
            return;
        }

        // === DEMAND PAGING LOGIC ===
        int frameSize = Config::getMemPerFrame();
        int instructionSize = sizeof(Instruction);  // Typically 16-32 bytes depending on struct
        int virtualPage = (currentInstructionIndex * instructionSize) / frameSize;

        // TOBEDELETED: Check if page is present; if not, allocate it (MO2 continuous page fault handling)
        auto& pt = this->getPageTableRef();
        if (pt.find(virtualPage) == pt.end() || !pt[virtualPage].valid) {
            if (memoryManager) {
                // TOBEDELETED: Continuously retry until page is allocated (MO2 requirement)
                while (pt.find(virtualPage) == pt.end() || !pt[virtualPage].valid) {
                    int frameNumber = memoryManager->allocatePage(this, virtualPage);
                    if (frameNumber >= 0) {
                        // TOBEDELETED: Page successfully allocated
                        break;
                    }
                    // TOBEDELETED: Page allocation failed, continue retrying (clock algorithm will find victim)
                }
            }
        }

        // Mark this page as recently accessed (for clock replacement)
        if (memoryManager && pt.find(virtualPage) != pt.end() && pt[virtualPage].valid) {
            int frameNumber = pt[virtualPage].frameNumber;
            memoryManager->markPageAccessed(frameNumber);  // Implement this in MemoryManager
        }

        // === EXECUTE INSTRUCTION ===
        const Instruction& currentInstr = instructions[currentInstructionIndex];
        switch (currentInstr.type) {
        case InstructionType::PRINT:     executePrintInstruction(currentInstr); break;
        case InstructionType::DECLARE:   executeDeclareInstruction(currentInstr); break;
        case InstructionType::ADD:       executeAddInstruction(currentInstr); break;
        case InstructionType::SUBTRACT:  executeSubtractInstruction(currentInstr); break;
        case InstructionType::SLEEP:     executeSleepInstruction(currentInstr); break;
        case InstructionType::FOR_START: executeForStartInstruction(currentInstr); break;
        case InstructionType::FOR_END:   executeForEndInstruction(currentInstr); break;
        case InstructionType::READ:      executeReadInstruction(currentInstr); break;
        case InstructionType::WRITE:     executeWriteInstruction(currentInstr); break;
        }

        currentInstructionIndex++;
        remainingInstructions--;

        if (remainingInstructions == 0 || currentInstructionIndex >= static_cast<int>(instructions.size())) { // TOBEDELETED: Fix C4018 warning
            status = ProcessStatus::Finished;
        }
    }


    void Process::generateRandomInstructions(int numInstructions) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> instrTypeDist(0, 8); // Now 9 instruction types
        std::uniform_int_distribution<> valueDist(1, 100);
        std::uniform_int_distribution<> forRepeatsDist(2, 5);
        std::uniform_int_distribution<> sleepDist(1, 10);
        
        instructions.clear();
        instructions.reserve(totalInstructions); // Reserve full space including potential FOR_ENDs
        
        int forNestingLevel = 0;
        std::vector<int> forStartPositions;
        
        for (int i = 0; i < numInstructions && static_cast<int>(instructions.size()) < totalInstructions; i++) { // TOBEDELETED: Fix C4018 warning
            InstructionType instrType = static_cast<InstructionType>(instrTypeDist(gen));
            
            // Limit FOR loop nesting to 3 levels and prevent new FOR loops if close to instruction limit
            if (instrType == InstructionType::FOR_START && 
                (forNestingLevel >= 3 || static_cast<int>(instructions.size()) >= totalInstructions - static_cast<int>(forStartPositions.size()))) { // TOBEDELETED: Fix C4018 warning
                instrType = InstructionType::PRINT; // Default to PRINT instead
            }
            
            Instruction instr(instrType);
            
            switch (instrType) {
                case InstructionType::PRINT: {
                    // Generate PRINT with variable or simple message
                    // TOBEDELETED: Fix undeclared 'variables' - use variableAddresses instead
                    if (variableAddresses.empty() || valueDist(gen) % 2 == 0) {
                        instr.arg1 = "Hello world from " + name + "!";
                    } else {
                        // Print a variable
                        auto varIt = variableAddresses.begin();
                        std::advance(varIt, valueDist(gen) % variableAddresses.size());
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
                case InstructionType::READ: {
                    instr.arg1 = generateRandomVariableName(); // Variable to store result
                    std::ostringstream addressStream;
                    addressStream << "0x" << std::hex << (gen() % (static_cast<uint32_t>(memoryRequirement) * static_cast<uint32_t>(Config::getMemPerFrame()))); // TOBEDELETED: Fix signed/unsigned 
                    instr.arg2 = addressStream.str();
                    break;
                }
                case InstructionType::WRITE: {
                    std::ostringstream addressStream;
                    addressStream << "0x" << std::hex << (gen() % (static_cast<uint32_t>(memoryRequirement) * static_cast<uint32_t>(Config::getMemPerFrame()))); // TOBEDELETED: Fix signed/unsigned 
                    instr.arg1 = addressStream.str();
                    instr.value = valueDist(gen); // Random value to write
                    break;
                }
            }
            
            instructions.push_back(instr);
        }
        
        // Close any unclosed FOR loops, but only if we have space
        while (!forStartPositions.empty() && static_cast<int>(instructions.size()) < totalInstructions) { // TOBEDELETED: Fix C4018 warning
            Instruction endInstr(InstructionType::FOR_END);
            endInstr.forLevel = --forNestingLevel;
            instructions.push_back(endInstr);
            forStartPositions.pop_back();
            // Don't increment totalInstructions or remainingInstructions anymore
        }
        
        // Fill any remaining slots with PRINT instructions
        while (static_cast<int>(instructions.size()) < totalInstructions) { // TOBEDELETED: Fix C4018 warning
            Instruction instr(InstructionType::PRINT);
            instr.arg1 = "Hello world from " + name + "!";
            instructions.push_back(instr);
        }
    }

    void Process::executePrintInstruction(const Instruction& instr) {
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore << " ";
        
        std::string printOutput; // TOBEDELETED: Store actual PRINT output separately
        
        if (instr.arg2 == "EXPRESSION") {
            // TOBEDELETED: Evaluate string concatenation expression like "Result: " + varC
            std::string result = evaluateStringExpression(instr.arg1);
            entry << "\"" << result << "\"";
            printOutput = result; // TOBEDELETED: Store the actual output
        } else if (instr.arg1.find("Value from:") == 0) {
            // Extract variable name and print its value
            std::string varName = instr.arg1.substr(12); // Remove "Value from: "
            uint16_t value = getVariableValue(varName);
            std::string fullOutput = instr.arg1 + " " + std::to_string(value);
            entry << "\"" << fullOutput << "\"";
            printOutput = fullOutput; // TOBEDELETED: Store the actual output
        } else {
            entry << "\"" << instr.arg1 << "\"";
            printOutput = instr.arg1; // TOBEDELETED: Store the actual output
        }
        
        logs.push_back(entry.str());
        printOutputs.push_back(printOutput); // TOBEDELETED: Track PRINT outputs separately
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
        // TOBEDELETED: MO2 specification - Variables stored in 64-byte symbol table segment, not std::map
        auto it = variableAddresses.find(varName);
        if (it != variableAddresses.end()) {
            // TOBEDELETED: Variable exists - read from memory at stored address
            uint32_t address = it->second;
            return readMemoryValue(address);
        }
        
        // TOBEDELETED: Auto-declare with value 0 if not found (MO1 requirement)
        // TOBEDELETED: Check if we have space in 64-byte symbol table (32 variables max)
        if (static_cast<int>(variableAddresses.size()) >= 32) { // TOBEDELETED: Fix size_t comparison
            // TOBEDELETED: Symbol table full - return 0 and log warning
            std::ostringstream entry;
            entry << getTimestamp() << " Core:" << assignedCore
                << " WARNING: Cannot auto-declare variable '" << varName << "' - symbol table full (32 variables)";
            logs.push_back(entry.str());
            return 0;
        }
        
        // TOBEDELETED: Allocate new variable in symbol table
        uint32_t address = nextVariableAddress;
        variableAddresses[varName] = address;
        nextVariableAddress += 2; // TOBEDELETED: uint16 takes 2 bytes
        
        // TOBEDELETED: Initialize to 0 in memory
        writeMemoryValue(address, 0);
        return 0;
    }

    void Process::setVariableValue(const std::string& varName, uint16_t value) {
        // TOBEDELETED: MO2 specification - Variables stored in 64-byte symbol table segment, not std::map
        auto it = variableAddresses.find(varName);
        if (it != variableAddresses.end()) {
            // TOBEDELETED: Variable exists - write to memory at stored address
            uint32_t address = it->second;
            uint16_t clampedValue = std::min(value, static_cast<uint16_t>(65535)); // TOBEDELETED: MO1 uint16 clamping
            writeMemoryValue(address, clampedValue);
            return;
        }
        
        // TOBEDELETED: Variable doesn't exist - check if we can create it
        if (static_cast<int>(variableAddresses.size()) >= 32) { // TOBEDELETED: Fix size_t comparison
            // TOBEDELETED: Symbol table full - cannot create new variable
            std::ostringstream entry;
            entry << getTimestamp() << " Core:" << assignedCore
                << " WARNING: Cannot create variable '" << varName << "' - symbol table limit reached (32 variables)";
            logs.push_back(entry.str());
            return;
        }
        
        // TOBEDELETED: Create new variable in symbol table
        uint32_t address = nextVariableAddress;
        variableAddresses[varName] = address;
        nextVariableAddress += 2; // TOBEDELETED: uint16 takes 2 bytes
        
        // TOBEDELETED: Store value in memory
        uint16_t clampedValue = std::min(value, static_cast<uint16_t>(65535)); // TOBEDELETED: MO1 uint16 clamping
        writeMemoryValue(address, clampedValue);
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
        return remainingInstructions == 0 || currentInstructionIndex >= static_cast<int>(instructions.size()); // TOBEDELETED: Fix C4018 warning
    }

    // Status and core management
    ProcessStatus Process::getStatus() const {
        return status;
    }

    void Process::setStatus(ProcessStatus newStatus) {
        status = newStatus;
        // Auto-update to Finished status when no instructions remain
        if (remainingInstructions == 0 || currentInstructionIndex >= static_cast<int>(instructions.size())) { // TOBEDELETED: Fix C4018 warning
            status = ProcessStatus::Finished;
        }
    }

    const std::unordered_map<int, PageTableEntry>& Process::getPageTable() const {
        return pageTable;
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

    bool Process::isValidMemoryAccess(uint32_t address) const {
        // Validate memory address is within process's allocated memory range
        // TOBEDELETED: Fix signed/unsigned mismatch
        return address < (static_cast<uint32_t>(memoryRequirement) * static_cast<uint32_t>(Config::getMemPerFrame()));
    }

    uint16_t Process::readMemoryValue(uint32_t address) {
        // Calculate which page this address falls in
        int pageSize = Config::getMemPerFrame();
        int pageNumber = address / pageSize;

        // TOBEDELETED: MO2 specification - "Page fault handling continuously occurs until a valid page has been returned"
        auto& pt = this->getPageTableRef();
        if (pt.find(pageNumber) == pt.end() || !pt[pageNumber].valid) {
            if (memoryManager) {
                // TOBEDELETED: Continuously retry until page is allocated (MO2 requirement)
                while (pt.find(pageNumber) == pt.end() || !pt[pageNumber].valid) {
                    int frameNumber = memoryManager->allocatePage(this, pageNumber);
                    if (frameNumber >= 0) {
                        // TOBEDELETED: Page successfully allocated
                        break;
                    }
                    // TOBEDELETED: Page allocation failed, continue retrying (clock algorithm will find victim)
                    // TOBEDELETED: In real OS this would be a blocking operation, here we busy-wait
                }
            }
        }

        // Return the value (0 if not initialized)
        auto it = memoryValues.find(address);
        if (it != memoryValues.end()) {
            return it->second;
        }
        return 0; // Return 0 for uninitialized memory
    }

    void Process::writeMemoryValue(uint32_t address, uint16_t value) {
        // Calculate which page this address falls in
        int pageSize = Config::getMemPerFrame();
        int pageNumber = address / pageSize;

        // TOBEDELETED: MO2 specification - "Page fault handling continuously occurs until a valid page has been returned"
        auto& pt = this->getPageTableRef();
        if (pt.find(pageNumber) == pt.end() || !pt[pageNumber].valid) {
            if (memoryManager) {
                // TOBEDELETED: Continuously retry until page is allocated (MO2 requirement)
                while (pt.find(pageNumber) == pt.end() || !pt[pageNumber].valid) {
                    int frameNumber = memoryManager->allocatePage(this, pageNumber);
                    if (frameNumber >= 0) {
                        // TOBEDELETED: Page successfully allocated
                        break;
                    }
                    // TOBEDELETED: Page allocation failed, continue retrying (clock algorithm will find victim)
                    // TOBEDELETED: In real OS this would be a blocking operation, here we busy-wait
                }
            }
        }

        // Write to memory map and mark page as dirty
        memoryValues[address] = value;
        if (pt.find(pageNumber) != pt.end() && pt[pageNumber].valid) {
            pt[pageNumber].dirty = true;
        }
    }

    void Process::executeReadInstruction(const Instruction& instr) {
        // Convert hex address to integer
        uint32_t address = 0;
        try {
            address = std::stoul(instr.arg2, nullptr, 16); // Parse as hex
        }
        catch (...) {
            // Invalid address format
            std::ostringstream entry;
            entry << getTimestamp() << " Core:" << assignedCore
                << " ERROR: Invalid memory address format: " << instr.arg2;
            logs.push_back(entry.str());
            status = ProcessStatus::Finished; // Terminate process on error
            
            // Set violation flags
            terminatedDueToMemoryViolation = true;
            memoryViolationTimestamp = getTimestamp();
            memoryViolationAddress = 0; // Use 0 for invalid format
            return;
        }

        // Check if address is within valid range
        if (!isValidMemoryAccess(address)) {
            std::ostringstream entry;
            entry << getTimestamp() << " Core:" << assignedCore
                << " ERROR: Memory access violation at address " << instr.arg2;
            logs.push_back(entry.str());
            status = ProcessStatus::Finished; // Terminate on access violation
            
            // Set violation flags
            terminatedDueToMemoryViolation = true;
            memoryViolationTimestamp = getTimestamp();
            memoryViolationAddress = address;
            return;
        }

        // Read value from memory
        uint16_t value = readMemoryValue(address);

        // Store value in variable
        setVariableValue(instr.arg1, value);

        // Log the operation
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore
            << " READ " << instr.arg1 << " = " << value << " from " << instr.arg2;
        logs.push_back(entry.str());
    }

    void Process::executeWriteInstruction(const Instruction& instr) {
        // Convert hex address to integer
        uint32_t address = 0;
        try {
            address = std::stoul(instr.arg1, nullptr, 16); // Parse as hex
        }
        catch (...) {
            // Invalid address format
            std::ostringstream entry;
            entry << getTimestamp() << " Core:" << assignedCore
                << " ERROR: Invalid memory address format: " << instr.arg1;
            logs.push_back(entry.str());
            status = ProcessStatus::Finished; // Terminate process on error
            
            // Set violation flags
            terminatedDueToMemoryViolation = true;
            memoryViolationTimestamp = getTimestamp();
            memoryViolationAddress = 0; // Use 0 for invalid format
            return;
        }

        // Check if address is within valid range
        if (!isValidMemoryAccess(address)) {
            std::ostringstream entry;
            entry << getTimestamp() << " Core:" << assignedCore
                << " ERROR: Memory access violation at address " << instr.arg1;
            logs.push_back(entry.str());
            status = ProcessStatus::Finished; // Terminate on access violation
            
            // Set violation flags
            terminatedDueToMemoryViolation = true;
            memoryViolationTimestamp = getTimestamp();
            memoryViolationAddress = address;
            return;
        }

        // Determine the value to write - either direct value or from variable
        uint16_t valueToWrite = instr.value;
        if (!instr.arg2.empty()) {
            // It's a variable reference - get its value
            valueToWrite = getVariableValue(instr.arg2);
        }

        // Write value to memory
        writeMemoryValue(address, valueToWrite);

        // Log the operation
        std::ostringstream entry;
        entry << getTimestamp() << " Core:" << assignedCore
            << " WRITE " << valueToWrite << " to " << instr.arg1;
        logs.push_back(entry.str());
    }

    uint16_t Process::getMemoryValueAt(uint32_t address) const {
        if (!isValidMemoryAccess(address))
            return 0;
        
        // Since this is const, we can't modify the memoryValues map
        // to auto-create entries for uninitialized addresses
        auto it = memoryValues.find(address);
        if (it != memoryValues.end()) {
            return it->second;
        }
        return 0; // Return 0 for uninitialized memory
    }

    bool Process::setMemoryValueAt(uint32_t address, uint16_t value) {
        if (!isValidMemoryAccess(address))
            return false;
        
        // Calculate which page this address falls in
        int pageSize = Config::getMemPerFrame();
        int pageNumber = address / pageSize;

        // Check if we need to page in this memory
        auto& pt = this->getPageTableRef();
        if (pt.find(pageNumber) == pt.end() || !pt[pageNumber].valid) {
            if (memoryManager) {
                memoryManager->allocatePage(this, pageNumber);
            }
        }

        // Write to memory map and mark page as dirty
        memoryValues[address] = value;
        if (pt.find(pageNumber) != pt.end() && pt[pageNumber].valid) {
            pt[pageNumber].dirty = true;
        }
        return true;
    }

    std::unordered_map<uint32_t, uint16_t> Process::getMemoryDump() const {
        return memoryValues;
    }

    void Process::setCustomInstructions(const std::string& instructionsStr) {
        instructions.clear();
        std::vector<std::string> instructionList;
        
        // Split by semicolons
        size_t pos = 0;
        std::string input = instructionsStr;
        while ((pos = input.find(';')) != std::string::npos) {
            std::string instr = input.substr(0, pos);
            instructionList.push_back(instr);
            input.erase(0, pos + 1);
        }
        if (!input.empty()) {
            instructionList.push_back(input);
        }
        
        // TOBEDELETED: Validate instruction count - MO2 spec requires exact error message
        if (instructionList.empty() || instructionList.size() > 50) {
            // TOBEDELETED: MO2 specification: "Throws 'invalid command' if the instruction size is not met"
            throw std::runtime_error("invalid command");
        }
        
        // Parse each instruction
        for (const auto& instrText : instructionList) {
            std::string trimmed = instrText;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
            
            std::stringstream ss(trimmed);
            std::string cmd;
            ss >> cmd;
            
            if (cmd == "DECLARE") {
                std::string varName;
                uint16_t value;
                ss >> varName >> value;
                
                Instruction instr(InstructionType::DECLARE);
                instr.arg1 = varName;
                instr.value = value;
                instructions.push_back(instr);
            } 
            else if (cmd == "ADD") {
                std::string result, op1, op2;
                ss >> result >> op1 >> op2;
                
                Instruction instr(InstructionType::ADD);
                instr.arg1 = result;
                instr.arg2 = op1;
                instr.arg3 = op2;
                instructions.push_back(instr);
            } 
            else if (cmd == "SUBTRACT") {
                std::string result, op1, op2;
                ss >> result >> op1 >> op2;
                
                Instruction instr(InstructionType::SUBTRACT);
                instr.arg1 = result;
                instr.arg2 = op1;
                instr.arg3 = op2;
                instructions.push_back(instr);
            }
            else if (cmd == "WRITE") {
                std::string address, value;
                ss >> address >> value;
                
                Instruction instr(InstructionType::WRITE);
                instr.arg1 = address;
                
                // Check if value is a variable or literal
                if (!value.empty() && std::isdigit(value[0])) {
                    instr.value = static_cast<uint16_t>(std::stoi(value));
                } else {
                    instr.arg2 = value; // It's a variable name
                }
                instructions.push_back(instr);
            }
            else if (cmd == "READ") {
                std::string varName, address;
                ss >> varName >> address;
                
                Instruction instr(InstructionType::READ);
                instr.arg1 = varName;
                instr.arg2 = address;
                instructions.push_back(instr);
            }
            else if (trimmed.find("PRINT") == 0) {
                // TOBEDELETED: Handle PRINT with string literals and expressions
                size_t openParen = trimmed.find("(");
                size_t closeParen = trimmed.find_last_of(")");
                
                if (openParen != std::string::npos && closeParen != std::string::npos) {
                    std::string content = trimmed.substr(openParen + 1, closeParen - openParen - 1);
                    
                    Instruction instr(InstructionType::PRINT);
                    instr.arg1 = content;
                    
                    // TOBEDELETED: Check if this is a string concatenation expression (contains + operator)
                    if (content.find(" + ") != std::string::npos) {
                        instr.arg2 = "EXPRESSION"; // TOBEDELETED: Flag for expression evaluation
                    }
                    
                    instructions.push_back(instr);
                }
            }
            else if (cmd == "SLEEP") {
                uint16_t cycles;
                ss >> cycles;
                
                Instruction instr(InstructionType::SLEEP);
                instr.value = cycles;
                instructions.push_back(instr);
            }
        }
        
        // TOBEDELETED: Fix size_t to int conversion warnings
        remainingInstructions = static_cast<int>(instructions.size());
        totalInstructions = static_cast<int>(instructions.size());
    }

    std::string Process::evaluateStringExpression(const std::string& expression) {
        // TOBEDELETED: Parse and evaluate string concatenation expressions like "Result: " + varC
        std::string result;
        std::string current = expression;
        
        // TOBEDELETED: Split by " + " to get parts
        size_t pos = 0;
        while ((pos = current.find(" + ")) != std::string::npos) {
            std::string part = current.substr(0, pos);
            
            // TOBEDELETED: Check if part is a string literal (has quotes) or variable
            if (!part.empty() && part.front() == '"' && part.back() == '"') {
                // TOBEDELETED: String literal - remove quotes
                result += part.substr(1, part.length() - 2);
            } else if (!part.empty() && part.front() == '"') {
                // TOBEDELETED: String literal without closing quote - remove opening quote
                result += part.substr(1);
            } else if (!part.empty() && part.back() == '"') {
                // TOBEDELETED: String literal without opening quote - remove closing quote
                result += part.substr(0, part.length() - 1);
            } else {
                // TOBEDELETED: Variable reference - get its value
                uint16_t value = getVariableValue(part);
                result += std::to_string(value);
            }
            
            current.erase(0, pos + 3); // TOBEDELETED: Remove processed part and " + " (pos is size_t, ok to add 3)
        }
        
        // TOBEDELETED: Handle final part
        if (!current.empty()) {
            if (current.front() == '"' && current.back() == '"') {
                // TOBEDELETED: String literal - remove quotes
                result += current.substr(1, current.length() - 2);
            } else if (current.front() == '"') {
                // TOBEDELETED: String literal without closing quote - remove opening quote
                result += current.substr(1);
            } else if (current.back() == '"') {
                // TOBEDELETED: String literal without opening quote - remove closing quote
                result += current.substr(0, current.length() - 1);
            } else {
                // TOBEDELETED: Variable reference - get its value
                uint16_t value = getVariableValue(current);
                result += std::to_string(value);
            }
        }
        
        return result;
    }