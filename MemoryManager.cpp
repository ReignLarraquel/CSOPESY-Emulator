#include "MemoryManager.h"
#include "Config.h"
#include "Process.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>

MemoryManager::MemoryManager() {
    totalMemorySize = Config::getMaxOverallMem();
    frameSize = Config::getMemPerFrame();
    processMemorySize = Config::getMemPerProc();
    numFrames = totalMemorySize / frameSize;

    freeFrameList.resize(numFrames, true);
    frameTable.resize(numFrames, { "", -1, false, false });

    for (int i = 0; i < numFrames; ++i) {
        freeFrames.push(i);
    }

    memoryBlocks.push_back(MemoryBlock(0, totalMemorySize, "", false));

    std::cout << "Memory Manager initialized:" << std::endl;
    std::cout << "  Total memory: " << totalMemorySize << " bytes" << std::endl;
    std::cout << "  Frame size: " << frameSize << " bytes" << std::endl;
    std::cout << "  Process memory size: " << processMemorySize << " bytes" << std::endl;
}

void MemoryManager::setProcessMap(const std::unordered_map<String, std::shared_ptr<Process>>& map) {
    allProcesses = map;
}

int MemoryManager::allocatePage(Process* proc, int pageNumber) {
    auto& pageTable = proc->getPageTableRef();

    // Try free frame first
    for (int i = 0; i < numFrames; ++i) {
        if (freeFrameList[i]) {
            freeFrameList[i] = false;
            frameTable[i] = { proc->getName(), pageNumber, true, true }; // referenced = true
            pageTable[pageNumber] = { i, true, false };
            pagedInCount++;

            // TOBEDELETED: MO2 backing store - try to load page data from backing store
            std::unordered_map<uint32_t, uint16_t> pageData;
            if (loadPageFromBackingStore(proc->getName(), pageNumber, pageData)) {
                // TOBEDELETED: Restore page data from backing store
                for (const auto& [address, value] : pageData) {
                    proc->setMemoryValueAt(address, value);
                }
            }

            return i;
        }
    }

    // CLOCK ALGORITHM: Find a victim frame to evict
    while (true) {
        FrameInfo& frame = frameTable[clockHand];

        if (!frame.referenced) {
            int victimPage = frame.pageNumber;
            String victimProcessName = frame.processName;

            // TOBEDELETED: MO2 backing store - save victim page data before eviction
            auto victimProcess = allProcesses.find(victimProcessName);
            if (victimProcess != allProcesses.end()) {
                // TOBEDELETED: Get page data from victim process
                auto memoryDump = victimProcess->second->getMemoryDump();
                std::unordered_map<uint32_t, uint16_t> pageData;
                
                // TOBEDELETED: Extract data for this specific page
                int pageSize = Config::getMemPerFrame();
                uint32_t pageStartAddress = victimPage * pageSize;
                uint32_t pageEndAddress = pageStartAddress + pageSize;
                
                for (const auto& [address, value] : memoryDump) {
                    if (address >= pageStartAddress && address < pageEndAddress) {
                        pageData[address] = value;
                    }
                }
                
                // TOBEDELETED: Save page data to backing store if it has content
                if (!pageData.empty()) {
                    savePageToBackingStore(victimProcessName, victimPage, pageData);
                }
            }

            // Invalidate the victim in its process's page table
            for (const auto& [name, process] : allProcesses) {
                auto& pt = process->getPageTableRef();
                for (auto& entry : pt) {
                    if (entry.second.frameNumber == clockHand && entry.second.valid) {
                        entry.second.valid = false;
                        break;
                    }
                }
            }

            //std::cout << "Page replacement: Evicting page " << victimPage
                //<< " of process " << frame.processName << std::endl;

            pagedOutCount++;

            // Replace victim with the new page
            frame = { proc->getName(), pageNumber, true, true }; // referenced = true
            pageTable[pageNumber] = { clockHand, true, false };
            pagedInCount++;

            // TOBEDELETED: MO2 backing store - try to load page data from backing store
            std::unordered_map<uint32_t, uint16_t> pageData;
            if (loadPageFromBackingStore(proc->getName(), pageNumber, pageData)) {
                // TOBEDELETED: Restore page data from backing store
                for (const auto& [address, value] : pageData) {
                    proc->setMemoryValueAt(address, value);
                }
            }

            int allocatedFrame = clockHand;
            clockHand = (clockHand + 1) % numFrames;
            return allocatedFrame;
        }

        // Give second chance
        frame.referenced = false;
        clockHand = (clockHand + 1) % numFrames;
    }
}

void MemoryManager::markPageAccessed(int frameNumber) {
    if (frameNumber >= 0 && frameNumber < frameTable.size()) {
        frameTable[frameNumber].referenced = true;
    }
}

bool MemoryManager::allocateMemory(const String& processName) {
    if (processToMemoryMap.find(processName) != processToMemoryMap.end()) {
        return true;
    }

    auto procIt = allProcesses.find(processName);
    if (procIt == allProcesses.end()) return false;

    std::shared_ptr<Process> proc = procIt->second;
    if (!proc) return false;

    // Try to allocate a memory block (just for memory_stamp)
    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
        if (!it->isAllocated && it->size >= processMemorySize) {
            int startAddr = it->startAddress;
            if (it->size == processMemorySize) {
                it->isAllocated = true;
                it->processName = processName;
            }
            else {
                MemoryBlock allocatedBlock(startAddr, processMemorySize, processName, true);
                MemoryBlock remainingBlock(startAddr + processMemorySize,
                    it->size - processMemorySize, "", false);
                *it = allocatedBlock;
                memoryBlocks.insert(it + 1, remainingBlock);
            }

            processToMemoryMap[processName] = startAddr;

            // ❌ REMOVE eager page allocation
            // int pagesNeeded = processMemorySize / frameSize;
            // for (int i = 0; i < pagesNeeded; ++i) {
            //     allocatePage(proc.get(), i);
            // }

            // TOBEDELETED: Memory block reserved for process
            return true;
        }
    }

    return false;
}


bool MemoryManager::deallocateMemory(const String& processName) {
    auto mapIt = processToMemoryMap.find(processName);
    if (mapIt == processToMemoryMap.end()) {
        return false;
    }
    int startAddr = mapIt->second;
    processToMemoryMap.erase(mapIt);
    for (auto& block : memoryBlocks) {
        if (block.isAllocated && block.startAddress == startAddr && block.processName == processName) {
            block.isAllocated = false;
            block.processName = "";
            break;
        }
    }
    mergeAdjacentFreeBlocks();
    return true;
}

void MemoryManager::mergeAdjacentFreeBlocks() {
    std::sort(memoryBlocks.begin(), memoryBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.startAddress < b.startAddress;
        });
    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end() - 1;) {
        if (!it->isAllocated && !(it + 1)->isAllocated &&
            it->startAddress + it->size == (it + 1)->startAddress) {
            it->size += (it + 1)->size;
            memoryBlocks.erase(it + 1);
        }
        else {
            ++it;
        }
    }
}

bool MemoryManager::hasMemoryFor(const String& processName) const {
    if (processToMemoryMap.find(processName) != processToMemoryMap.end()) {
        return true;
    }
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
    return totalFreeMemory - largestFreeBlock;
}

int MemoryManager::getExternalFragmentationKB() const {
    return calculateExternalFragmentation() / 1024;
}

String MemoryManager::generateASCIIMemoryMap() const {
    std::stringstream ss;
    std::vector<MemoryBlock> sortedBlocks = memoryBlocks;
    std::sort(sortedBlocks.begin(), sortedBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.startAddress < b.startAddress;
        });
    ss << "----end---- = " << totalMemorySize << std::endl;
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

    auto now = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif

    int pagesUsed = 0;
    for (const auto& frame : frameTable) {
        if (!frame.processName.empty()) pagesUsed++;
    }

    file << "Timestamp: (" << std::put_time(&tm, "%m/%d/%Y %I:%M:%S%p") << ")\n";
    file << "Number of used frames: " << pagesUsed << "\n";
    file << "Total frames: " << frameTable.size() << "\n";
    file << "Frame size: " << frameSize << " bytes\n\n";

    file << "Frame | Process | Page # | Referenced\n";
    file << "--------------------------------------\n";

    for (int i = 0; i < frameTable.size(); ++i) {
        const FrameInfo& frame = frameTable[i];
        if (!frame.processName.empty()) {
            file << std::setw(5) << i << " | "
                << std::setw(7) << frame.processName << " | "
                << std::setw(6) << frame.pageNumber << " | "
                << (frame.referenced ? "Yes" : "No") << "\n";
        }
    }

    file.close();
}

void MemoryManager::printMemoryStatus() const {
    std::cout << "=== Memory Status ===" << std::endl;
    std::cout << "Processes in memory: " << getProcessesInMemory() << std::endl;
    std::cout << "External fragmentation: " << getExternalFragmentationKB() << " KB" << std::endl;
    std::cout << std::endl;
    std::cout << generateASCIIMemoryMap() << std::endl;
}

int MemoryManager::getUsedFrameCount() const {
    return static_cast<int>(std::count(freeFrameList.begin(), freeFrameList.end(), false)); // TOBEDELETED: Fix C4244 warning
}

int MemoryManager::getFreeFrameCount() const {
    return static_cast<int>(std::count(freeFrameList.begin(), freeFrameList.end(), true)); // TOBEDELETED: Fix C4244 warning
}

int MemoryManager::getTotalFrames() const {
    return numFrames;
}

int MemoryManager::getFrameSize() const {
    return frameSize;
}

void MemoryManager::dumpBackingStoreToFile(const std::string& filename) const {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Failed to open backing store file for writing.\n";
        return;
    }

    outFile << "=== Backing Store Dump ===\n\n";

    for (const auto& [processName, processPtr] : allProcesses) {
        outFile << "Process: " << processName << "\n";
        const auto& pageTable = processPtr->getPageTable();

        for (const auto& [pageNum, entry] : pageTable) {
            outFile << "  Page " << pageNum << " => "
                << (entry.valid ? "Frame " + std::to_string(entry.frameNumber) : "Not in memory")
                << "\n";
        }

        outFile << "\n";
    }

    outFile.close();
}

// TOBEDELETED: MO2 backing store - save actual page data to file
void MemoryManager::savePageToBackingStore(const String& processName, int pageNumber, const std::unordered_map<uint32_t, uint16_t>& pageData) {
    // TOBEDELETED: Create backing store data file with binary format for efficiency
    std::string filename = "csopesy-backing-store-data.bin";
    std::ofstream outFile(filename, std::ios::binary | std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error: Failed to open backing store data file for writing.\n";
        return;
    }
    
    // TOBEDELETED: Write page header: process name length, process name, page number, data count
    uint32_t nameLength = static_cast<uint32_t>(processName.length()); // TOBEDELETED: Fix C4267 warning
    outFile.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
    outFile.write(processName.c_str(), nameLength);
    outFile.write(reinterpret_cast<const char*>(&pageNumber), sizeof(pageNumber));
    
    uint32_t dataCount = static_cast<uint32_t>(pageData.size()); // TOBEDELETED: Fix C4267 warning
    outFile.write(reinterpret_cast<const char*>(&dataCount), sizeof(dataCount));
    
    // TOBEDELETED: Write page data: address-value pairs
    for (const auto& [address, value] : pageData) {
        outFile.write(reinterpret_cast<const char*>(&address), sizeof(address));
        outFile.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    
    outFile.close();
}

// TOBEDELETED: MO2 backing store - load actual page data from file
bool MemoryManager::loadPageFromBackingStore(const String& processName, int pageNumber, std::unordered_map<uint32_t, uint16_t>& pageData) {
    // TOBEDELETED: Read backing store data file to find the page
    std::string filename = "csopesy-backing-store-data.bin";
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) {
        return false; // TOBEDELETED: No backing store file exists yet
    }
    
    // TOBEDELETED: Search through the file for the matching process and page
    while (inFile.good()) {
        uint32_t nameLength;
        if (!inFile.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength))) {
            break; // TOBEDELETED: End of file
        }
        
        std::string storedProcessName(nameLength, '\0');
        if (!inFile.read(&storedProcessName[0], nameLength)) {
            break;
        }
        
        int storedPageNumber;
        if (!inFile.read(reinterpret_cast<char*>(&storedPageNumber), sizeof(storedPageNumber))) {
            break;
        }
        
        uint32_t dataCount;
        if (!inFile.read(reinterpret_cast<char*>(&dataCount), sizeof(dataCount))) {
            break;
        }
        
        if (storedProcessName == processName && storedPageNumber == pageNumber) {
            // TOBEDELETED: Found the page - load the data
            pageData.clear();
            for (uint32_t i = 0; i < dataCount; ++i) {
                uint32_t address;
                uint16_t value;
                if (!inFile.read(reinterpret_cast<char*>(&address), sizeof(address)) ||
                    !inFile.read(reinterpret_cast<char*>(&value), sizeof(value))) {
                    return false;
                }
                pageData[address] = value;
            }
            inFile.close();
            return true;
        } else {
            // TOBEDELETED: Skip this page's data
            for (uint32_t i = 0; i < dataCount; ++i) {
                uint32_t address;
                uint16_t value;
                if (!inFile.read(reinterpret_cast<char*>(&address), sizeof(address)) ||
                    !inFile.read(reinterpret_cast<char*>(&value), sizeof(value))) {
                    break;
                }
            }
        }
    }
    
    inFile.close();
    return false; // TOBEDELETED: Page not found in backing store
}
