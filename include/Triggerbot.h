#ifndef TRIGGERBOT_H
#define TRIGGERBOT_H

#include <windows.h>

// offsets
extern DWORD_PTR dwEntityList;
extern DWORD_PTR dwLocalPlayerPawn;
extern DWORD m_iIDEntIndex;
extern DWORD m_iTeamNum;
extern DWORD m_iHealth;

// Function Prototypes
DWORD_PTR GetModuleBaseAddress(DWORD procId, const wchar_t* modName);
void LeftMouseClick();
void ProcessTriggerBot(HANDLE hProcess, DWORD_PTR clientBase);

#endif // TRIGGERBOT_H
