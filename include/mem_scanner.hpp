#ifndef XITENGINE_MEMSCANNER
#define XITENGINE_MEMSCAN

#include <iostream>
#include <windows.h>
#include <vector>
#include <tlhelp32.h>
#include <string>

class MemoryScanner {
    public:

        HANDLE hProcess = NULL;
        std::vector<LPVOID> foundAddresses;
        DWORD targetPID = 0;
        uintptr_t speedMultiplierOffset = 0;

        bool ConnectToProcess(const std::wstring& procName);

        bool WriteMemory(LPVOID address, int value);

        void FirstScan(void* value, size_t size, bool onlyWritable);

        void NextScan(void* value, size_t size);

        uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);

        bool CompareFloat(float val1, float val2, float epsilon = 0.01f);

        bool CompareDouble(double val1, double val2, double epsilon = 0.01f);

        bool UpdateRemoteSpeed(DWORD processId, float newSpeed);

};

#endif