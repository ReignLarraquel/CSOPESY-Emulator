#include "Process.h"

#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>


    // Constructor: initializes all four fields
    Process::Process(const std::string& processName, int processId, int numInstructions)
        : name(processName), id(processId), totalInstructions(numInstructions), remainingInstructions(numInstructions) {}

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

    // Executes one instruction, logs it with timestamp
    void Process::executeInstruction() {
        if (remainingInstructions <= 0) return;

        --remainingInstructions;
        std::ostringstream entry;
        entry << getTimestamp()
            << " \"Hello world from " << name << "!\"";
        logs.push_back(entry.str());
    }


    // How many instructions are left
    int Process::getRemainingInstructions() const {
        return remainingInstructions;
    }

    // Has the process completed all its work?
    bool Process::hasFinished() const {
        return remainingInstructions == 0;
    }
