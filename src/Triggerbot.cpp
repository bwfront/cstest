#include "TriggerBot.h"
#include <tlhelp32.h>
#include <thread>
#include <chrono>
#include <iostream>

// offsets
// https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.json
DWORD_PTR dwEntityList = 0x1A37A30;
DWORD_PTR dwLocalPlayerPawn = 0x188BF30;
DWORD m_iIDEntIndex = 0x1458;
DWORD m_iTeamNum = 0x3E3;
DWORD m_iHealth = 0x344;

// function to get the base address of a module
DWORD_PTR GetModuleBaseAddress(DWORD procId, const wchar_t *modName)
{
    DWORD_PTR modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, modName))
                {
                    modBaseAddr = (DWORD_PTR)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }
    return modBaseAddr;
}

// function to simulate a left mouse click
void LeftMouseClick()
{
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(10 + rand() % 40));

    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

// function to process the trigger bot
void ProcessTriggerBot(HANDLE hProcess, DWORD_PTR clientBase)
{
    // read local player pointer
    DWORD_PTR localPlayer = 0;
    if (!ReadProcessMemory(hProcess, (LPCVOID)(clientBase + dwLocalPlayerPawn), &localPlayer, sizeof(localPlayer), nullptr))
    {
        continue;
    }

    // read entity ID
    int entityId = 0;
    if (!ReadProcessMemory(hProcess, (LPCVOID)(localPlayer + m_iIDEntIndex), &entityId, sizeof(entityId), nullptr))
    {
        continue;
    }

    if (entityId > 0)
    {
        // read entitiy list
        DWORD_PTR entityList = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)(clientBase + dwEntityList), &entityList, sizeof(entityList), nullptr))
        {
            continue;
        }

        // calc entity addresses
        DWORD_PTR entEntry = 0;
        SIZE_T bytesRead = 0;
        // (entityId >> 9) * 0x8 + 0x10
        if (!ReadProcessMemory(hProcess, (LPCVOID)(entityList + 0x8 * (entityId >> 9) + 0x10), &entEntry, sizeof(entEntry), &bytesRead))
        {
            continue;
        }

        DWORD_PTR entity = 0;
        // 120 * (entityId & 0x1FF)
        if (!ReadProcessMemory(hProcess, (LPCVOID)(entEntry + 120 * (entityId & 0x1FF)), &entity, sizeof(entity), &bytesRead))
        {
            continue;
        }

        // read team ids
        int entityTeam = 0, playerTeam = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)(entity + m_iTeamNum), &entityTeam, sizeof(entityTeam), nullptr))
        {
            continue;
        }
        if (!ReadProcessMemory(hProcess, (LPCVOID)(localPlayer + m_iTeamNum), &playerTeam, sizeof(playerTeam), nullptr))
        {
            continue;
        }

        // check if entity is not on the same team
        if (entityTeam != playerTeam)
        {
            int entityHp = 0;
            if (!ReadProcessMemory(hProcess, (LPCVOID)(entity + m_iHealth), &entityHp, sizeof(entityHp), nullptr))
            {
                continue;
            }
            std::cout << "Entity HP: " << entityHp << std::endl;
            if (entityHp > 0)
            {
                // random sleep time
                std::this_thread::sleep_for(std::chrono::milliseconds(distLoop(gen)));
                LeftMouseClick();
            }
        }
    }
}
