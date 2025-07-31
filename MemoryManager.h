#pragma once
#include "TypedefRepo.h"
#include <vector>
#include <map>

struct MemoryBlock {
    int startAddress;
    int size;
    String processName;
    bool isAllocated;
    
    MemoryBlock(int start, int blockSize, const String& procName = "", bool allocated = false)
        : startAddress(start), size(blockSize), processName(procName), isAllocated(allocated) {}
};

class MemoryManager {
private:
    int totalMemorySize;
    int frameSize;
    int processMemorySize;
    std::vector<MemoryBlock> memoryBlocks;
    std::map<String, int> processToMemoryMap; // processName -> startAddress
    
    void mergeAdjacentFreeBlocks();
    int calculateExternalFragmentation() const;
    
public:
    MemoryManager();
    ~MemoryManager() = default;
    
    // Memory allocation/deallocation
    bool allocateMemory(const String& processName);
    bool deallocateMemory(const String& processName);
    
    // Memory status
    bool hasMemoryFor(const String& processName) const;
    int getProcessesInMemory() const;
    int getExternalFragmentationKB() const;
    
    // Visualization
    String generateASCIIMemoryMap() const;
    void generateMemorySnapshot(int quantumCycle) const;
    
    // Debugging
    void printMemoryStatus() const;
}; 