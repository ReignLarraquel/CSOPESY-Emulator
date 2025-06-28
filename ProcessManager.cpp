#include "ProcessManager.h"
#include <algorithm>

ProcessManager::ProcessManager() {
}

ProcessManager::~ProcessManager() {
}

void ProcessManager::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(processMutex);
    if (process) {
        processMap[process->getName()] = process;
    }
}

void ProcessManager::removeProcess(const String& processName) {
    std::lock_guard<std::mutex> lock(processMutex);
    processMap.erase(processName);
}

bool ProcessManager::hasProcess(const String& processName) const {
    std::lock_guard<std::mutex> lock(processMutex);
    return processMap.find(processName) != processMap.end();
}

void ProcessManager::updateProcessStatus(const String& processName, ProcessStatus status) {
    std::lock_guard<std::mutex> lock(processMutex);
    auto process = getProcessUnsafe(processName);
    if (process) {
        process->setStatus(status);
        
        // Ensure consistency: if setting to Waiting or Sleeping, core should be -1
        if (status == ProcessStatus::Waiting || status == ProcessStatus::Sleeping) {
            process->setAssignedCore(-1);
        }
    }
}

void ProcessManager::setProcessCore(const String& processName, int coreId) {
    std::lock_guard<std::mutex> lock(processMutex);
    auto process = getProcessUnsafe(processName);
    if (process) {
        process->setAssignedCore(coreId);
    }
}

void ProcessManager::executeProcessInstruction(const String& processName) {
    std::lock_guard<std::mutex> lock(processMutex);
    auto process = getProcessUnsafe(processName);
    if (process && process->getRemainingInstructions() > 0) {
        process->executeInstruction();
    }
}

std::shared_ptr<Process> ProcessManager::getProcess(const String& processName) const {
    std::lock_guard<std::mutex> lock(processMutex);
    return getProcessUnsafe(processName);
}

std::vector<String> ProcessManager::getProcessesByStatus(ProcessStatus status) const {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<std::pair<String, std::shared_ptr<Process>>> filteredProcesses;
    
    // First, collect processes with matching status
    for (const auto& [name, process] : processMap) {
        if (process && process->getStatus() == status) {
            filteredProcesses.push_back({name, process});
        }
    }
    
    // Sort by creation time (chronological order)
    std::sort(filteredProcesses.begin(), filteredProcesses.end(),
        [](const auto& a, const auto& b) {
            return a.second->getCreationTime() < b.second->getCreationTime();
        });
    
    // Extract just the names
    std::vector<String> result;
    for (const auto& [name, process] : filteredProcesses) {
        result.push_back(name);
    }
    
    return result;
}

std::vector<String> ProcessManager::getAllProcessNames() const {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<String> result;
    
    for (const auto& [name, process] : processMap) {
        result.push_back(name);
    }
    
    return result;
}

std::vector<ProcessManager::ProcessInfo> ProcessManager::executeInstructionsForProcesses(const std::vector<String>& processNames) {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<ProcessInfo> results;
    
    for (const String& processName : processNames) {
        auto process = getProcessUnsafe(processName);
        if (process) {
            bool wasFinished = (process->getRemainingInstructions() == 0);
            
            if (!wasFinished && process->getRemainingInstructions() > 0) {
                process->executeInstruction();
            }
            
            bool isNowFinished = (process->getRemainingInstructions() == 0);
            
            results.push_back({
                processName,
                process,
                !wasFinished && isNowFinished  // Just finished this tick
            });
        }
    }
    
    return results;
}

std::shared_ptr<Process> ProcessManager::getProcessUnsafe(const String& processName) const {
    auto it = processMap.find(processName);
    return (it != processMap.end()) ? it->second : nullptr;
} 