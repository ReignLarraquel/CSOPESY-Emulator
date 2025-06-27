#pragma once
#include "TypedefRepo.h"
#include "AConsole.h"

class MarqueeConsole : public AConsole {
public:
    MarqueeConsole();
    ~MarqueeConsole() = default;
    
    void onEnabled() override;
    void display() override;
    void process() override;
};
