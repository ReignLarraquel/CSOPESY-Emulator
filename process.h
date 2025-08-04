#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include "Config.h"

class MemoryManager; // Forward declaration for MemoryManager
struct PageTableEntry {
    int frameNumber;   // -1 if not in memory
    bool valid;        // true if page is in physical memory
    bool dirty;        // optional, for write tracking
};


// Process status enumeration
enum class ProcessStatus {
    Waiting,    
    Running,    
    Sleeping,   // Process is sleeping and should relinquish CPU
    Finished    
};

// Instruction types for the process programming language
enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR_START,
    FOR_END,
    READ,
    WRITE
};


// Individual instruction structure
struct Instruction {
    InstructionType type;
    std::string arg1;       // Variable name, message, or operand
    std::string arg2;       // Second operand (for ADD/SUBTRACT)
    std::string arg3;       // Third operand (for ADD/SUBTRACT)
    uint16_t value;         // Numeric value (for DECLARE, SLEEP, FOR repeats)
    int forLevel;           // Nesting level for FOR loops (0-2)
    
    Instruction(InstructionType t) : type(t), value(0), forLevel(0) {}
};

// A simple Process class that tracks execution instructions, logs, and progress.
class Process {
private:
    std::string name;             // Human-readable process name
    int id;                       // Numeric process identifier
    int totalInstructions;        // How many instructions it started with
    int remainingInstructions;    // How many are left to execute
    std::vector<std::string> logs;      // Stored log entries
    ProcessStatus status;         // Current execution status
    int assignedCore;             // Which CPU core is running this (-1 if none)
    std::string creationTime;     // Timestamp when process was created
    int memoryRequirement;        // Memory requirement in pages


    std::unordered_map<int, PageTableEntry> pageTable;

    // Process instruction system
    std::vector<Instruction> instructions;              // Generated instruction sequence
    int currentInstructionIndex;                        // Current instruction being executed
    std::map<std::string, uint16_t> variables;          // Variable storage (uint16)
    std::vector<int> forLoopStack;                      // Stack for nested FOR loops
    std::vector<int> forCounterStack;                   // Current iteration counters
    int sleepCyclesRemaining;                           // For SLEEP instruction


private:
    bool terminatedDueToMemoryViolation = false;  // Fixed variable name
    std::string memoryViolationTimestamp;
    uint32_t memoryViolationAddress = 0;

public:
    // Constructor: initializes all fields and generates random instructions
    Process(const std::string& name, int id, int numInstructions, int memorySize,
        const std::string& customInstructions = "");


    void printProcess() const;
    std::string getTimestamp() const;
    void displayLogs() const;
    void executeInstruction();
    int getRemainingInstructions() const;
    bool hasFinished() const;
    ProcessStatus getStatus() const;
    void setStatus(ProcessStatus newStatus);
    int getAssignedCore() const;
    int getMemoryRequirement() const;
    void setAssignedCore(int coreId);
    const std::string& getName() const;
    int getId() const;
    int getTotalInstructions() const;
    const std::string& getCreationTime() const;
    const std::unordered_map<int, PageTableEntry>& getPageTable() const;
    std::unordered_map<int, PageTableEntry>& getPageTableRef() {
        return pageTable;
    }
    void setMemoryManager(MemoryManager* mgr) {
        memoryManager = mgr;
    }
    bool setVariable(const std::string& varName, uint16_t value) {
        setVariableValue(varName, value);
        return true;
    }

    uint16_t getMemoryValueAt(uint32_t address) const;
    bool setMemoryValueAt(uint32_t address, uint16_t value);
    std::unordered_map<uint32_t, uint16_t> getMemoryDump() const;
    void setCustomInstructions(const std::string& instructionsStr);

    // Add these accessor methods
    bool wasTerminatedDueToMemoryViolation() const { return terminatedDueToMemoryViolation; }
    const std::string& getMemoryViolationTimestamp() const { return memoryViolationTimestamp; }
    uint32_t getMemoryViolationAddress() const { return memoryViolationAddress; }

    // Add this section
    void markAsMemoryViolation(uint32_t address) {
        terminatedDueToMemoryViolation = true;
        memoryViolationTimestamp = getTimestamp();
        memoryViolationAddress = address;
        status = ProcessStatus::Finished; // Mark process as finished
    }

    // Move this from private to public
    bool isValidMemoryAccess(uint32_t address) const;

private:
    // Instruction system helper methods
    void generateRandomInstructions(int numInstructions);
    void executePrintInstruction(const Instruction& instr);
    void executeDeclareInstruction(const Instruction& instr);
    void executeAddInstruction(const Instruction& instr);
    void executeSubtractInstruction(const Instruction& instr);
    void executeSleepInstruction(const Instruction& instr);
    void executeForStartInstruction(const Instruction& instr);
    void executeForEndInstruction(const Instruction& instr);
    MemoryManager* memoryManager = nullptr;

    uint16_t getVariableValue(const std::string& varName);
    void setVariableValue(const std::string& varName, uint16_t value);
    std::string generateRandomVariableName();
    std::string formatInstructionForLog(const Instruction& instr);

    std::unordered_map<uint32_t, uint16_t> memoryValues; // Address -> Value
    uint16_t readMemoryValue(uint32_t address);
    void writeMemoryValue(uint32_t address, uint16_t value);
    void executeReadInstruction(const Instruction& instr);
    void executeWriteInstruction(const Instruction& instr);
};