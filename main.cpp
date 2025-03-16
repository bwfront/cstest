#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <string>

// https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.json
DWORD_PTR dwEntityList      = 0x1A37A30;
DWORD_PTR dwLocalPlayerPawn = 0x188BF30;
DWORD m_iIDEntIndex         = 0x1458;
DWORD m_iTeamNum            = 0x3E3;
DWORD m_iHealth             = 0x1410;

// Funktion, um den Modul-Basisadresse von client.dll zu finden
DWORD_PTR GetModuleBaseAddress(DWORD procId, const wchar_t* modName) {
    DWORD_PTR modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, modName)) {
                    modBaseAddr = (DWORD_PTR)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }
    return modBaseAddr;
}

// Funktion, um einen linken Mausklick zu simulieren
void LeftMouseClick() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    // Zufällige kurze Pause
    std::this_thread::sleep_for(std::chrono::milliseconds(10 + rand() % 40));

    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

int main() {
    std::cout << "[-] TriggerBot gestartet.\n[-] Trigger-Taste: SHIFT" << std::endl;

    // Suchen des Prozesses "cs2.exe"
    DWORD procId = 0;
    HWND hWnd = FindWindow(NULL, L"Counter-Strike 2");
    if (hWnd == NULL) {
        std::cout << "Bitte Counter-Strike 2 starten!" << std::endl;
        return 1;
    }

    // Hole den Prozess-ID über den Fensterhandle
    GetWindowThreadProcessId(hWnd, &procId);
    if (procId == 0) {
        std::cout << "Prozess-ID konnte nicht ermittelt werden." << std::endl;
        return 1;
    }

    // Öffne den Prozess mit Lesezugriff
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, procId);
    if (!hProcess) {
        std::cout << "Fehler beim Öffnen des Prozesses." << std::endl;
        return 1;
    }

    // Hole die Basisadresse von "client.dll"
    DWORD_PTR clientBase = GetModuleBaseAddress(procId, L"client.dll");
    if (!clientBase) {
        std::cout << "client.dll nicht gefunden." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    // Zufallszahlengenerator für Schlafzeiten
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distClick(10, 50);   // in Millisekunden
    std::uniform_int_distribution<> distLoop(10, 30);      // in Millisekunden

    while (true) {
        wchar_t wndTitle[256];
        HWND foreground = GetForegroundWindow();
        GetWindowTextW(foreground, wndTitle, sizeof(wndTitle) / sizeof(wchar_t));
        if (std::wstring(wndTitle) != L"Counter-Strike 2") {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Prüfe, ob SHIFT gedrückt wird
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
            // Lese den Spieler-Pointer
            DWORD_PTR localPlayer = 0;
            if (!ReadProcessMemory(hProcess, (LPCVOID)(clientBase + dwLocalPlayerPawn), &localPlayer, sizeof(localPlayer), nullptr)) {
                continue;
            }

            // Lese Entity-ID
            int entityId = 0;
            if (!ReadProcessMemory(hProcess, (LPCVOID)(localPlayer + m_iIDEntIndex), &entityId, sizeof(entityId), nullptr)) {
                continue;
            }

            if (entityId > 0) {
                // Lese Entity-Liste
                DWORD_PTR entityList = 0;
                if (!ReadProcessMemory(hProcess, (LPCVOID)(clientBase + dwEntityList), &entityList, sizeof(entityList), nullptr)) {
                    continue;
                }

                // Berechne Adressen basierend auf entityId
                DWORD_PTR entEntry = 0;
                SIZE_T bytesRead = 0;
                // Annahme: (entityId >> 9) * 0x8 + 0x10
                if (!ReadProcessMemory(hProcess, (LPCVOID)(entityList + 0x8 * (entityId >> 9) + 0x10), &entEntry, sizeof(entEntry), &bytesRead)) {
                    continue;
                }
                DWORD_PTR entity = 0;
                // Annahme: 120 * (entityId & 0x1FF)
                if (!ReadProcessMemory(hProcess, (LPCVOID)(entEntry + 120 * (entityId & 0x1FF)), &entity, sizeof(entity), &bytesRead)) {
                    continue;
                }

                // Lese Team-IDs
                int entityTeam = 0, playerTeam = 0;
                if (!ReadProcessMemory(hProcess, (LPCVOID)(entity + m_iTeamNum), &entityTeam, sizeof(entityTeam), nullptr)) {
                    continue;
                }
                if (!ReadProcessMemory(hProcess, (LPCVOID)(localPlayer + m_iTeamNum), &playerTeam, sizeof(playerTeam), nullptr)) {
                    continue;
                }

                // Wenn Gegner
                if (entityTeam != playerTeam) {
                    int entityHp = 0;
                    if (!ReadProcessMemory(hProcess, (LPCVOID)(entity + m_iHealth), &entityHp, sizeof(entityHp), nullptr)) {
                        continue;
                    }
                    if (entityHp > 0) {
                        // Zufällige Verzögerung vor dem Schuss
                        std::this_thread::sleep_for(std::chrono::milliseconds(distLoop(gen)));
                        LeftMouseClick();
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    CloseHandle(hProcess);
    return 0;
}
