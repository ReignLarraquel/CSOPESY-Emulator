#pragma once
#include "TypedefRepo.h"
#include "AConsole.h"

class SchedulingConsole : public AConsole {
public:
    SchedulingConsole();
    ~SchedulingConsole() = default;
    
    void onEnabled() override;
    void display() override;
    void process() override;
};
