# Classic DLL Injection — PoC

A minimal proof-of-concept demonstrating the classic **DLL injection via `CreateRemoteThread`** technique on Windows.

## Accompanying Article

This repository accompanies the following technical article, which explains the Windows internals behind the implementation, including virtual memory, VADs, PE loading, handles, threads, and detection considerations.

[**Beyond the APIs: A Windows Internals Deep Dive into Process Injection**](https://medium.com/@ronepstein7/beyond-the-apis-a-windows-internals-deep-dive-into-process-injection-afeb4d756bdd?sharedUserId=ronepstein7)

---

## How It Works

DLL injection abuses the Windows loader to force a target process to load an arbitrary DLL. This is achieved in four steps:

```
Injector                          Target Process (e.g. notepad.exe)
─────────────────────────────     ──────────────────────────────────
1. OpenProcess()              →   obtain handle
2. VirtualAllocEx()           →   allocate RW buffer
3. WriteProcessMemory()       →   write DLL path string into buffer
4. CreateRemoteThread()       →   spawn thread → LoadLibraryA(dll path)
                                                  └─ DllMain() called
                                                     └─ payload executes
```

**Why does this work?**  
`LoadLibraryA` is exported from `kernel32.dll`, which is mapped at the **same base address in every process**. This means the function pointer resolved in the injector is valid inside the target process as well — making it a perfect candidate for `CreateRemoteThread`'s start routine.

---

## Files

| File | Description |
|------|-------------|
| `injector.c` | Opens the target process and loads the DLL into it |
| `MyDll.c` | Payload DLL — shows a message box on attach to prove execution |
| `BUILD.md` | Compile instructions |

---

## Usage

### 1. Build

See [BUILD.md](BUILD.md).

### 2. Find the target PID

Open Task Manager or Process Hacker and note the PID of your target (e.g. `notepad.exe`).

### 3. Run the injector

```
injector.exe <PID> <full path to MyDll.dll>
```

**Example:**
```
injector.exe 1234 C:\Users\user\Desktop\MyDll.dll
```

**Expected output:**
```
[*] Target PID : 1234
[*] DLL path   : C:\Users\user\Desktop\MyDll.dll
[+] Opened process handle: 0x000000A0
[+] Allocated remote buffer at: 0x000001F3A2B40000
[+] DLL path written to remote buffer
[+] Remote thread created. Thread ID visible in Process Hacker.
[+] Injection complete.
```

A message box will appear inside the target process as proof of execution.

---

## Observing in Process Hacker

In Process Hacker, open the target process → **Threads** tab. During injection you will see:
- A new thread with start address `kernel32.dll!LoadLibraryAStub` (may show as `kernel32.dll+0x42cd0` if symbols are not loaded)
- `MyDll.dll` appearing in the **Modules** tab after the thread completes

---

## Detection Notes

This technique is well-known and flagged by most EDRs. Common detection points:
- `OpenProcess` with `PROCESS_ALL_ACCESS` on a sensitive process
- `VirtualAllocEx` + `WriteProcessMemory` cross-process
- `CreateRemoteThread` pointing to `LoadLibraryA`
- Unsigned DLL appearing in a signed process's module list
