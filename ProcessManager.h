#pragma once
#include "TypedefRepo.h"
#include "process.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();
    
    // Process lifecycle
    void addProcess(std::shared_ptr<Process> process);
    void removeProcess(const String& processName);
    bool hasProcess(const String& processName) const;
    
    // Process state management
    void updateProcessStatus(const String& processName, ProcessStatus status);
    void setProcessCore(const String& processName, int coreId);
    void executeProcessInstruction(const String& processName);
    
    // Process queries
    std::shared_ptr<Process> getProcess(const String& processName) const;
    std::vector<String> getProcessesByStatus(ProcessStatus status) const;
    std::vector<String> getAllProcessNames() const;
    std::map<String, std::shared_ptr<Process>> getAllProcesses() const;

    
    // Batch operations (for tick processing)
    struct ProcessInfo {
        String name;
        std::shared_ptr<Process> process;
        bool isFinished;
    };
    
    std::vector<ProcessInfo> executeInstructionsForProcesses(const std::vector<String>& processNames);
    
private:
    std::map<String, std::shared_ptr<Process>> processMap;
    mutable std::mutex processMutex;
    
    // Private helper (assumes lock held)
    std::shared_ptr<Process> getProcessUnsafe(const String& processName) const;
}; 