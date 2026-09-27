// clang++ dummy.cpp -o dummy.exe
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

// Flag to keep track of running state
volatile sig_atomic_t g_running = 1;

// Signal handler for graceful exit on Ctrl + C
void signalHandler(int signum) {
    std::cout << "\n[!] Interrupt signal received. Closing gracefully...\n";
    g_running = 0;
}

int main() {
    // Register signal handler for Ctrl + C (SIGINT)
    signal(SIGINT, signalHandler);

    std::cout << "==========================================\n";
    std::cout << "  C++ Background App Running...\n";
    std::cout << "  Press [Ctrl + C] or close the window to exit.\n";
    std::cout << "==========================================\n";

    while (g_running) {
        // Put your background task logic here
        std::cout << "-> App is active and working...\n";
        
        // Sleep for 5 seconds total, checking the running flag every 1 second 
        // so it exits quickly when interrupted.
        for (int i = 0; i < 5 && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "App closed successfully.\n";
    return 0;
}