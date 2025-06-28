#include "process.h"
#include "Config.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>


    // Constructor: initializes all fields
    Process::Process(const std::string& processName, int processId, int numInstructions)
        : name(processName), id(processId), totalInstructions(numInstructions), 
          remainingInstructions(numInstructions), status(ProcessStatus::Waiting), assignedCore(-1) {
        creationTime = getTimestamp();
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

    // Executes one instruction, logs it with timestamp
    void Process::executeInstruction() {
        if (remainingInstructions <= 0) return;

        // Apply delay-per-exec configuration
        int delay = Config::getDelayPerExec();
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }

        --remainingInstructions;
        std::ostringstream entry;
        entry << getTimestamp()  // Current execution time, not creation time
            << " Core:" << assignedCore << " \"Hello world from " << name << "!\"";
        logs.push_back(entry.str());
        
        // Auto-update status when finished
        if (remainingInstructions == 0) {
            status = ProcessStatus::Finished;
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

    // Status and core management
    ProcessStatus Process::getStatus() const {
        return status;
    }

    void Process::setStatus(ProcessStatus newStatus) {
        status = newStatus;
        // Auto-update to Finished status when no instructions remain
        if (remainingInstructions == 0) {
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
