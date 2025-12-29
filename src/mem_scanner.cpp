#include "mem_scanner.hpp"

bool MemoryScanner::ConnectToProcess(const std::wstring& procName) {
    if (hProcess) {
        CloseHandle(hProcess);
        hProcess = NULL;
    }
    ResetScan();

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (procName == pe32.szExeFile) {
                targetPID = pe32.th32ProcessID;
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
                CloseHandle(hSnapshot);
                return hProcess != NULL;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return false;
}

BOOL MemoryScanner::WriteIntMemory(LPVOID address, int32_t value) {
    if (!hProcess) return false;
    SIZE_T written = 0;
    return WriteProcessMemory(hProcess, address, &value, sizeof(value), &written) && written == sizeof(value);
}

BOOL MemoryScanner::WriteFloatMemory(LPVOID address, float value) {
    if (!hProcess) return false;
    SIZE_T written = 0;
    return WriteProcessMemory(hProcess, address, &value, sizeof(value), &written) && written == sizeof(value);
}

BOOL MemoryScanner::WriteDoubleMemory(LPVOID address, double value) {
    if (!hProcess) return false;
    SIZE_T written = 0;
    return WriteProcessMemory(hProcess, address, &value, sizeof(value), &written) && written == sizeof(value);
}

BOOL MemoryScanner::WriteLongMemory(LPVOID address, int64_t value) {
    if (!hProcess) return false;
    SIZE_T written = 0;
    return WriteProcessMemory(hProcess, address, &value, sizeof(value), &written) && written == sizeof(value);
}

BOOL MemoryScanner::WriteByteMemory(LPVOID address, uint8_t value) {
    if (!hProcess) return false;
    SIZE_T written = 0;
    return WriteProcessMemory(hProcess, address, &value, sizeof(value), &written) && written == sizeof(value);
}

void MemoryScanner::ResetScan() {
    foundAddresses.clear();
    speedMultiplierOffset = 0;
}

void MemoryScanner::FirstScan(void* value, size_t size, bool onlyWritable) {
    // Limpa resultados anteriores antes de iniciar
    foundAddresses.clear();
    if (!hProcess) return;

    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* addr = nullptr;

    // Itera sobre as regiões de memória do processo
    while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi))) {
        // Filtro de Estado: Apenas memória commitada (em uso físico)
        if (mbi.State == MEM_COMMIT) {
            
            // Lógica de proteção baseada na escolha do usuário
            bool isWritable = (mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_EXECUTE_READWRITE);
            bool isReadable = (mbi.Protect & PAGE_READONLY) || (mbi.Protect & PAGE_READWRITE) || 
                            (mbi.Protect & PAGE_EXECUTE_READ) || (mbi.Protect & PAGE_EXECUTE_READWRITE);
            // Decisão: se o usuário quer 'apenas escrevível', filtramos. 
            // Caso contrário, verificamos se a página permite ao menos leitura.
            if ((onlyWritable && isWritable) || (!onlyWritable && isReadable)) {
                std::vector<unsigned char> buffer(mbi.RegionSize);
                SIZE_T bytesRead;
                // Lê o bloco inteiro de uma vez para performance
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                    // Varre o buffer local em busca do valor
                    // O loop vai até (bytesRead - size) para não ler fora do buffer

                    // PARA CASOS COM FLOAT/DOUBLE, USAR EPISILON
                    // if (selectedType == 1) { // Se for float
                    //     float target = *(float*)value;
                    //     for (size_t i = 0; i <= bytesRead - sizeof(float); i++) {
                    //         float current = *(float*)&buffer[i];
                    //         if (CompareFloat(current, target)) {
                    //             foundAddresses.push_back((LPVOID)((uintptr_t)mbi.BaseAddress + i));
                    //         }
                    //     }
                    // } else { // Mantém o memcmp para tipos inteiros
                    //     for (size_t i = 0; i <= bytesRead - size; i++) {
                    //         if (memcmp(&buffer[i], value, size) == 0) {
                    //             // Armazena o endereço real (Base + deslocamento do loop)
                    //             foundAddresses.push_back((LPVOID)((uintptr_t)mbi.BaseAddress + i));
                    //         }
                    //     }
                    // }
                    
                    for (size_t i = 0; i <= bytesRead - size; i++) {
                        if (memcmp(&buffer[i], value, size) == 0) {
                            // Armazena o endereço real (Base + deslocamento do loop)
                            foundAddresses.push_back((LPVOID)((uintptr_t)mbi.BaseAddress + i));
                        }
                    }


                }
            }
        }
        // Move o ponteiro para a próxima região de memória
        addr += mbi.RegionSize;
    }
}

void MemoryScanner::NextScan(void* value, size_t size) {
    if (!hProcess || foundAddresses.empty()) return;

    std::vector<LPVOID> newResults;
    for (LPVOID addr : foundAddresses) {
        std::vector<unsigned char> tempBuffer(size);
        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, addr, tempBuffer.data(), size, &bytesRead)) {
            if (memcmp(tempBuffer.data(), value, size) == 0) {
                newResults.push_back(addr);
            }
        }
    }
    foundAddresses = newResults;
}

uintptr_t MemoryScanner::GetModuleBaseAddress(DWORD procId, const wchar_t* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me32;
        me32.dwSize = sizeof(MODULEENTRY32W);
        if (Module32FirstW(hSnap, &me32)) {
            do {
                if (_wcsicmp(me32.szModule, modName) == 0) {
                    modBaseAddr = (uintptr_t)me32.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &me32));
        }
        CloseHandle(hSnap);
    }
    return modBaseAddr;
}

bool MemoryScanner::CompareFloat(float val1, float val2, float epsilon) {
    return std::abs(val1 - val2) < epsilon;
}

bool MemoryScanner::CompareDouble(double val1, double val2, double epsilon) {
    return std::abs(val1 - val2) < epsilon;
}

bool MemoryScanner::UpdateRemoteSpeed(DWORD processId, float newSpeed) {
    uintptr_t dllBase = GetModuleBaseAddress(processId, L"speedhack.dll");
    if (dllBase == 0) {
        std::cout << "Erro: DLL nao encontrada no jogo!\n";
        return false;
    }

    if (speedMultiplierOffset == 0) {
        HMODULE hLocalDll = LoadLibraryA("speedhack.dll");
        if (hLocalDll) {
            FARPROC remoteVarAddr = GetProcAddress(hLocalDll, "speedFactor");
            if (remoteVarAddr) {
                speedMultiplierOffset = (uintptr_t)remoteVarAddr - (uintptr_t)hLocalDll;
            }
            FreeLibrary(hLocalDll);
        }
    }

    if (speedMultiplierOffset == 0) return false;

    uintptr_t finalAddr = dllBase + speedMultiplierOffset;
    std::cout << "Escrevendo em: " << std::hex << finalAddr << "\n";

    if (hProcess) {
        return WriteProcessMemory(hProcess, (LPVOID)finalAddr, &newSpeed, sizeof(float), NULL);
    }
    return false;
}