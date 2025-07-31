#pragma once
#include "TypedefRepo.h"
#include "process.h"
#include "ProcessManager.h"
#include "CoreManager.h"
#include "MemoryManager.h"
#include <queue>
#include <deque>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

class CPUScheduler {
public:
    CPUScheduler();
    ~CPUScheduler();
    
    // Main scheduler control
    void start();
    void stop();
    bool isRunning() const;
    
    // Process management
    void addProcess(std::shared_ptr<Process> process);
    void removeProcess(const String& processName);
    
    // Auto process generation (scheduler-start functionality)
    void startProcessGeneration();
    void stopProcessGeneration();
    
    // CPU utilization and core info
    double getCpuUtilization() const;
    int getCoresUsed() const;
    int getCoresAvailable() const;
    
    // Core management
    std::vector<int> getActiveCores() const;
    
    // Process information (for MainConsole)
    std::shared_ptr<Process> getProcess(const String& processName) const;
    std::vector<String> getProcessesByStatus(ProcessStatus status) const;
    std::vector<String> getAllProcessNames() const;
    bool hasProcess(const String& processName) const;
    
    // Memory management
    void printMemoryStatus() const;
    
private:
    // TICK-DRIVEN ARCHITECTURE - CPU ticks drive everything!
    void cpuTickManager();           // Master tick generator
    void onCpuTick();               // Called every CPU tick - drives execution
    void processGenerator();
    
    // Scheduler algorithms are now integrated into onCpuTick phases
    
    // Thread management
    std::thread tickThread;
    std::thread generatorThread;
    
    // NEW ARCHITECTURE: Separated concerns
    ProcessManager processManager;          // Owns all process state
    CoreManager coreManager;               // Owns all core assignments
    MemoryManager memoryManager;           // Owns all memory allocations
    
    // Process queues (depending on algorithm)
    std::queue<String> fcfsQueue;           // FCFS ready queue
    std::deque<String> roundRobinQueue;     // RR ready queue (circular)
    
    // Thread synchronization (only for queues now!)
    std::mutex queueMutex;
    
    // Scheduler state
    std::atomic<bool> schedulerRunning;
    std::atomic<bool> generatorRunning;
    std::atomic<long long> cpuTicks;
    std::atomic<int> nextProcessId;
    
    // Timing
    std::chrono::steady_clock::time_point startTime;
    
    // Helper functions
    String generateProcessName();
    std::shared_ptr<Process> createRandomProcess(const String& name);
    
    // Scheduling operations (clean, no deadlocks!)
    void handleProcessExecution();    // Execute instructions for running processes
    void handleSleepingProcesses();   // Handle sleeping processes and wake them up
    void handleProcessCompletion();   // Remove finished processes
    void handleQuantumExpiration();   // Preempt processes whose quantum expired
    void scheduleWaitingProcesses();  // Assign waiting processes to available cores
}; 