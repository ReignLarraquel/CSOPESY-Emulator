#include "process.h"

#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>


    // Constructor: initializes all four fields
    Process::Process(const std::string& processName, int processId, int numInstructions)
        : name(processName), id(processId), totalInstructions(numInstructions), remainingInstructions(numInstructions) {}

    void Process::printProcess() const {
        std::cout << "Process name: " << name << "\n";
        std::cout << "ID: " << id << "\n";

        std::cout << "Logs:\n";
        //call logs here

        std::cout << "Current instruction line: " << /*num here <<*/ "\n";
        std::cout << "Lines of code: " << totalInstructions << "\n\n";
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

    void Process::displayLogs() const {
        std::cout << getTimestamp() << "Core: " << /*core <<*/ "0* " << "Hello world from " << name << "!\n";
    }

    // Execute one instruction (if any remain)
    void Process::executeInstruction() {
        if (remainingInstructions > 0) {
            std::cout << "Executing instruction for Process " << id << ": " << name << "\n";
            displayLogs();
            --remainingInstructions;   // consume one instruction
        }
        else {
            std::cout << "Process " << id << ": " << name << " has already finished.\n";
        }
    }

    // How many instructions are left
    int Process::getRemainingInstructions() const {
        return remainingInstructions;
    }

    // Has the process completed all its work?
    bool Process::hasFinished() const {
        return remainingInstructions == 0;
    }
