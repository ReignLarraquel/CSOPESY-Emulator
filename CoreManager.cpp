#include "CoreManager.h"

CoreManager::CoreManager(int numCores) : numCores(numCores) {
    coreAssignments.resize(numCores, "");  // Empty strings = available
    quantumRemaining.resize(numCores, 0);
}

CoreManager::~CoreManager() {
}

bool CoreManager::tryAssignProcess(int coreId, const String& processName) {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (!isValidCoreId(coreId) || processName.empty()) {
        return false;
    }
    
    // Only assign if core is available
    if (coreAssignments[coreId].empty()) {
        coreAssignments[coreId] = processName;
        return true;
    }
    
    return false;
}

void CoreManager::clearAssignment(int coreId) {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (isValidCoreId(coreId)) {
        coreAssignments[coreId] = "";
        quantumRemaining[coreId] = 0;
    }
}

String CoreManager::getAssignment(int coreId) const {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (isValidCoreId(coreId)) {
        return coreAssignments[coreId];
    }
    
    return "";
}

bool CoreManager::isCoreAvailable(int coreId) const {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (isValidCoreId(coreId)) {
        return coreAssignments[coreId].empty();
    }
    
    return false;
}

void CoreManager::setQuantum(int coreId, int quantum) {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (isValidCoreId(coreId)) {
        quantumRemaining[coreId] = quantum;
    }
}

int CoreManager::getQuantum(int coreId) const {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (isValidCoreId(coreId)) {
        return quantumRemaining[coreId];
    }
    
    return 0;
}

void CoreManager::decrementQuantum(int coreId) {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (isValidCoreId(coreId) && quantumRemaining[coreId] > 0) {
        quantumRemaining[coreId]--;
    }
}

bool CoreManager::isQuantumExpired(int coreId) const {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    if (isValidCoreId(coreId)) {
        return quantumRemaining[coreId] <= 0 && !coreAssignments[coreId].empty();
    }
    
    return false;
}

std::vector<String> CoreManager::getAllAssignments() const {
    std::lock_guard<std::mutex> lock(coreMutex);
    return coreAssignments;
}

std::vector<String> CoreManager::getNonEmptyAssignments() const {
    std::lock_guard<std::mutex> lock(coreMutex);
    std::vector<String> result;
    
    for (const String& assignment : coreAssignments) {
        if (!assignment.empty()) {
            result.push_back(assignment);
        }
    }
    
    return result;
}

std::vector<int> CoreManager::getAvailableCores() const {
    std::lock_guard<std::mutex> lock(coreMutex);
    std::vector<int> result;
    
    for (int i = 0; i < numCores; i++) {
        if (coreAssignments[i].empty()) {
            result.push_back(i);
        }
    }
    
    return result;
}

std::vector<int> CoreManager::getUsedCores() const {
    std::lock_guard<std::mutex> lock(coreMutex);
    std::vector<int> result;
    
    for (int i = 0; i < numCores; i++) {
        if (!coreAssignments[i].empty()) {
            result.push_back(i);
        }
    }
    
    return result;
}

int CoreManager::getCoreCount() const {
    return numCores;
}

int CoreManager::getUsedCoreCount() const {
    std::lock_guard<std::mutex> lock(coreMutex);
    int count = 0;
    
    for (const String& assignment : coreAssignments) {
        if (!assignment.empty()) {
            count++;
        }
    }
    
    return count;
}

int CoreManager::getAvailableCoreCount() const {
    return numCores - getUsedCoreCount();
}

std::vector<CoreManager::CoreInfo> CoreManager::getActiveProcessesWithQuantum() const {
    std::lock_guard<std::mutex> lock(coreMutex);
    std::vector<CoreInfo> result;
    
    for (int i = 0; i < numCores; i++) {
        if (!coreAssignments[i].empty()) {
            result.push_back({
                i,
                coreAssignments[i],
                quantumRemaining[i],
                quantumRemaining[i] <= 0
            });
        }
    }
    
    return result;
}

void CoreManager::updateQuantums() {
    std::lock_guard<std::mutex> lock(coreMutex);
    
    for (int i = 0; i < numCores; i++) {
        if (!coreAssignments[i].empty() && quantumRemaining[i] > 0) {
            quantumRemaining[i]--;
        }
    }
}

bool CoreManager::isValidCoreId(int coreId) const {
    return coreId >= 0 && coreId < numCores;
} 