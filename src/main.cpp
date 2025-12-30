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
static char procName[128] = "RobloxPlayerBeta.exe";
static char procFilter[128] = ""; // search filter
static bool isConnectedToProcess = false;
static bool errorWhileConnecting = false;

// Scanner
static MemoryScanner scanner;
static bool onlyWritable = true;
const char* const* memTypes = GetMemoryTypeNameList();
static bool isFirstScan = true;

const char* const* firstScanFilterTypes = GetScanFilterTypeNameListInFirstScan();
const char* const* nextScanFilterTypes = GetScanFilterTypeNameListInNextScan();

static int selectedScanMemType = 0; // 0 = int
static int selectedScanFilter = 0; // 0 = exact value

static int32_t valueToFindInt = 0;
static float valueToFindFloat = 0.0f;
static double valueToFindDouble = 0.0;
static int64_t valueToFindLong = 0;
static uint8_t valueToFindByte = 0;

static int32_t valueToReplaceInt = 0;
static float valueToReplaceFloat = 0.0f;
static double valueToReplaceDouble = 0.0;
static int64_t valueToReplaceLong = 0;
static uint8_t valueToReplaceByte = 0;

static int selectedAddresIndex = -1;

// SpeedHack
static bool speedHackActivated = false;
static float speedFactor = 5.0f;
static bool speedDLLInjected = false;

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
    ImGui::SameLine();
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
    ImGui::Combo("Tipo", &selectedScanMemType, memTypes, MEM_COUNT);

    if (isFirstScan) {
        ImGui::Combo("Filtro", &selectedScanFilter, firstScanFilterTypes, allowedInFirstScanCount);
    } else {
        ImGui::Combo("Filtro", &selectedScanFilter, nextScanFilterTypes, SCAN_FILTER_COUNT);
    }
    ImGui::Checkbox("Only Writable Memory", &onlyWritable);
    ImGui::Text("Valor:");
    ImGui::SameLine();
    MemoryType selectedMemType = ToMemoryType(selectedScanMemType);
    void* currentValPtr = nullptr;
    if (selectedMemType == MEM_INT) {
        ImGui::InputScalar("##val", ImGuiDataType_S32, &valueToFindInt);
        currentValPtr = &valueToFindInt;
    }
    else if (selectedMemType == MEM_FLOAT) {
        ImGui::InputScalar("##val", ImGuiDataType_Float, &valueToFindFloat);
        currentValPtr = &valueToFindFloat;
    }
    else if (selectedMemType == MEM_DOUBLE) {
        ImGui::InputScalar("##val", ImGuiDataType_Double, &valueToFindDouble);
        currentValPtr = &valueToFindDouble;
    }
    else if (selectedMemType == MEM_LONG) {
        ImGui::InputScalar("##val", ImGuiDataType_S64, &valueToFindLong);
        currentValPtr = &valueToFindLong;
    }
    else if (selectedMemType == MEM_BYTE) {
        ImGui::InputScalar("##val", ImGuiDataType_U8, &valueToFindByte);
        currentValPtr = &valueToFindByte;
    }

    if (isFirstScan) {
        if (ImGui::Button("First Scan", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0))) {
            scanner.FirstScan(currentValPtr, onlyWritable, selectedMemType, ToScanFilterType(selectedScanFilter));
            isFirstScan = false;
        }
    } else {
        if (ImGui::Button("Reset Scan", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0))) {
            scanner.ResetScan();
            isFirstScan = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Next Scan", ImVec2(-1, 0))) {
            scanner.NextScan(currentValPtr, selectedMemType, ToScanFilterType(selectedScanFilter));
        }
    }
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Encontrados: %zu", scanner.foundAddresses.size());

    const static float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    const static float clipperHeight = itemHeight * 10.0f;
    ImGui::BeginChild("AddressesList", ImVec2(0, clipperHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGuiListClipper clipper;
    clipper.Begin(scanner.foundAddresses.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const MemoryScanner::AddressInfo& a = scanner.foundAddresses[i];
            char cur[32], prev[32], first[32];
            a.formatValue(cur,   sizeof(cur),   a.current);
            a.formatValue(prev,  sizeof(prev),  a.previous);
            a.formatValue(first, sizeof(first), a.first);
            char line[160];
            snprintf(line, sizeof(line),
                    "0x%p | cur:%s | prev:%s | first:%s",
                    (void*)a.address, cur, prev, first);
            if (ImGui::Selectable(line, selectedAddresIndex == i)) {
                selectedAddresIndex = i;
            }
        }
    }
    ImGui::EndChild();

    // Write to mem
    if (!scanner.foundAddresses.empty()) {
        ImGui::Text("Novo Valor:");
        ImGui::SameLine();
        if (selectedMemType == MEM_INT) {
            ImGui::InputScalar("##newValue", ImGuiDataType_S32, &valueToReplaceInt);
            if (ImGui::Button("Escrever em TODOS", ImVec2(-1, 0))) {
                for (MemoryScanner::AddressInfo addr : scanner.foundAddresses) {
                    scanner.WriteIntMemory(addr.getLPAddress(), valueToReplaceInt);
                }
            }
        }
        else if (selectedMemType == MEM_FLOAT) {
            ImGui::InputScalar("##newValue", ImGuiDataType_Float, &valueToReplaceFloat);
            if (ImGui::Button("Escrever em TODOS", ImVec2(-1, 0))) {
                for (MemoryScanner::AddressInfo addr : scanner.foundAddresses) {
                    scanner.WriteFloatMemory(addr.getLPAddress(), valueToReplaceFloat);
                }
            }
        }
        else if (selectedMemType == MEM_DOUBLE) {
            ImGui::InputScalar("##newValue", ImGuiDataType_Double, &valueToReplaceDouble);
            if (ImGui::Button("Escrever em TODOS", ImVec2(-1, 0))) {
                for (MemoryScanner::AddressInfo addr : scanner.foundAddresses) {
                    scanner.WriteDoubleMemory(addr.getLPAddress(), valueToReplaceDouble);
                }
            }
        }
        else if (selectedMemType == MEM_LONG) {
            ImGui::InputScalar("##newValue", ImGuiDataType_S64, &valueToReplaceLong);
            if (ImGui::Button("Escrever em TODOS", ImVec2(-1, 0))) {
                for (MemoryScanner::AddressInfo addr : scanner.foundAddresses) {
                    scanner.WriteLongMemory(addr.getLPAddress(), valueToReplaceLong);
                }
            }
        }
        else if (selectedMemType == MEM_BYTE) {
            ImGui::InputScalar("##newValue", ImGuiDataType_U8, &valueToReplaceByte);
            if (ImGui::Button("Escrever em TODOS", ImVec2(-1, 0))) {
                for (MemoryScanner::AddressInfo addr : scanner.foundAddresses) {
                    scanner.WriteByteMemory(addr.getLPAddress(), valueToReplaceByte);
                }
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
        if (!speedDLLInjected) {
            wchar_t currentPath[MAX_PATH];
            GetModuleFileNameW(NULL, currentPath, MAX_PATH);
            std::wstring fullPath(currentPath);
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            std::wstring dllPath = fullPath.substr(0, lastSlash + 1) + L"speedhack.dll";
            std::string finalPath(dllPath.begin(), dllPath.end());
            std::cout << "speed hack dll is: " << finalPath << "\n";
            if (Utils::InjectDLL(scanner.targetPID, finalPath.c_str())) {
                speedDLLInjected = true;
            }
        } else {
            if (ImGui::SliderFloat("Fator de Velocidade", &speedFactor, 0.1f, 10.0f, "%.1fx")) {
                if (!scanner.UpdateRemoteSpeed(scanner.targetPID, speedFactor)) {
                    speedDLLInjected = false;
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