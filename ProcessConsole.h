#pragma once
#include "TypedefRepo.h"
#include "AConsole.h"
#include "process.h"
#include <memory>

class ProcessConsole : public AConsole {
public:
    ProcessConsole(std::shared_ptr<Process> process = nullptr);
    ~ProcessConsole() = default;
    
    void onEnabled() override;
    void display() override;
    void process() override;

private:
    std::shared_ptr<Process> attachedProcess;
    bool handleCommand(const String& command);
    
    void showProcessInfo();     // process-smi
    void exitToMain();          // exit
    
    void showProcessHeader();
    void showErrorMessage(const String& error);
}; 