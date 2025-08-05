#pragma once
#include "TypedefRepo.h"
#include "CoreManager.h"
#include <vector>
#include <map>
#include <queue>

class Process;

struct MemoryBlock {
    int startAddress;
    int size;
    String processName;
    bool isAllocated;

    MemoryBlock(int start, int blockSize, const String& procName = "", bool allocated = false)
        : startAddress(start), size(blockSize), processName(procName), isAllocated(allocated) {
    }
};

struct FrameInfo {
    String processName;
    int pageNumber;
    bool isOccupied;
    bool referenced; // for clocked algo
};

class MemoryManager {
private:
    int totalMemorySize;
    int frameSize;
    int numFrames;
    int processMemorySize;
    int pagedInCount = 0;
    int pagedOutCount = 0;
    int clockHand = 0;  // Points to next frame to consider for replacement


    std::vector<MemoryBlock> memoryBlocks;          // For non-paging allocation (legacy)
    std::vector<bool> freeFrameList;                // true if frame is free
    std::vector<FrameInfo> frameTable;              // frameTable[frameNumber] = info
    std::queue<int> freeFrames;                     // Available frames
    std::map<String, int> processToMemoryMap;       // processName -> startAddress (for block allocation)

    std::map<std::pair<String, int>, int> pageTable;
    const std::map<std::pair<String, int>, int>& getPageTable() const {
        return pageTable;
    }

    void mergeAdjacentFreeBlocks();
    int calculateExternalFragmentation() const;
    std::unordered_map<String, std::shared_ptr<Process>> allProcesses;

public:
    MemoryManager();
    ~MemoryManager() = default;

    // Demand paging
    int allocatePage(Process* proc, int pageNumber); // Returns frameNumber or -1 on fail
    bool deallocatePage(const String& processName, int pageNumber); // Optional for replacement

    int getPagedInCount() const { return pagedInCount; }
    int getPagedOutCount() const { return pagedOutCount; }

    // TOBEDELETED: MO2 backing store - real data storage functionality
    void savePageToBackingStore(const String& processName, int pageNumber, const std::unordered_map<uint32_t, uint16_t>& pageData);
    bool loadPageFromBackingStore(const String& processName, int pageNumber, std::unordered_map<uint32_t, uint16_t>& pageData);

    // Whole-process allocation (FCFS-style)
    bool allocateMemory(const String& processName);
    void markPageAccessed(int frameNumber);
    bool deallocateMemory(const String& processName);

    // Memory status
    bool hasMemoryFor(const String& processName) const;
    int getProcessesInMemory() const;
    int getExternalFragmentationKB() const;

    // Visualization
    String generateASCIIMemoryMap() const;
    void generateMemorySnapshot(int quantumCycle) const;

    void dumpBackingStoreToFile(const std::string& filename = "csopesy-backing-store.txt") const;


    // Debug
    void printMemoryStatus() const;

    void setProcessMap(const std::unordered_map<String, std::shared_ptr<Process>>& map);


    // Accessors
    int getUsedFrameCount() const;
    int getFreeFrameCount() const;
    int getTotalFrames() const;
    int getFrameSize() const;
};
