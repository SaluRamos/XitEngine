// natives Includes
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// ImGui Includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

// xit engine Includes
#include "utils.hpp"
#include "enums.hpp"
#include "mem_scanner.hpp"

static bool showProcessSelector = false;
static std::vector<Utils::ProcessInfo> processList;
static int selectedProcessIndex = -1;
static MemoryScanner scanner;
static char procName[128] = "RobloxPlayerBeta.exe";
static bool onlyWritable = true;
static int valueToFindInt = 0;
static float valueToFindFloat = 0;
static double valueToFindDouble = 0;
static int selectedType = 0;
const char* types[] = { "int", "float", "double", "long" };
static bool isConnectedToProcess = false;
static bool errorWhileConnecting = false;
static int writeValue = 0;
static char procFilter[128] = ""; // search filter
static bool speedHackActivated = false;
static float speedFactor = 5.0f;
static bool dllInjected = false;


void DrawProcessSelectorSection() {
    ImGui::Text("--- Process Selector ---");
    if (ImGui::Button("Abrir Lista de Processos", ImVec2(-1, 0))) {
        processList = Utils::GetActiveProcesses();
        showProcessSelector = true;
        ImGui::OpenPopup("ProcessListPopup");
    }

    if (ImGui::BeginPopupModal("ProcessListPopup", &showProcessSelector, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Buscar Processo:");
        ImGui::InputText("##filter", procFilter, IM_ARRAYSIZE(procFilter));
        ImGui::Separator();
        if (ImGui::BeginChild("ProcListChild", ImVec2(400, 300), true)) {
            std::string filterStr(procFilter);
            // convert filter string to lower for case-insensitive search
            std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);
            for (int n = 0; n < processList.size(); n++) {
                std::wstring ws = processList[n].name;
                std::string sName(ws.begin(), ws.end());
                if (!filterStr.empty()) {
                    std::string sNameLower = sName;
                    std::transform(sNameLower.begin(), sNameLower.end(), sNameLower.begin(), ::tolower);
                    if (sNameLower.find(filterStr) == std::string::npos) { //process name doesnt match
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
        if (ImGui::Button("Abrir", ImVec2(120, 0)) && selectedProcessIndex != -1) {
            isConnectedToProcess = scanner.ConnectToProcess(processList[selectedProcessIndex].name);
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
    // process manual input
    ImGui::Text("Nome do Processo:");
    ImGui::InputText("##proc", procName, IM_ARRAYSIZE(procName));
    if (ImGui::Button("Conectar", ImVec2(-1, 0))) {
            std::string s(procName);
            std::wstring ws(s.begin(), s.end());
            isConnectedToProcess = scanner.ConnectToProcess(ws);
            if (isConnectedToProcess) {
                std::cout << "[LOG] Conectado com sucesso ao PID: " << scanner.targetPID << "\n";
            } else {
                errorWhileConnecting = true;
                std::cout << "[LOG] Falha ao conectar. Verifique se o processo esta aberto e voce e Admin.\n";
            }
    }
    // Process connection status
    if (isConnectedToProcess) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: CONECTADO (PID: %lu)", scanner.targetPID);
    } else if (errorWhileConnecting) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: FALHA AO CONECTAR, Verifique se o processo esta aberto e voce e Admin.");
    } else {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Status: DESCONECTADO");
    }
}

void DrawMemScannerSection() {
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
    // Write to mem
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
}

void DrawSpeedHackSection() {
    ImGui::Text("--- Speed Hack (DLL INJECTION) ---");
    if (ImGui::Button("Ativar SpeedHack")) {
        speedHackActivated = true;
    }
    if (speedHackActivated) {
        if (!dllInjected) {
            wchar_t currentPath[MAX_PATH];
            GetModuleFileNameW(NULL, currentPath, MAX_PATH);
            std::wstring fullPath(currentPath);
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            std::wstring dllPath = fullPath.substr(0, lastSlash + 1) + L"speedhack.dll";
            std::string finalPath(dllPath.begin(), dllPath.end());
            std::cout << "speed hack dll is: " << finalPath << "\n";
            if (Utils::InjectDLL(scanner.targetPID, finalPath.c_str())) {
                dllInjected = true;
            }
        } else {
            if (ImGui::SliderFloat("Fator de Velocidade", &speedFactor, 0.1f, 10.0f, "%.1fx")) {
                if (!scanner.UpdateRemoteSpeed(scanner.targetPID, speedFactor)) {
                    dllInjected = false;
                }
            }
            if (ImGui::Button("Resetar")) {
                speedFactor = 1.0f;
                scanner.UpdateRemoteSpeed(scanner.targetPID, speedFactor);
            }
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "DLL Ativa! Ajuste o slider para mudar o tempo.");
        }
    }
}

int main() {
    Utils::RequestAdminPrivileges();

    // Setup GLFW
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(600, 500, "XitEngine", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw Interface
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoDecoration);

        DrawProcessSelectorSection();

        ImGui::Separator();

        if (!isConnectedToProcess) ImGui::BeginDisabled();

        DrawMemScannerSection();

        ImGui::Separator();

        DrawSpeedHackSection();

        if (!isConnectedToProcess) ImGui::EndDisabled();

        ImGui::End();

        // Renderization
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