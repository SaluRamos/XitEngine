#include "utils.hpp"

bool Utils::IsRunAsAdmin() {
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

void Utils::RequestAdminPrivileges() {
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

std::vector<Utils::ProcessInfo> Utils::GetActiveProcesses() {
    std::vector<Utils::ProcessInfo> processes;
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

bool Utils::InjectDLL(DWORD processId, const char* dllPath) {
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