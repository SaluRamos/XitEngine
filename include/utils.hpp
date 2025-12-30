#ifndef XITENGINE_UTILS
#define XITENGINE_UTILS

#include <windows.h>
#include <vector>
#include <string>
#include <tlhelp32.h>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace Utils {

    struct ProcessInfo {
        DWORD pid;
        std::wstring name;
    };

    bool IsRunAsAdmin();
    void RequestAdminPrivileges();
    std::vector<Utils::ProcessInfo> GetActiveProcesses();
    bool InjectDLL(DWORD processId, const char* dllPath);

    //-------------------------------------------------------TIME-------------------------------------------------------

    int64_t getActualTimestampNanoSeconds();
    int64_t getActualTimestampSeconds();
    int64_t getActualTimestampMiliSeconds();
    double getTimeDifferenceInSeconds(int64_t start, int64_t end);
    int64_t nanoToSeconds(int64_t value);
    int64_t nanoToMilisecond(int64_t value);
    int64_t milisecondToSecond(int64_t value);
    int64_t secondToMilisecond(int64_t value);
    int64_t milisecondToNano(int64_t value);
    int64_t secondToNano(int64_t value);
    std::string timestampToDateString(int64_t timestamp);

};

#endif