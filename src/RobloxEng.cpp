#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

//// ImGui Includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <tlhelp32.h>
#include <shellapi.h>

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

struct ProcessInfo {
    DWORD pid;
    std::wstring name;
};

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

static bool showProcessSelector = false;
static std::vector<ProcessInfo> processList;
static int selectedProcessIndex = -1;

enum MemoryType {
    MEM_INT,
    MEM_FLOAT,
    MEM_DOUBLE,
    MEM_LONG,
    MEM_BYTE,
    MEM_TWO_BYTES,
    MEM_FOUR_BYTES,
    MEM_EIGHT_BYTES
};

// Lógica de Memória (Simplificada para o exemplo)
class MemoryScanner {
    public:
        HANDLE hProcess = NULL;
        std::vector<LPVOID> foundAddresses;
        DWORD targetPID = 0;

        // Conecta ao processo alvo buscando pelo nome
        bool Connect(const std::wstring& procName) {
            if (hProcess) {
                CloseHandle(hProcess);
                hProcess = NULL;
            }
            foundAddresses.clear();

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

        bool WriteMemory(LPVOID address, int value) {
            if (!hProcess) return false;
            return WriteProcessMemory(hProcess, address, &value, sizeof(int), NULL);
        }

        void FirstScan(void* value, size_t size, bool onlyWritable) {
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

        void NextScan(void* value, size_t size) {
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

        bool CompareFloat(float val1, float val2, float epsilon = 0.01f) {
            return std::abs(val1 - val2) < epsilon;
        }

        bool CompareDouble(double val1, double val2, double epsilon = 0.01f) {
            return std::abs(val1 - val2) < epsilon;
        }

};

MemoryScanner scanner;

int main() {
    RequestAdminPrivileges();

    // 1. Setup GLFW
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(500, 500, "MemChanger", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    // 2. Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Variáveis da UI
    static char procName[128] = "RobloxPlayerBeta.exe";
    static bool onlyWritable = true;
    static int valueToFindInt = 0;
    static float valueToFindFloat = 0;
    static double valueToFindDouble = 0;
    static int selectedType = 0;
    const char* types[] = { "int", "float", "double", "long" };
    static bool isConnected = false;
    static bool errorWhileConnecting = false;
    static int writeValue = 0;
    static char procFilter[128] = ""; // Filtro de busca

    //// Loop Principal
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Desenho da Interface ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Scanner Principal", nullptr, ImGuiWindowFlags_NoDecoration);

        //Seletor de janelas
        if (ImGui::Button("Abrir Lista de Processos", ImVec2(-1, 0))) {
            processList = GetActiveProcesses();
            showProcessSelector = true;
            ImGui::OpenPopup("ProcessListPopup");
        }

        if (ImGui::BeginPopupModal("ProcessListPopup", &showProcessSelector, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Buscar Processo:");
            
            // Campo de entrada para o filtro
            ImGui::InputText("##filter", procFilter, IM_ARRAYSIZE(procFilter));
            ImGui::Separator();

            if (ImGui::BeginChild("ProcListChild", ImVec2(400, 300), true)) {
                std::string filterStr(procFilter);
                // Converter filtro para minúsculo para busca case-insensitive
                std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

                for (int n = 0; n < processList.size(); n++) {
                    // Preparar o nome do processo para exibição e busca
                    std::wstring ws = processList[n].name;
                    std::string sName(ws.begin(), ws.end());
                    
                    // Lógica de Filtro
                    if (!filterStr.empty()) {
                        std::string sNameLower = sName;
                        std::transform(sNameLower.begin(), sNameLower.end(), sNameLower.begin(), ::tolower);
                        
                        // Se o filtro não estiver no nome, pula este processo
                        if (sNameLower.find(filterStr) == std::string::npos) {
                            continue;
                        }
                    }

                    std::string label = std::to_string(processList[n].pid) + " - " + sName;

                    if (ImGui::Selectable(label.c_str(), selectedProcessIndex == n)) {
                        selectedProcessIndex = n;
                    }
                }
                ImGui::EndChild();
            }

            ImGui::Separator();

            // Botões de ação
            if (ImGui::Button("Abrir", ImVec2(120, 0)) && selectedProcessIndex != -1) {
                isConnected = scanner.Connect(processList[selectedProcessIndex].name);
                
                // Sincroniza o campo de texto principal
                std::wstring ws = processList[selectedProcessIndex].name;
                std::string sName(ws.begin(), ws.end());
                strcpy_s(procName, sName.c_str());
                
                showProcessSelector = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                showProcessSelector = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        //input manual da janela

        ImGui::Text("Nome do Processo:");
        ImGui::InputText("##proc", procName, IM_ARRAYSIZE(procName));

        if (ImGui::Button("Conectar", ImVec2(-1, 0))) {
                std::string s(procName);
                std::wstring ws(s.begin(), s.end());
                isConnected = scanner.Connect(ws);
                if (isConnected) {
                    std::cout << "[LOG] Conectado com sucesso ao PID: " << scanner.targetPID << std::endl;
                } else {
                    errorWhileConnecting = true;
                    std::cout << "[LOG] Falha ao conectar. Verifique se o processo esta aberto e voce e Admin." << std::endl;
                }
        }

        // Exibição do Status de Conexão
            if (isConnected) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: CONECTADO (PID: %lu)", scanner.targetPID);
            } else if (errorWhileConnecting) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: FALHA AO CONECTAR, Verifique se o processo esta aberto e voce e Admin.");
            } else {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Status: DESCONECTADO");
            }

        ImGui::Separator();

        if (!isConnected) ImGui::BeginDisabled();

        ImGui::Text("--- Scanner ---");

        ImGui::Combo("Tipo", &selectedType, types, IM_ARRAYSIZE(types));

        ImGui::Checkbox("Somente Memoria Escrevivel (Writable)", &onlyWritable);

        if (selectedType == 0) // int
            ImGui::InputScalar("##val", ImGuiDataType_S32, &valueToFindInt);
        else if (selectedType == 1) // float
            ImGui::InputScalar("##val", ImGuiDataType_Float, &valueToFindFloat);
        else if (selectedType == 2) // double
            ImGui::InputScalar("##val", ImGuiDataType_Double, &valueToFindDouble);

        
        void* currentValPtr = nullptr;
        size_t currentSize = 0;

        if (selectedType == 0) { currentValPtr = &valueToFindInt; currentSize = sizeof(int); }
        else if (selectedType == 1) { currentValPtr = &valueToFindFloat; currentSize = sizeof(float); }
        else if (selectedType == 2) { currentValPtr = &valueToFindDouble; currentSize = sizeof(double); }


       if (ImGui::Button("First Scan", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0))) {
           scanner.FirstScan(currentValPtr, currentSize, onlyWritable);
       }
       ImGui::SameLine();
       if (ImGui::Button("Next Scan", ImVec2(-1, 0))) {
           scanner.NextScan(currentValPtr, currentSize);
       }
       ImGui::TextColored(ImVec4(1, 1, 0, 1), "Encontrados: %zu", scanner.foundAddresses.size());

        // --- Seção de Edição (Write) ---
        if (!scanner.foundAddresses.empty()) {
            ImGui::Separator();
            ImGui::Text("--- Alterar Valor ---");
            ImGui::InputInt("Novo Valor", &writeValue);
            if (ImGui::Button("Escrever em TODOS", ImVec2(-1, 0))) {
                for (LPVOID addr : scanner.foundAddresses) {
                    scanner.WriteMemory(addr, writeValue);
                }
            }
        }

        if (!isConnected) ImGui::EndDisabled();

        ImGui::End();

        // Renderização
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}