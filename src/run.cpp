// clang++ -std=c++17 run.cpp -o run.exe -mconsole

#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>
#include <windows.h>

namespace fs = std::filesystem;

int main() {
    const fs::path currentDir = fs::current_path();

    // ------------------------------------------------------------
    // 1. Get target path
    // ------------------------------------------------------------
    std::cout << "Get paths from r\\DiscordQuests: \nhttps://www.reddit.com/r/DiscordQuests/wiki/game-index/\n";
    std::cout << "Enter the relative path (e.g., test\\folder\\myApp.exe or ..\\test\\folder\\myApp.exe): ";

    std::string relPathStr;
    std::getline(std::cin, relPathStr);

    // 1. Trim leading and trailing spaces/whitespace
    const auto strBegin = relPathStr.find_first_not_of(" \t\n\r");
    if (strBegin != std::string::npos) {
        const auto strEnd = relPathStr.find_last_not_of(" \t\n\r");
        relPathStr = relPathStr.substr(strBegin, strEnd - strBegin + 1);
    } else {
        relPathStr.clear();
    }

    if (relPathStr.empty()) {
        std::cerr << "Error: path cannot be empty.\n";
        return 1;
    }

    // 2. Strip any leading slashes, backslashes, or dots
    while (!relPathStr.empty() && (relPathStr[0] == '\\' || relPathStr[0] == '/' || relPathStr[0] == '.')) {
        relPathStr.erase(relPathStr.begin());
    }

    // 3. Trim any leftover spaces after removing leading symbols (e.g., "\  file.exe")
    const auto strBegin2 = relPathStr.find_first_not_of(" \t\n\r");
    if (strBegin2 != std::string::npos) {
        relPathStr = relPathStr.substr(strBegin2);
    } else {
        relPathStr.clear();
    }

    if (relPathStr.empty()) {
        std::cerr << "Error: invalid path.\n";
        return 1;
    }

    fs::path relPath(relPathStr);

    if (relPath.is_absolute()) {
        std::cerr << "Error: please enter a relative path.\n";
        return 1;
    }

    const fs::path targetFullPath = currentDir / "current" / relPath;
    const fs::path targetDir = targetFullPath.parent_path();

    // ------------------------------------------------------------
    // 2. Determine what directory we are allowed to clean up
    // ------------------------------------------------------------
    fs::path topLevelDir;
    auto it = relPath.begin();
    if (it != relPath.end()) {
        topLevelDir = currentDir / *it;
    }

    const bool topLevelExistedBefore =
        !topLevelDir.empty() && fs::exists(topLevelDir);

    // ------------------------------------------------------------
    // 3. Create target directories
    // ------------------------------------------------------------
    std::cout << "[*] Creating directories...\n";

    try {
        if (!targetDir.empty()) {
            fs::create_directories(targetDir);
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating directories: " << e.what() << "\n";
        return 1;
    }

    // ------------------------------------------------------------
    // 4. Locate dummy.exe
    // ------------------------------------------------------------
    const fs::path sourceDummy = currentDir / "dummy.exe";

    if (!fs::exists(sourceDummy)) {
        std::cerr << "Error: 'dummy.exe' not found in the current directory.\n";
        if (!topLevelExistedBefore && !topLevelDir.empty() && fs::exists(topLevelDir)) {
            std::error_code ec;
            fs::remove_all(topLevelDir, ec);
        }
        return 1;
    }

    // ------------------------------------------------------------
    // 5. Copy dummy.exe to target
    // ------------------------------------------------------------
    std::cout << "[*] Copying and renaming dummy.exe...\n";

    try {
        fs::copy_file(
            sourceDummy,
            targetFullPath,
            fs::copy_options::overwrite_existing
        );
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error copying file: " << e.what() << "\n";
        if (!topLevelExistedBefore && !topLevelDir.empty() && fs::exists(topLevelDir)) {
            std::error_code ec;
            fs::remove_all(topLevelDir, ec);
        }
        return 1;
    }

    // ------------------------------------------------------------
    // 6. Get runtime duration
    // ------------------------------------------------------------

    std::cout << "Enter runtime duration in minutes (default 15): ";

    std::string inputLine;
    int mins = 15; // Set your default here

    if (std::getline(std::cin, inputLine) && !inputLine.empty()) {
        std::stringstream ss(inputLine);
        if (!(ss >> mins) || mins <= 0) {
            std::cerr << "Error: duration must be a positive integer.\n";
            std::error_code ec;
            fs::remove(targetFullPath, ec);
            if (!topLevelExistedBefore && !topLevelDir.empty() && fs::exists(topLevelDir)) {
                fs::remove_all(topLevelDir, ec);
            }
            return 1;
        }
    }

    std::cout << "[*] Launching application... "
              << "Running for " << mins << " minute(s).\n"
              << "[*] NOTE: 'run.exe' will stay active in Task Manager to manage the session.\n";

    // ------------------------------------------------------------
    // 7. Launch target process using ShellExecuteExW (Emulates Shell/PowerShell execution)
    // ------------------------------------------------------------

    std::wstring dirStr = targetDir.wstring();       // Store string persistently
    std::wstring fileStr = targetFullPath.wstring(); // Store file path persistently

    SHELLEXECUTEINFOW pi = { sizeof(pi) };
    pi.fMask = SEE_MASK_NOCLOSEPROCESS;
    pi.lpFile = fileStr.c_str();
    pi.lpDirectory = dirStr.c_str(); // Run in its own folder
    pi.nShow = SW_NORMAL;

    if (!ShellExecuteExW(&pi)) {
        const DWORD error = GetLastError();
        std::cerr << "Failed to launch process via ShellExecute. Windows error code: " << error << "\n";
        
        // Cleanup on failure
        std::error_code ec;
        fs::remove(targetFullPath, ec);
        if (!topLevelExistedBefore && !topLevelDir.empty() && fs::exists(topLevelDir)) {
            fs::remove_all(topLevelDir, ec);
        }
        return 1;
    }

    // Capture the process handle from the shell execution structure
    HANDLE hProcess = pi.hProcess;

    // ------------------------------------------------------------
    // Bind target lifetime to run.exe (Kills target if run.exe dies)
    // ------------------------------------------------------------
    HANDLE hJob = CreateJobObjectW(nullptr, nullptr);
    if (hJob) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
        AssignProcessToJobObject(hJob, hProcess);
    }

    // ------------------------------------------------------------
    // 8. Active Manager Loop (Shows live progress in MM:SS format)
    // ------------------------------------------------------------
    auto startTick = std::chrono::steady_clock::now();
    auto maxDuration = std::chrono::minutes(mins) + std::chrono::seconds(15); // Safety 15 secs for guaranteed finished
    auto totalSecs = std::chrono::duration_cast<std::chrono::seconds>(maxDuration).count();

    // Helper lambda to format seconds into MM:SS
    auto formatTime = [](long long secs) {
        long long m = secs / 60;
        long long s = secs % 60;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02lld:%02lld", m, s);
        return std::string(buf);
    };

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - startTick;
        
        // Check if user requested time has elapsed
        if (elapsed >= maxDuration) {
            std::cout << "\n"; // Move to a new line when finished
            break;
        }

        // Check if the target process crashed or closed prematurely on its own
        DWORD exitCode = 0;
        if (GetExitCodeProcess(hProcess, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                std::cout << "\n[*] Target application exited early on its own.\n";
                break;
            }
        }

        // Calculate time breakdown
        auto elapsedSecs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        long long remainingSecs = totalSecs - elapsedSecs;
        if (remainingSecs < 0) remainingSecs = 0;

        // Print live updating progress line
        std::cout << "\r[*] Running... Elapsed: " << formatTime(elapsedSecs) 
                  << " / " << formatTime(totalSecs) 
                  << " (Remaining: " << formatTime(remainingSecs) << ") " << std::flush;

        // Sleep briefly in chunks so the runner stays responsive
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // ------------------------------------------------------------
    // 9. Shutdown & Cleanup phase
    // ------------------------------------------------------------
    DWORD exitCode = 0;
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        if (exitCode == STILL_ACTIVE) {
            std::cout << "[*] Time's up. Stopping the managed process...\n";
            if (!TerminateProcess(pi.hProcess, 0)) {
                std::cerr << "Warning: failed to terminate process. Error code: " << GetLastError() << "\n";
            } else {
                WaitForSingleObject(pi.hProcess, 2000);
            }
        }
    }

    CloseHandle(pi.hProcess);

    std::cout << "[*] Cleaning up created files...\n";
    try {
        if (fs::exists(targetFullPath)) {
            fs::remove(targetFullPath);
        }
        if (!topLevelExistedBefore && !topLevelDir.empty() && fs::exists(topLevelDir)) {
            fs::remove_all(topLevelDir);
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error during cleanup: " << e.what() << "\n";
    }

    std::cout << "[+] Cleanup complete. Done!\n";
    return 0;
}