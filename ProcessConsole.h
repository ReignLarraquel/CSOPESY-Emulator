#pragma once
#include "TypedefRepo.h"
#include "AConsole.h"
#include "process.h"
#include "Scheduler.h"
#include <memory>

class ProcessConsole : public AConsole {
public:
    ProcessConsole(CPUScheduler& scheduler, std::shared_ptr<Process> process);
    ~ProcessConsole() = default;

    void onEnabled() override;
    void display() override;
    void process() override;

private:
    CPUScheduler& scheduler;
    std::shared_ptr<Process> attachedProcess;
    bool handleCommand(const String& command);

    void showProcessInfo();     // process-smi
    void exitToMain();          // exit

    void showProcessHeader();
    void showErrorMessage(const String& error);
};
