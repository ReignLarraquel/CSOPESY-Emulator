#pragma once

#include <iostream>
#include <string>
#include <vector>

// Process status enumeration
enum class ProcessStatus {
    Waiting,    
    Running,    
    Finished    
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


public:
    // Constructor: initializes all fields
    Process(const std::string& processName, int processId, int numInstructions);


    void printProcess() const;
    std::string getTimestamp() const;
    void displayLogs() const;
    void executeInstruction();
    int getRemainingInstructions() const;
    bool hasFinished() const;
    ProcessStatus getStatus() const;
    void setStatus(ProcessStatus newStatus);
    int getAssignedCore() const;
    void setAssignedCore(int coreId);
    const std::string& getName() const;
    int getId() const;
    int getTotalInstructions() const;
    const std::string& getCreationTime() const;
};
