#ifndef XITENGINE_UTILS
#define XITENGINE_UTILS

#include <windows.h>
#include <vector>
#include <string>
#include <tlhelp32.h>

class Utils {

    public:

        struct ProcessInfo {
            DWORD pid;
            std::wstring name;
        };

        static bool IsRunAsAdmin();
        static void RequestAdminPrivileges();
        static std::vector<Utils::ProcessInfo> GetActiveProcesses();
        static bool InjectDLL(DWORD processId, const char* dllPath);

};

#endif