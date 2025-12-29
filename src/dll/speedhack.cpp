#include <windows.h>
#include <MinHook.h>
#include <mutex>
#include <cmath>

// Variável de controle exportada
extern "C" __declspec(dllexport) float speedFactor = 2.0f;

// Locks de sincronização (equivalente ao TSimpleLock do Pascal)
std::mutex gtcLock;
std::mutex qpcLock;

// Protótipos das funções originais
typedef DWORD (WINAPI* GTC_T)(VOID);
typedef BOOL (WINAPI* QPC_T)(LARGE_INTEGER*);

GTC_T pGTCOriginal = nullptr;
QPC_T pQPCOriginal = nullptr;

// Variáveis de estado para o cálculo relativo
DWORD initialTimeGTC = 0;
DWORD initialOffsetGTC = 0;

LARGE_INTEGER initialTimeQPC = { 0 };
LARGE_INTEGER initialOffsetQPC = { 0 };

// --- Implementação GetTickCount (Fiel ao speedhackversion_GetTickCount) ---
DWORD WINAPI DetourGetTickCount() {
    std::lock_guard<std::mutex> lock(gtcLock);

    DWORD currentTime = pGTCOriginal();

    if (initialTimeGTC == 0) {
        initialTimeGTC = currentTime;
        initialOffsetGTC = currentTime;
    }

    // Lógica Pascal: trunc((currentTime - initialtime) * speedmultiplier) + initialoffset
    return (DWORD)((currentTime - initialTimeGTC) * speedFactor) + initialOffsetGTC;
}

// --- Implementação QPC (Fiel ao speedhackversion_QueryPerformanceCounter) ---
BOOL WINAPI DetourQPC(LARGE_INTEGER* lpPerformanceCount) {
    std::lock_guard<std::mutex> lock(qpcLock);

    LARGE_INTEGER currentTime;
    BOOL result = pQPCOriginal(&currentTime);

    if (initialTimeQPC.QuadPart == 0) {
        initialTimeQPC = currentTime;
        // Captura o que o jogo "acha" que é o tempo atual para manter a continuidade
        QueryPerformanceCounter(&initialOffsetQPC); 
    }

    // Lógica Pascal para QPC
    lpPerformanceCount->QuadPart = (LONGLONG)((currentTime.QuadPart - initialTimeQPC.QuadPart) * speedFactor) + initialOffsetQPC.QuadPart;

    return result;
}

// --- Inicialização da DLL ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        if (MH_Initialize() == MH_OK) {
            // Hook GetTickCount (Kernel32)
            MH_CreateHook(reinterpret_cast<LPVOID>(GetTickCount), 
                          reinterpret_cast<LPVOID>(DetourGetTickCount), 
                          reinterpret_cast<LPVOID*>(&pGTCOriginal));

            // Hook QueryPerformanceCounter (Kernel32)
            MH_CreateHook(reinterpret_cast<LPVOID>(QueryPerformanceCounter), 
                          reinterpret_cast<LPVOID>(DetourQPC), 
                          reinterpret_cast<LPVOID*>(&pQPCOriginal));

            MH_EnableHook(MH_ALL_HOOKS);
        }
    }
    return TRUE;
}