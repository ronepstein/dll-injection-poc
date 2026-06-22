# Build Instructions

Requires [MinGW-w64](https://www.mingw-w64.org/) (gcc) on Windows.

## Compile the payload DLL

```bash
gcc -shared -o MyDll.dll MyDll.c -luser32
```

## Compile the injector

```bash
gcc injector.c -o injector.exe
```
