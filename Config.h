#pragma once
#include "TypedefRepo.h"

namespace Config {
    // Configuration parameters
    struct SystemConfig {
        int numCpu;
        String scheduler;
        int quantumCycles;
        int batchProcessFreq;
        int minIns;
        int maxIns;
        int delayPerExec;
        // Memory management parameters
        int maxOverallMem;
        int memPerFrame;
        int memPerProc;
    
        int minMemPerProc;
        int maxMemPerProc;
    };

    // Configuration management functions
    bool loadFromFile(const String& filename);
    void loadDefaults();
    
    // Getters for configuration values
    int getNumCpu();
    String getScheduler();
    int getQuantumCycles();
    int getBatchProcessFreq();
    int getMinIns();
    int getMaxIns();
    int getDelayPerExec();
    
    // Memory management getters
    int getMaxOverallMem();
    int getMemPerFrame();
    int getMemPerProc();
    

    int getMinMemPerProc();
    int getMaxMemPerProc();
    
    // System state
    bool isInitialized();
} 