#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Config {
    // Global configuration instance
    static SystemConfig g_config;
    static bool g_initialized = false;

    void loadDefaults() {
        g_config.numCpu = 4;
        g_config.scheduler = "fcfs";
        g_config.quantumCycles = 5;
        g_config.batchProcessFreq = 1;
        g_config.minIns = 1000;
        g_config.maxIns = 2000;
        g_config.delayPerExec = 0;
        // Memory management defaults
        g_config.maxOverallMem = 16384;
        g_config.memPerFrame = 16;
        g_config.memPerProc = 4096;
        g_initialized = true;
    }

    bool loadFromFile(const String& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Warning: Could not open " << filename << ". Using default values." << std::endl;
            loadDefaults();
            return false;
        }

        loadDefaults(); // Start with defaults
        
        String line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
            
            std::istringstream iss(line);
            String key, value;
            
            if (iss >> key >> value) {
                if (key == "num-cpu") {
                    g_config.numCpu = std::stoi(value);
                }
                else if (key == "scheduler") {
                    // Remove quotes if present
                    if (value.front() == '"' && value.back() == '"') {
                        value = value.substr(1, value.length() - 2);
                    }
                    g_config.scheduler = value;
                }
                else if (key == "quantum-cycles") {
                    g_config.quantumCycles = std::stoi(value);
                }
                else if (key == "batch-process-freq") {
                    g_config.batchProcessFreq = std::stoi(value);
                }
                else if (key == "min-ins") {
                    g_config.minIns = std::stoi(value);
                }
                else if (key == "max-ins") {
                    g_config.maxIns = std::stoi(value);
                }
                else if (key == "delay-per-exec") {
                    g_config.delayPerExec = std::stoi(value);
                }
                else if (key == "max-overall-mem") {
                    g_config.maxOverallMem = std::stoi(value);
                }
                else if (key == "mem-per-frame") {
                    g_config.memPerFrame = std::stoi(value);
                }
                else if (key == "mem-per-proc") {
                    g_config.memPerProc = std::stoi(value);
                }
            }
        }
        
        file.close();
        g_initialized = true;
        
        std::cout << "Configuration loaded successfully:" << std::endl;
        std::cout << "  num-cpu: " << g_config.numCpu << std::endl;
        std::cout << "  scheduler: " << g_config.scheduler << std::endl;
        std::cout << "  quantum-cycles: " << g_config.quantumCycles << std::endl;
        std::cout << "  batch-process-freq: " << g_config.batchProcessFreq << std::endl;
        std::cout << "  min-ins: " << g_config.minIns << std::endl;
        std::cout << "  max-ins: " << g_config.maxIns << std::endl;
        std::cout << "  delay-per-exec: " << g_config.delayPerExec << std::endl;
        std::cout << "  max-overall-mem: " << g_config.maxOverallMem << std::endl;
        std::cout << "  mem-per-frame: " << g_config.memPerFrame << std::endl;
        std::cout << "  mem-per-proc: " << g_config.memPerProc << std::endl;
        
        return true;
    }

    // Getter functions
    int getNumCpu() { return g_initialized ? g_config.numCpu : 4; }
    String getScheduler() { return g_initialized ? g_config.scheduler : "fcfs"; }
    int getQuantumCycles() { return g_initialized ? g_config.quantumCycles : 5; }
    int getBatchProcessFreq() { return g_initialized ? g_config.batchProcessFreq : 1; }
    int getMinIns() { return g_initialized ? g_config.minIns : 1000; }
    int getMaxIns() { return g_initialized ? g_config.maxIns : 2000; }
    int getDelayPerExec() { return g_initialized ? g_config.delayPerExec : 0; }
    
    // Memory management getters
    int getMaxOverallMem() { return g_initialized ? g_config.maxOverallMem : 16384; }
    int getMemPerFrame() { return g_initialized ? g_config.memPerFrame : 16; }
    int getMemPerProc() { return g_initialized ? g_config.memPerProc : 4096; }
    
    bool isInitialized() { return g_initialized; }
} 