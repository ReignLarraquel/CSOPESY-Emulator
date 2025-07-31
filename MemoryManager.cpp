#include "MemoryManager.h"
#include "Config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

MemoryManager::MemoryManager() {
    totalMemorySize = Config::getMaxOverallMem();
    frameSize = Config::getMemPerFrame();
    processMemorySize = Config::getMemPerProc();
    
    // Initialize with one large free block
    memoryBlocks.push_back(MemoryBlock(0, totalMemorySize, "", false));
    
    std::cout << "Memory Manager initialized:" << std::endl;
    std::cout << "  Total memory: " << totalMemorySize << " bytes" << std::endl;
    std::cout << "  Frame size: " << frameSize << " bytes" << std::endl;
    std::cout << "  Process memory size: " << processMemorySize << " bytes" << std::endl;
}

bool MemoryManager::allocateMemory(const String& processName) {
    // Check if process already has memory allocated
    if (processToMemoryMap.find(processName) != processToMemoryMap.end()) {
        return true; // Already allocated
    }
    
    // First-fit algorithm: find first available block large enough
    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
        if (!it->isAllocated && it->size >= processMemorySize) {
            // Found suitable block
            int startAddr = it->startAddress;
            
            // If block is exactly the right size, just mark it as allocated
            if (it->size == processMemorySize) {
                it->isAllocated = true;
                it->processName = processName;
            } else {
                // Split the block: create allocated block and remaining free block
                MemoryBlock allocatedBlock(startAddr, processMemorySize, processName, true);
                MemoryBlock remainingBlock(startAddr + processMemorySize, 
                                         it->size - processMemorySize, "", false);
                
                // Replace current block with allocated block
                *it = allocatedBlock;
                // Insert remaining free block after the allocated one
                memoryBlocks.insert(it + 1, remainingBlock);
            }
            
            processToMemoryMap[processName] = startAddr;
            return true;
        }
    }
    
    // No suitable block found
    return false;
}

bool MemoryManager::deallocateMemory(const String& processName) {
    auto mapIt = processToMemoryMap.find(processName);
    if (mapIt == processToMemoryMap.end()) {
        return false; // Process not found
    }
    
    int startAddr = mapIt->second;
    processToMemoryMap.erase(mapIt);
    
    // Find and deallocate the memory block
    for (auto& block : memoryBlocks) {
        if (block.isAllocated && block.startAddress == startAddr && 
            block.processName == processName) {
            block.isAllocated = false;
            block.processName = "";
            break;
        }
    }
    
    // Merge adjacent free blocks
    mergeAdjacentFreeBlocks();
    return true;
}

void MemoryManager::mergeAdjacentFreeBlocks() {
    // Sort blocks by start address
    std::sort(memoryBlocks.begin(), memoryBlocks.end(), 
              [](const MemoryBlock& a, const MemoryBlock& b) {
                  return a.startAddress < b.startAddress;
              });
    
    // Merge adjacent free blocks
    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end() - 1; ) {
        if (!it->isAllocated && !(it + 1)->isAllocated &&
            it->startAddress + it->size == (it + 1)->startAddress) {
            // Merge current block with next block
            it->size += (it + 1)->size;
            memoryBlocks.erase(it + 1);
        } else {
            ++it;
        }
    }
}

bool MemoryManager::hasMemoryFor(const String& processName) const {
    // Check if process already has memory
    if (processToMemoryMap.find(processName) != processToMemoryMap.end()) {
        return true;
    }
    
    // Check if there's a free block large enough
    for (const auto& block : memoryBlocks) {
        if (!block.isAllocated && block.size >= processMemorySize) {
            return true;
        }
    }
    return false;
}

int MemoryManager::getProcessesInMemory() const {
    return static_cast<int>(processToMemoryMap.size());
}

int MemoryManager::calculateExternalFragmentation() const {
    int totalFreeMemory = 0;
    int largestFreeBlock = 0;
    
    for (const auto& block : memoryBlocks) {
        if (!block.isAllocated) {
            totalFreeMemory += block.size;
            largestFreeBlock = std::max(largestFreeBlock, block.size);
        }
    }
    
    // External fragmentation = total free memory - largest free block
    return totalFreeMemory - largestFreeBlock;
}

int MemoryManager::getExternalFragmentationKB() const {
    return calculateExternalFragmentation() / 1024;
}

String MemoryManager::generateASCIIMemoryMap() const {
    std::stringstream ss;
    
    // Sort blocks by start address for display
    std::vector<MemoryBlock> sortedBlocks = memoryBlocks;
    std::sort(sortedBlocks.begin(), sortedBlocks.end(), 
              [](const MemoryBlock& a, const MemoryBlock& b) {
                  return a.startAddress < b.startAddress;
              });
    
    // Generate ASCII representation from top to bottom (high to low addresses)
    ss << "----end---- = " << totalMemorySize << std::endl;
    
    // Reverse iteration to show from high to low addresses
    for (auto it = sortedBlocks.rbegin(); it != sortedBlocks.rend(); ++it) {
        int endAddr = it->startAddress + it->size;
        ss << endAddr << std::endl;
        
        if (it->isAllocated) {
            ss << it->processName << std::endl;
        }
        
        ss << it->startAddress << std::endl;
        ss << std::endl;
    }
    
    ss << "----start----- = 0" << std::endl;
    
    return ss.str();
}

void MemoryManager::generateMemorySnapshot(int quantumCycle) const {
    std::stringstream filename;
    filename << "memory_stamp_" << std::setfill('0') << std::setw(2) << quantumCycle << ".txt";
    
    std::ofstream file(filename.str());
    if (!file.is_open()) {
        std::cerr << "Error: Could not create memory snapshot file: " << filename.str() << std::endl;
        return;
    }
    
    // Get current timestamp
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);
    
    file << "Timestamp: (" << std::put_time(&tm, "%m/%d/%Y %I:%M:%S%p") << ")" << std::endl;
    file << "Number of processes in memory: " << getProcessesInMemory() << std::endl;
    file << "Total external fragmentation in KB: " << getExternalFragmentationKB() << std::endl;
    file << std::endl;
    file << generateASCIIMemoryMap();
    
    file.close();
}

void MemoryManager::printMemoryStatus() const {
    std::cout << "=== Memory Status ===" << std::endl;
    std::cout << "Processes in memory: " << getProcessesInMemory() << std::endl;
    std::cout << "External fragmentation: " << getExternalFragmentationKB() << " KB" << std::endl;
    std::cout << std::endl;
    std::cout << generateASCIIMemoryMap() << std::endl;
} 