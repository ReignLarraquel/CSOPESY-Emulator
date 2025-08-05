#include "Scheduler.h"
#include "Config.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "MainConsole.h"

CPUScheduler::CPUScheduler() 
    : processManager(),
      coreManager(Config::getNumCpu()),
      schedulerRunning(false), 
      generatorRunning(false), 
      cpuTicks(0), 
      nextProcessId(1) {
    
    startTime = std::chrono::steady_clock::now();
}

CPUScheduler::~CPUScheduler() {
    // Force immediate stop for cleanup
    schedulerRunning.store(false);
    generatorRunning.store(false);
    
    // Wait for all threads to complete
    if (tickThread.joinable()) {
        tickThread.join();
    }
    if (generatorThread.joinable()) {
        generatorThread.join();
    }
}

// TOBEDELETED: Start CPU execution immediately (called during initialize)
void CPUScheduler::startCpuExecution() {
    if (schedulerRunning.load()) return;
    
    schedulerRunning.store(true);
    String algorithm = Config::getScheduler();
    
    std::cout << "Starting " << algorithm << " CPU with " 
              << coreManager.getCoreCount() << " cores..." << std::endl;
    
    // TOBEDELETED: Start the master CPU tick thread - this makes CPU "active"
    tickThread = std::thread(&CPUScheduler::cpuTickManager, this);
}

// TOBEDELETED: Stop CPU execution (called during shutdown)
void CPUScheduler::stopCpuExecution() {
    schedulerRunning.store(false);
    if (tickThread.joinable()) {
        tickThread.join();
    }
}

// TOBEDELETED: scheduler-start command - now only controls process generation
void CPUScheduler::start() {
    if (generatorRunning.load()) {
        std::cout << "\033[33mAutomatic process generation is already running.\033[0m" << std::endl;
        return;
    }
    
    std::cout << "Starting automatic process generation..." << std::endl;
    startProcessGeneration();
}

// TOBEDELETED: scheduler-stop command - now only stops process generation
void CPUScheduler::stop() {
    if (!generatorRunning.load()) {
        std::cout << "\033[33mAutomatic process generation is not running.\033[0m" << std::endl;
        return;
    }
    
    std::cout << "Stopping automatic process generation..." << std::endl;
    std::cout << "CPU will continue executing existing processes." << std::endl;
    
    // TOBEDELETED: Stop generating new processes immediately
    // TOBEDELETED: CPU execution continues - only stop the process generator
    stopProcessGeneration();
}

void CPUScheduler::addProcess(std::shared_ptr<Process> process) {
    if (!process) return;

    String name = process->getName();

    // Add to process manager
    processManager.addProcess(process);

    // Set the memory manager reference in the process
    process->setMemoryManager(&memoryManager);

    // Convert to unordered_map and update MemoryManager
    std::unordered_map<String, std::shared_ptr<Process>> map;
    for (const auto& pname : processManager.getAllProcessNames()) {
        auto p = processManager.getProcess(pname);
        if (p) map[pname] = p;
    }
    memoryManager.setProcessMap(map);

    // Add to appropriate queue
    {
        std::lock_guard<std::mutex> queueLock(queueMutex);
        String algorithm = Config::getScheduler();

        if (algorithm == "fcfs") {
            fcfsQueue.push(name);
        }
        else if (algorithm == "rr") {
            roundRobinQueue.push_back(name);
        }
    }

    process->setStatus(ProcessStatus::Waiting);
}

// MASTER CPU TICK MANAGER - DRIVES THE ENTIRE SYSTEM!
void CPUScheduler::cpuTickManager() {
    while (schedulerRunning.load()) {
        cpuTicks++;
        
        // NEW CLEAN ARCHITECTURE - NO DEADLOCKS!
        onCpuTick();
        
        // TOBEDELETED: Only auto-shutdown if generator was explicitly stopped (not if it was never started)
        // TOBEDELETED: This prevents CPU from shutting down when user creates processes manually
        // TOBEDELETED: Commented out auto-shutdown logic - CPU should run until explicitly stopped
        /*
        // Check if scheduler-stop was called and all processes are finished
        if (!generatorRunning.load()) {
            // Check if all processes are finished (including sleeping ones)
            auto runningProcesses = processManager.getProcessesByStatus(ProcessStatus::Running);
            auto waitingProcesses = processManager.getProcessesByStatus(ProcessStatus::Waiting);
            auto sleepingProcesses = processManager.getProcessesByStatus(ProcessStatus::Sleeping);
            
            if (runningProcesses.empty() && waitingProcesses.empty() && sleepingProcesses.empty()) {
                std::cout << "All processes completed. Scheduler stopped." << std::endl;
                schedulerRunning.store(false);
                break;
            }
        }
        */
        
        // Sleep for tick interval (1ms = 1000 ticks per second)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// CALLED EVERY CPU TICK - CLEAN SEPARATION OF CONCERNS!
void CPUScheduler::onCpuTick() {
    // Phase 1: Execute instructions for running processes
    handleProcessExecution();
    
    // Phase 2: Handle sleeping processes (decrement sleep counters)
    handleSleepingProcesses();
    
    // Phase 3: Handle completed processes
    handleProcessCompletion();
    
    // Phase 4: Handle quantum expiration (Round Robin)
    handleQuantumExpiration();
    
    // Phase 5: Schedule new processes to available cores
    scheduleWaitingProcesses();
    
    // Phase 6: Generate memory snapshot every quantum cycle
    int quantumCycles = Config::getQuantumCycles();
    // TOBEDELETED: Only generate memory snapshots if quantumCycles > 0 (avoid division by zero in FCFS)
    if (quantumCycles > 0 && cpuTicks.load() % quantumCycles == 0) {
        int currentQuantum = static_cast<int>(cpuTicks.load() / quantumCycles);
        memoryManager.generateMemorySnapshot(currentQuantum);
    }
}

void CPUScheduler::handleProcessExecution() {
    // Get all currently running processes
    auto runningProcesses = coreManager.getNonEmptyAssignments();

    // Execute one instruction for each running process
    auto results = processManager.executeInstructionsForProcesses(runningProcesses);

    for (const auto& result : results) {
        if (result.isFinished) {
            // Remove from core and mark finished
            for (int coreId = 0; coreId < coreManager.getCoreCount(); coreId++) {
                if (coreManager.getAssignment(coreId) == result.name) {
                    coreManager.clearAssignment(coreId);
                    processManager.updateProcessStatus(result.name, ProcessStatus::Finished);
                    break;
                }
            }
        }
        else if (result.process && result.process->getStatus() == ProcessStatus::Sleeping) {
            // Remove from core
            for (int coreId = 0; coreId < coreManager.getCoreCount(); coreId++) {
                if (coreManager.getAssignment(coreId) == result.name) {
                    coreManager.clearAssignment(coreId);
                    processManager.setProcessCore(result.name, -1);
                    break;
                }
            }
            // Return to ready queue (optional depending on scheduler)
            if (Config::getScheduler() == "fcfs") {
                fcfsQueue.push(result.name);
            }
            else if (Config::getScheduler() == "rr") {
                roundRobinQueue.push_back(result.name);
            }
        }
    }
}

void CPUScheduler::handleSleepingProcesses() {
    // Get all sleeping processes and update their sleep counters
    auto sleepingProcesses = processManager.getProcessesByStatus(ProcessStatus::Sleeping);
    
    for (const String& processName : sleepingProcesses) {
        auto process = processManager.getProcess(processName);
        if (process && process->getRemainingInstructions() > 0) {
            // Process decrements its own sleep counter in executeInstruction
            // When sleep ends, process automatically returns to Waiting status
            process->executeInstruction();
            
            // If sleep finished, add back to waiting queue
            if (process->getStatus() == ProcessStatus::Waiting) {
                std::lock_guard<std::mutex> queueLock(queueMutex);
                String algorithm = Config::getScheduler();
                
                if (algorithm == "fcfs") {
                    fcfsQueue.push(processName);
                } else if (algorithm == "rr") {
                    roundRobinQueue.push_back(processName);
                }
            }
        }
    }
}

void CPUScheduler::handleProcessCompletion() {
    // Get all finished processes and deallocate their memory
    auto finishedProcesses = processManager.getProcessesByStatus(ProcessStatus::Finished);
    
    for (const String& processName : finishedProcesses) {
        // Deallocate memory when process finishes
        memoryManager.deallocateMemory(processName);
    }
    
    // Note: Keep finished processes in ProcessManager for reporting purposes
    // They should only be removed from cores/queues, not from memory
    // This allows 'screen -ls' and 'report-util' to show completed processes
}

void CPUScheduler::handleQuantumExpiration() {
    String algorithm = Config::getScheduler();
    if (algorithm != "rr") return;  // Only for Round Robin
    
    // Update quantum for all cores
    coreManager.updateQuantums();
    
    // Get cores with expired quantum
    auto coreInfos = coreManager.getActiveProcessesWithQuantum();
    
    std::vector<String> preemptedProcesses;
    for (const auto& coreInfo : coreInfos) {
        if (coreInfo.quantumExpired) {
            // Clear core assignment FIRST
            coreManager.clearAssignment(coreInfo.coreId);
            
            // Then update process state (core should be -1 AFTER status change)
            processManager.updateProcessStatus(coreInfo.processName, ProcessStatus::Waiting);
            processManager.setProcessCore(coreInfo.processName, -1);
            
            // Add back to Round Robin queue
            preemptedProcesses.push_back(coreInfo.processName);
        }
    }
    
    // Add preempted processes back to queue (exclude finished ones)
    if (!preemptedProcesses.empty()) {
        std::lock_guard<std::mutex> queueLock(queueMutex);
        for (const String& processName : preemptedProcesses) {
            auto process = processManager.getProcess(processName);
            if (process && process->getStatus() != ProcessStatus::Finished) {
                roundRobinQueue.push_back(processName);
            }
        }
    }
}

void CPUScheduler::scheduleWaitingProcesses() {
    std::lock_guard<std::mutex> queueLock(queueMutex);

    String algorithm = Config::getScheduler();
    auto availableCores = coreManager.getAvailableCores();

    for (int coreId : availableCores) {
        String processName = "";

        if (algorithm == "fcfs" && !fcfsQueue.empty()) {
            processName = fcfsQueue.front();
            fcfsQueue.pop();
        }
        else if (algorithm == "rr" && !roundRobinQueue.empty()) {
            processName = roundRobinQueue.front();
            roundRobinQueue.pop_front();
        }

        if (processName.empty() || !processManager.hasProcess(processName)) {
            continue;
        }

        auto process = processManager.getProcess(processName);
        if (!process || process->getStatus() == ProcessStatus::Finished) {
            continue;
        }

        // TOBEDELETED: CRITICAL FIX - Check if memory is available before assigning to core
        // TOBEDELETED: This enforces the memory bottleneck - if no memory, process waits!
        if (!memoryManager.allocateMemory(processName)) {
            // TOBEDELETED: Not enough memory - put process back in queue and continue to next core
            if (algorithm == "fcfs") {
                fcfsQueue.push(processName);
            }
            else if (algorithm == "rr") {
                roundRobinQueue.push_front(processName);
            }
            continue;
        }

        if (coreManager.tryAssignProcess(coreId, processName)) {
            processManager.setProcessCore(processName, coreId);
            processManager.updateProcessStatus(processName, ProcessStatus::Running);

            if (algorithm == "rr") {
                coreManager.setQuantum(coreId, Config::getQuantumCycles());
            }
        }
        else {
            // TOBEDELETED: Core assignment failed - deallocate the reserved memory
            memoryManager.deallocateMemory(processName);
            
            if (algorithm == "fcfs") {
                fcfsQueue.push(processName);
            }
            else if (algorithm == "rr") {
                roundRobinQueue.push_front(processName);
            }
        }
    }
}


void CPUScheduler::startProcessGeneration() {
    if (generatorRunning.load()) return;
    
    generatorRunning.store(true);
    generatorThread = std::thread(&CPUScheduler::processGenerator, this);
}

void CPUScheduler::stopProcessGeneration() {
    generatorRunning.store(false);
    if (generatorThread.joinable()) {
        generatorThread.join();
    }
}

void CPUScheduler::processGenerator() {
    while (generatorRunning.load()) {
        int frequency = Config::getBatchProcessFreq();
        
        // Generate new process
        String processName = generateProcessName();
        auto process = createRandomProcess(processName);
        addProcess(process);
        
        // Wait for next generation cycle (250ms multiplier for reasonable rate)
        std::this_thread::sleep_for(std::chrono::milliseconds(frequency * 250));
    }
}

String CPUScheduler::generateProcessName() {
    std::ostringstream oss;
    oss << "p" << std::setfill('0') << std::setw(2) << nextProcessId++;
    return oss.str();
}

std::shared_ptr<Process> CPUScheduler::createRandomProcess(const String& name) {
    int minIns = Config::getMinIns();
    int maxIns = Config::getMaxIns();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> insDis(minIns, maxIns);

    int numInstructions = insDis(gen);

    // TOBEDELETED: MO2 specification - Use min/max-mem-per-proc range for auto-generated processes
    // TOBEDELETED: "Let P be the number of pages required by a process and M is the rolled value 
    // TOBEDELETED: between min-mem-per-proc and max-mem-proc. P can be computed as M/ mem-per-frame."
    int minMem = Config::getMinMemPerProc();
    int maxMem = Config::getMaxMemPerProc();
    int frameSize = Config::getMemPerFrame();
    
    // TOBEDELETED: Generate random value M between min and max
    std::uniform_int_distribution<> memDis(minMem, maxMem);
    int rolledValue = memDis(gen); // This is M
    
    // TOBEDELETED: Calculate P = M / mem-per-frame (number of pages required)
    int pagesRequired = (rolledValue + frameSize - 1) / frameSize; // TOBEDELETED: Ceiling division
    
    // TOBEDELETED: Calculate minimum memory size needed for P pages
    int minimumMemoryNeeded = pagesRequired * frameSize;
    
    // TOBEDELETED: Find smallest power of 2 that provides at least the required memory
    int memorySize = 64; // TOBEDELETED: Start with minimum [2^6]
    while (memorySize < minimumMemoryNeeded && memorySize < 65536) {
        memorySize *= 2;
    }
    
    // TOBEDELETED: Ensure we stay within bounds [64, 65536] = [2^6, 2^16]
    if (memorySize > 65536) memorySize = 65536;
    if (memorySize < 64) memorySize = 64;

    return std::make_shared<Process>(name, nextProcessId - 1, numInstructions, memorySize);
}


// Utility functions
bool CPUScheduler::isRunning() const { 
    return schedulerRunning.load(); 
}

double CPUScheduler::getCpuUtilization() const {
    int usedCores = coreManager.getUsedCoreCount();
    int totalCores = coreManager.getCoreCount();
    return totalCores > 0 ? (double(usedCores) / totalCores) * 100.0 : 0.0;
}

int CPUScheduler::getCoresUsed() const {
    return coreManager.getUsedCoreCount();
}

int CPUScheduler::getCoresAvailable() const {
    return coreManager.getAvailableCoreCount();
}

std::vector<int> CPUScheduler::getActiveCores() const {
    return coreManager.getUsedCores();
}

void CPUScheduler::removeProcess(const String& processName) {
    processManager.removeProcess(processName);
}

// Process information delegation methods
std::shared_ptr<Process> CPUScheduler::getProcess(const String& processName) const {
    return processManager.getProcess(processName);
}

std::vector<String> CPUScheduler::getProcessesByStatus(ProcessStatus status) const {
    return processManager.getProcessesByStatus(status);
}

std::vector<String> CPUScheduler::getAllProcessNames() const {
    return processManager.getAllProcessNames();
}

bool CPUScheduler::hasProcess(const String& processName) const {
    return processManager.hasProcess(processName);
}

void CPUScheduler::printMemoryStatus() const {
    memoryManager.printMemoryStatus();
} 

void CPUScheduler::dumpBackingStoreToFile(const std::string& filename) const {
    memoryManager.dumpBackingStoreToFile(filename);
}

int CPUScheduler::getNextProcessId() {
    return nextProcessId++;
}

void CPUScheduler::executeProcessDirectly(const String& processName) {
    auto process = processManager.getProcess(processName);
    if (!process) {
        std::cout << "Process not found: " << processName << std::endl;
        return;
    }

    process->setMemoryManager(&memoryManager);
    // Execute all instructions until the process is finished
    std::cout << "Executing instructions for process " << processName << "..." << std::endl;
    int executedCount = 0;

    while (!process->hasFinished()) {
        process->executeInstruction();
        executedCount++;

        // Debug output for larger instruction sets
        if (executedCount % 5 == 0) {
            std::cout << "Executed " << executedCount << " instructions..." << std::endl;
        }
    }

    // Make sure memory changes persist by marking pages as accessed
    for (const auto& [pageNum, entry] : process->getPageTable()) {
        if (entry.valid) {
            memoryManager.markPageAccessed(entry.frameNumber);
        }
    }

    std::cout << "Executed " << executedCount << " total instructions." << std::endl;

    // Ensure process is marked as finished
    process->setStatus(ProcessStatus::Finished);
}