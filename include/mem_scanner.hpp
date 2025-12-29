#ifndef XITENGINE_MEMSCANNER
#define XITENGINE_MEMSCANNER

#include <iostream>
#include <windows.h>
#include <vector>
#include <tlhelp32.h>
#include <string>
#include <cstdint>
#include "enums.hpp"

class MemoryScanner {
    public:

        // process
        HANDLE hProcess = NULL;
        DWORD targetPID = 0;
        bool ConnectToProcess(const std::wstring& procName);
        
        // scan
        std::vector<LPVOID> foundAddresses;
        BOOL WriteIntMemory(LPVOID address, int32_t value);
        BOOL WriteFloatMemory(LPVOID address, float value);
        BOOL WriteDoubleMemory(LPVOID address, double value);
        BOOL WriteLongMemory(LPVOID address, int64_t value);
        BOOL WriteByteMemory(LPVOID address, uint8_t value);
        void ResetScan();
        void FirstScan(void* value, size_t size, bool onlyWritable, ScanFilterType scanType);
        void NextScan(void* value, size_t size, ScanFilterType scanType);
        bool CompareFloat(float val1, float val2, float epsilon = 0.01f);
        bool CompareDouble(double val1, double val2, double epsilon = 0.01f);
        
        // speed hack
        uintptr_t speedMultiplierOffset = 0;
        void ResetSpeedHack();
        bool UpdateRemoteSpeed(DWORD processId, float newSpeed);
        uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);

};

#endif