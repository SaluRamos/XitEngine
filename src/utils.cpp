#include "utils.hpp"

namespace Utils {

    bool IsRunAsAdmin() {
        BOOL fRet = FALSE;
        HANDLE hToken = NULL;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD dwSize;
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
                fRet = elevation.TokenIsElevated;
            }
        }
        if (hToken) CloseHandle(hToken);
        return fRet;
    }
    
    void RequestAdminPrivileges() {
        if (!IsRunAsAdmin()) {
            wchar_t szPath[MAX_PATH];
            GetModuleFileNameW(NULL, szPath, MAX_PATH);
    
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;
    
            if (ShellExecuteExW(&sei)) {
                exit(0); // Fecha a instância atual sem privilégios
            }
        }
    }
    
    std::vector<ProcessInfo> GetActiveProcesses() {
        std::vector<ProcessInfo> processes;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return processes;
    
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
    
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                processes.push_back({ pe32.th32ProcessID, pe32.szExeFile });
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
        return processes;
    }
    
    bool InjectDLL(DWORD processId, const char* dllPath) {
        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProc) return false;
    
        LPVOID pRemotePath = VirtualAllocEx(hProc, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
        WriteProcessMemory(hProc, pRemotePath, dllPath, strlen(dllPath) + 1, NULL);
    
        HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, 
            (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"), 
            pRemotePath, 0, NULL);
    
        if (hThread) {
            WaitForSingleObject(hThread, 5000); 
            CloseHandle(hThread);
        }
    
        VirtualFreeEx(hProc, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return true;
    }
    
    //---------------------------------------------------- TIME --------------------------------------------------------------
        
    int64_t getActualTimestampNanoSeconds() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        return nanoseconds;
    }
    
    int64_t getActualTimestampSeconds() {
        return nanoToSeconds(getActualTimestampNanoSeconds());
    }
    
    int64_t getActualTimestampMiliSeconds() {
        return nanoToMilisecond(getActualTimestampNanoSeconds());
    }
    
    double getTimeDifferenceInSeconds(int64_t start, int64_t end) {
        int64_t dif = end - start;
        return nanoToSeconds(dif);
    }
    
    std::string timestampToDateString(int64_t timestamp) {
        //1672587123 em segundos (10 números)
        //1672587123456 em milisegundos (13 números)
        //1672587123456789012 em nanosegundos (19 números)
        int timestampSize = std::to_string(timestamp).size();
        if (timestampSize == 13) {
            timestamp = milisecondToSecond(timestamp);
        } else if (timestampSize == 19) {
            timestamp = nanoToSeconds(timestamp);
        } else if (timestampSize != 10) {
            throw std::runtime_error("Unknown timestamp time unit");
        }
        std::chrono::seconds seconds(timestamp);
        auto time_point = std::chrono::system_clock::time_point(seconds);
        std::time_t time_t_format = std::chrono::system_clock::to_time_t(time_point);
        std::tm tm_local{};
        #ifdef _WIN32
            localtime_s(&tm_local, &time_t_format);
        #else
            // POSIX systems
            localtime_r(&time_t_format, &tm_local);
        #endif
        std::ostringstream oss;
        oss << std::put_time(&tm_local, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
    
    int64_t nanoToSeconds(int64_t value) {
        return value/1'000'000'000.0;
    }
    
    int64_t nanoToMilisecond(int64_t value) {
        return value/1'000'000.0;
    }
    
    int64_t milisecondToSecond(int64_t value) {
        return value/1'000;
    }
    
    int64_t secondToMilisecond(int64_t value) {
        return value*1'000;
    }
    
    int64_t milisecondToNano(int64_t value) {
        return value*1'000'000.0;
    }
    
    int64_t secondToNano(int64_t value) {
        return value*1'000'000'000.0;
    }

}
