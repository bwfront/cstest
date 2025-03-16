#include "TriggerBot.h"
#include "Utility.h"
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <string>

int main() {
    std::cout << "clientexe start\n[-] Trigger-key: SHIFT" << std::endl;

    // search for cs2 window
    DWORD procId = 0;
    HWND hWnd = FindWindow(NULL, L"Counter-Strike 2");
    if (hWnd == NULL) {
        std::cout << "Bitte Counter-Strike 2 starten!" << std::endl;
        return 1;
    }

    // get process id
    GetWindowThreadProcessId(hWnd, &procId);
    if (procId == 0) {
        std::cout << "Prozess-ID konnte nicht ermittelt werden." << std::endl;
        return 1;
    }

    // open process with read acess
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, procId);
    if (!hProcess) {
        std::cout << "Fehler beim Öffnen des Prozesses." << std::endl;
        return 1;
    }

    // base adress client.dll
    DWORD_PTR clientBase = GetModuleBaseAddress(procId, L"client.dll");
    if (!clientBase) {
        std::cout << "client.dll nicht gefunden." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    // random sleep time
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distLoop(6, 25);      // ms

    while (true) {
        // Check if game is in focus
        wchar_t wndTitle[256];
        HWND foreground = GetForegroundWindow();
        GetWindowTextW(foreground, wndTitle, sizeof(wndTitle) / sizeof(wchar_t));
        if (std::wstring(wndTitle) != L"Counter-Strike 2") {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Triggerbot
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
            ProcessTriggerBot(hProcess, clientBase);
            std::this_thread::sleep_for(std::chrono::milliseconds(distLoop(gen)));
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    CloseHandle(hProcess);
    return 0;
}
