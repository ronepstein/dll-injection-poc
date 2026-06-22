// MyDll.c
// Payload DLL for the process injection PoC.
//
// DllMain is the entry point called automatically by Windows when the DLL
// is loaded into a process. On DLL_PROCESS_ATTACH we show a message box
// as visible proof that code execution was achieved inside the target process.

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // MB_TOPMOST | MB_SETFOREGROUND forces the popup to the front,
        // otherwise it may appear hidden behind other windows.
        MessageBoxA(
            GetForegroundWindow(),
            "DLL injected successfully!\nRunning inside the target process.",
            "Process Injection PoC",
            MB_OK | MB_TOPMOST | MB_SETFOREGROUND
        );
    }
    return TRUE;
}
