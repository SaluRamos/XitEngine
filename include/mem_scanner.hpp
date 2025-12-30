#ifndef XITENGINE_MEMSCANNER
#define XITENGINE_MEMSCANNER

#include <iostream>
#include <windows.h>
#include <vector>
#include <tlhelp32.h>
#include <string>
#include <cstdint>
#include <cstring>
#include "enums.hpp"

class MemoryScanner {

    public:

        class AddressInfo {

            public:

                AddressInfo(uintptr_t addr, uint64_t cur, uint64_t prev, uint64_t first, MemoryType type) :
                    address(addr),
                    current(cur),
                    previous(prev),
                    first(first),
                    type(type)
                {}

                uintptr_t address;
                uint64_t current; //raw
                uint64_t previous; //raw
                uint64_t first; //raw
                MemoryType type;
                //std::memcpy(&raw, &f, sizeof(float)); //para escrita
                // float out;
                // std::memcpy(&out, &raw, sizeof(float)); //para leitura

                LPVOID getLPAddress() const {
                    return reinterpret_cast<LPVOID>(address);
                }

                template<typename T>
                static T readRaw(uint64_t raw) {
                    T out{};
                    std::memcpy(&out, &raw, sizeof(T));
                    return out;
                }

                void formatValue(char* buf, size_t sz, uint64_t raw) const {
                    switch (type) {
                    case MEM_FLOAT:
                        snprintf(buf, sz, "%.3f", readRaw<float>(raw));
                        break;

                    case MEM_DOUBLE:
                        snprintf(buf, sz, "%.6f", readRaw<double>(raw));
                        break;

                    case MEM_INT:
                        snprintf(buf, sz, "%d", readRaw<int32_t>(raw));
                        break;

                    case MEM_LONG:
                        snprintf(buf, sz, "%lld", readRaw<int64_t>(raw));
                        break;

                    case MEM_BYTE:
                        snprintf(buf, sz, "%u", readRaw<uint8_t>(raw));
                        break;
                }
                }

        };

        class SavedAddress {

            public:
                std::string description; // set by user
                AddressInfo address;

        };

        // process
        HANDLE hProcess = NULL;
        DWORD targetPID = 0;
        bool ConnectToProcess(const std::wstring& procName);
        
        // scan
        std::vector<AddressInfo> foundAddresses;
        std::vector<SavedAddress> savedAddresses;
        BOOL WriteIntMemory(LPVOID address, int32_t value);
        BOOL WriteFloatMemory(LPVOID address, float value);
        BOOL WriteDoubleMemory(LPVOID address, double value);
        BOOL WriteLongMemory(LPVOID address, int64_t value);
        BOOL WriteByteMemory(LPVOID address, uint8_t value);
        void ResetScan();
        void FirstScan(void* value, bool onlyWritable, MemoryType memType, ScanFilterType scanType);
        void NextScan(void* value, MemoryType memType, ScanFilterType scanType);
        bool CompareFloat(float val1, float val2, float epsilon = 0.01f);
        bool CompareDouble(double val1, double val2, double epsilon = 0.01f);
        
        // speed hack
        uintptr_t speedMultiplierOffset = 0;
        void ResetSpeedHack();
        bool UpdateRemoteSpeed(DWORD processId, float newSpeed);
        uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);

};

#endif