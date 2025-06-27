#pragma once

#include <iostream>
#include <string>
#include <vector>

// A simple Process class that tracks execution instructions, logs, and progress.
class Process {
private:
    std::string name;             // Human-readable process name
    int id;                       // Numeric process identifier
    int totalInstructions;        // How many instructions it started with
    int remainingInstructions;    // How many are left to execute

public:
    // Constructor: initializes all fields
    Process(const std::string& processName, int processId, int numInstructions);

    // Print summary of the process (name, ID, progress)
    void printProcess() const;

    // Get a timestamp string in format (MM/DD/YYYY hh:mm:ssAM/PM)
    std::string getTimestamp() const;

    // Display all collected log entries with timestamps and core info
    void displayLogs() const;

    // Execute one instruction (if any remain) and record a log entry
    void executeInstruction();

    // How many instructions are left
    int getRemainingInstructions() const;

    // Has the process completed all its work?
    bool hasFinished() const;
};
