#pragma once
#include "TypedefRepo.h"
#include <vector>
#include <mutex>

class CoreManager {
public:
    CoreManager(int numCores);
    ~CoreManager();
    
    // Core assignment operations
    bool tryAssignProcess(int coreId, const String& processName);
    void clearAssignment(int coreId);
    String getAssignment(int coreId) const;
    bool isCoreAvailable(int coreId) const;
    
    // Quantum management (for Round Robin)
    void setQuantum(int coreId, int quantum);
    int getQuantum(int coreId) const;
    void decrementQuantum(int coreId);
    bool isQuantumExpired(int coreId) const;
    
    // Bulk operations
    std::vector<String> getAllAssignments() const;
    std::vector<String> getNonEmptyAssignments() const;
    std::vector<int> getAvailableCores() const;
    std::vector<int> getUsedCores() const;
    
    // Core statistics
    int getCoreCount() const;
    int getUsedCoreCount() const;
    int getAvailableCoreCount() const;
    
    // Batch processing for tick operations
    struct CoreInfo {
        int coreId;
        String processName;
        int quantumRemaining;
        bool quantumExpired;
    };
    
    std::vector<CoreInfo> getActiveProcessesWithQuantum() const;
    void updateQuantums();  // Decrements all active quantum counters
    int getActiveTicks() const { return activeTicks; }
    int getIdleTicks() const { return idleTicks; }
    int getTotalTicks() const { return totalTicks; }

    
private:
    std::vector<String> coreAssignments;
    std::vector<int> quantumRemaining;
    mutable std::mutex coreMutex;
    int numCores;

    int activeTicks = 0;
    int idleTicks = 0;
    int totalTicks = 0;
    
    // Private helpers (assume lock held)
    bool isValidCoreId(int coreId) const;
}; 