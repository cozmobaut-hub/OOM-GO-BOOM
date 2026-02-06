// hog.c — workers hog RAM; controller respawns 2x when one dies
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void run_worker(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    SIZE_T pageSize = si.dwPageSize;

    const SIZE_T chunkSize = 64ULL * 1024 * 1024; // 64 MB
    unsigned long long total = 0;
    int pattern = 0;
    DWORD pid = GetCurrentProcessId();

    printf("[WORKER %lu] Page size: %zu bytes\n", pid, (size_t)pageSize);

    for (;;) {
        void *p = VirtualAlloc(NULL, chunkSize,
                               MEM_RESERVE | MEM_COMMIT,
                               PAGE_READWRITE);
        if (!p) {
            DWORD err = GetLastError();
            printf("[WORKER %lu] VirtualAlloc FAILED at %.2f MB, error=%lu\n",
                   pid,
                   (double)total / (1024.0 * 1024.0),
                   (unsigned long)err);
            fflush(stdout);
            break; // stop allocating but keep process alive
        }

        volatile char *c = (volatile char *)p;
        for (SIZE_T off = 0; off < chunkSize; off += pageSize) {
            c[off] = (char)(pattern & 0xFF);
        }

        total += chunkSize;
        printf("[WORKER %lu] Hogged %.2f MB so far\n",
               pid,
               (double)total / (1024.0 * 1024.0));
        fflush(stdout);

        pattern++;
        Sleep(10);
    }

    printf("[WORKER %lu] Done hogging, sleeping forever...\n", pid);
    for (;;)
        Sleep(60000);
}

static BOOL spawn_worker(const char *exePath, PROCESS_INFORMATION *piOut) {
    char cmdLine[512];
    snprintf(cmdLine, sizeof(cmdLine), "\"%s\" worker", exePath);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(piOut, sizeof(*piOut));

    BOOL ok = CreateProcessA(
        NULL,
        cmdLine,
        NULL, NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        piOut
    );
    if (!ok) {
        DWORD err = GetLastError();
        printf("[CONTROLLER] CreateProcess failed, error=%lu\n", (unsigned long)err);
    } else {
        printf("[CONTROLLER] Spawned worker PID=%lu\n",
               (unsigned long)piOut->dwProcessId);
    }
    return ok;
}

int main(int argc, char **argv) {
    if (argc >= 2 && _stricmp(argv[1], "worker") == 0) {
        run_worker();
        return 0;
    }

    // CONTROLLER MODE
    char exePath[MAX_PATH];
    if (!GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        fprintf(stderr, "GetModuleFileName failed, error=%lu\n", GetLastError());
        return 1;
    }

    const int initialWorkers = 10; // starting number
    const int maxTracked     = 512;

    PROCESS_INFORMATION workers[maxTracked];
    int workerCount = 0;

    printf("[CONTROLLER] Spawning %d initial workers...\n", initialWorkers);

    for (int i = 0; i < initialWorkers && workerCount < maxTracked; ++i) {
        if (spawn_worker(exePath, &workers[workerCount])) {
            workerCount++;
        }
    }

    printf("[CONTROLLER] Entering monitor loop (1 dead -> spawn 2)...\n");

    for (;;) {
        Sleep(2000);

        for (int i = 0; i < workerCount; ++i) {
            HANDLE hProc = workers[i].hProcess;
            if (!hProc) continue;

            DWORD exitCode = 0;
            if (!GetExitCodeProcess(hProc, &exitCode)) {
                // Can't query; assume dead and close handle
                CloseHandle(hProc);
                workers[i].hProcess = NULL;
                workers[i].hThread  = NULL;
                continue;
            }

            if (exitCode != STILL_ACTIVE) {
                printf("[CONTROLLER] Worker PID=%lu died (exit=%lu). Spawning 2 more.\n",
                       (unsigned long)workers[i].dwProcessId,
                       (unsigned long)exitCode);

                CloseHandle(workers[i].hProcess);
                CloseHandle(workers[i].hThread);
                workers[i].hProcess = NULL;
                workers[i].hThread  = NULL;

                // Spawn 2 new workers (if we have room)
                for (int k = 0; k < 2 && workerCount < maxTracked; ++k) {
                    if (spawn_worker(exePath, &workers[workerCount])) {
                        workerCount++;
                    }
                }
            }
        }
    }
}
