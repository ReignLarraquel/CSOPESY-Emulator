#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <map>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <fstream>
#include <atomic>

enum class ScreenType {
    MAIN_CONSOLE,
    PROCESS_CONSOLE
};

enum class ProcessStatus {
    Waiting,
    Running,
    Finished
};

class Process {
public:
    std::string name;
    std::string timestamp;
    std::atomic<int> progress{ 0 };
    int assignedCore = -1;
    ProcessStatus status = ProcessStatus::Waiting;

    // Default constructor
    Process() = default;

    // Parameterized constructor
    Process(const std::string& processName) : name(processName) {
        timestamp = getCurrentTimestamp();
    }

    // Copy constructor (required because of std::atomic)
    Process(const Process& other)
        : name(other.name),
        timestamp(other.timestamp),
        progress(other.progress.load()),
        assignedCore(other.assignedCore),
        status(other.status) {
    }

    // Copy assignment operator (also required)
    Process& operator=(const Process& other) {
        if (this != &other) {
            name = other.name;
            timestamp = other.timestamp;
            progress.store(other.progress.load());
            assignedCore = other.assignedCore;
            status = other.status;
        }
        return *this;
    }

private:
    std::string getCurrentTimestamp() {
        std::time_t now = std::time(0);
        std::tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << (localTime.tm_mon + 1) << "/"
            << std::setfill('0') << std::setw(2) << localTime.tm_mday << "/"
            << (localTime.tm_year + 1900) << ", ";

        int hour = localTime.tm_hour;
        std::string ampm = (hour >= 12) ? "PM" : "AM";
        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;

        oss << std::setfill('0') << std::setw(2) << hour << ":"
            << std::setfill('0') << std::setw(2) << localTime.tm_min << ":"
            << std::setfill('0') << std::setw(2) << localTime.tm_sec << " " << ampm;

        return oss.str();
    }
};

ScreenType currentScreen = ScreenType::MAIN_CONSOLE;
std::map<std::string, Process> processes;
std::queue<std::string> fcfsQueue;
std::mutex queueMutex;
bool schedulerActive = false;
bool stopScheduler = false;
std::string currentProcessName = "";

void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void printLogo() {
    std::cout << "  ___  ____   __   ____  ____  ____  _  _ " << std::endl;
    std::cout << " / __)/ ___) /  \\ (  _ \\(  __)/ ___)( \\/ )" << std::endl;
    std::cout << "( (__ \\___ \\(  O ) ) __/ ) _) \\___ \\ )  / " << std::endl;
    std::cout << " \\___)(____/ \\__/ (__)  (____)(____/(__/  " << std::endl;
}

std::string toLower(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = std::tolower(c);
    }
    return result;
}

void runScheduler(int coreId) {
    while (!stopScheduler) {
        std::string procName;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!fcfsQueue.empty()) {
                procName = fcfsQueue.front();
                fcfsQueue.pop();

                Process& p = processes[procName];
                p.status = ProcessStatus::Running;
                p.assignedCore = coreId;
            }
        }

        if (!procName.empty()) {
            Process& p = processes[procName];
            std::ofstream out(procName + ".txt");
            for (int i = 0; i < 100; ++i) {
                out << p.timestamp << " Core: " << coreId
                    << " \"Hello world from " << procName << "!\"\n";
                p.progress = i + 1;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            p.status = ProcessStatus::Finished;
            out.close();
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

class Console {
public:
    virtual ~Console() = default;
    virtual void run() = 0;
    virtual void display() = 0;
};

class MainConsole : public Console {
public:
    void display() override {
        clearScreen();
        printLogo();
        std::cout << "\033[32mHello. Welcome to CSOPESY commandLine!\033[0m" << std::endl;
        std::cout << "Type \033[33m'exit'\033[0m to quit, \033[33m'clear'\033[0m to clear the screen" << std::endl;
        std::cout << "Enter a command: " << std::endl;
    }

    void run() override {
        display();
        std::string command;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, command);
            if (handleCommand(command)) return;
        }
    }

private:
    bool handleCommand(const std::string& command) {
        std::string lower = toLower(command);
        if (lower == "clear") display();
        else if (lower == "exit") return true;
        else if (lower == "screen -ls") {
            if (processes.empty()) std::cout << "No active processes.\n";
            else {
                std::cout << "-------------------------------------------------\n";
                std::cout << "\nRunning processes:\n";
                for (const auto& [n, p] : processes) {
                    if (p.status == ProcessStatus::Running)
                        std::cout << n << " (" << p.timestamp << ") Core: "
                        << p.assignedCore << " Progress: "
                        << p.progress << "/100\n";
                }
                std::cout << "\nFinished processes:\n";
                for (const auto& [n, p] : processes) {
                    if (p.status == ProcessStatus::Finished)
                        std::cout << n << " (" << p.timestamp << ")\n";
                }
                std::cout << "\nWaiting processes:\n";
                for (const auto& [n, p] : processes) {
                    if (p.status == ProcessStatus::Waiting)
                        std::cout << n << " (" << p.timestamp << ")\n";
                }
                std::cout << "-------------------------------------------------\n";
            }
        }
        else if (lower.find("screen -s ") == 0) {
            std::string name = command.substr(10);
            Process proc(name);
            processes[name] = proc;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                fcfsQueue.push(name);
            }
            std::cout << "Process " << name << " created.\n";
        }
        else if (lower == "scheduler-test") {
            if (!schedulerActive) {
                schedulerActive = true;
                for (int i = 0; i < 4; ++i)
                    std::thread(runScheduler, i).detach();
                std::cout << "Scheduler started.\n";
            }
        }
        else if (lower == "scheduler-stop") {
            stopScheduler = true;
            std::cout << "Scheduler stopping...\n";
        }
        else std::cout << "Unknown command.\n";
        return false;
    }
};

class ProcessConsole : public Console {
public:
    void display() override {
        clearScreen();
        std::cout << "Process name: " << currentProcessName << std::endl;
    }

    void run() override {
        display();
        std::string command;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, command);
            if (toLower(command) == "exit") {
                currentScreen = ScreenType::MAIN_CONSOLE;
                break;
            }
        }
    }
};

int main() {
    bool running = true;
    MainConsole mainConsole;
    ProcessConsole processConsole;
    while (running) {
        if (currentScreen == ScreenType::MAIN_CONSOLE) {
            mainConsole.run();
            if (currentScreen == ScreenType::MAIN_CONSOLE) running = false;
        }
        else if (currentScreen == ScreenType::PROCESS_CONSOLE) {
            processConsole.run();
        }
    }
    return 0;
}
