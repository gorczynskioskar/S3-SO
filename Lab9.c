#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static int is_natural_number_str(const char* s) {
    if (!s || *s == '\0') return 0;
    for (const char* p = s; *p; ++p) {
        if ((unsigned char)*p < '0' || (unsigned char)*p > '9') return 0;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: only one argument is allowed.\n");
        return 1;
    }
    if (is_natural_number_str(argv[1]) == 0) {
        fprintf(stderr, "Error: the argument must be a natural number.\n");
        return 2;
    }
    int argument = atoi(argv[1]);
    if (argument < 1 || argument > 13) {
        fprintf(stderr, "Error: the argument must be a number between 1 and 13.\n");
        return 3;
    }
    if (argument == 1 || argument == 2) {
        return 1;
    }
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    char arg1[16];
    snprintf(arg1, sizeof(arg1), "%d", argument - 1);

    char exePath[MAX_PATH];
    DWORD got = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) {
        fprintf(stderr, "Error: cannot resolve module filename. GetLastError=%lu\n", GetLastError());
        return 4;
    }

    char cmdLine1[MAX_PATH + 20];
    snprintf(cmdLine1, sizeof(cmdLine1), "\"%s\" %s", exePath, arg1);

    char arg2[16];
    snprintf(arg2, sizeof(arg2), "%d", argument - 2);
    char cmdLine2[MAX_PATH + 20];
    snprintf(cmdLine2, sizeof(cmdLine2), "\"%s\" %s", exePath, arg2);

    STARTUPINFOA si1 = { 0 }, si2 = { 0 };
    PROCESS_INFORMATION pi1 = { 0 }, pi2 = { 0 };
    si1.cb = sizeof(si1);
    si2.cb = sizeof(si2);

    if (!CreateProcessA(NULL, cmdLine1, NULL, NULL, 0, 0, NULL, NULL, &si1, &pi1)) {
        fprintf(stderr, "Error: CreateProcessA for child1 failed (%lu).\n", (unsigned long)GetLastError());
        return 4;
    }

    if (!CreateProcessA(NULL, cmdLine2, NULL, NULL, 0, 0, NULL, NULL, &si2, &pi2)) {
        fprintf(stderr, "Error: CreateProcessA for child2 failed (%lu).\n", (unsigned long)GetLastError());
        CloseHandle(pi1.hProcess);
        CloseHandle(pi1.hThread);
        return 4;
    }

    struct ChildInfo {
        HANDLE h;
        DWORD pid;
        const char* arg;
        DWORD exitCode;
    } children[2];

    children[0].h = pi1.hProcess;
    children[0].pid = pi1.dwProcessId;
    children[0].arg = arg1;
    children[0].exitCode = 0;

    children[1].h = pi2.hProcess;
    children[1].pid = pi2.dwProcessId;
    children[1].arg = arg2;
    children[1].exitCode = 0;

    HANDLE handles[2] = { children[0].h, children[1].h };

    DWORD waitIndex = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    if (!(waitIndex >= WAIT_OBJECT_0 && waitIndex < WAIT_OBJECT_0 + 2)) {
        fprintf(stderr, "Error: WaitForMultipleObjects failed (%lu).\n", (unsigned long)GetLastError());
        CloseHandle(pi1.hProcess);
        CloseHandle(pi1.hThread);
        CloseHandle(pi2.hProcess);
        CloseHandle(pi2.hThread);
        return 4;
    }

    int firstIdx = (int)(waitIndex - WAIT_OBJECT_0);
    int secondIdx = 1 - firstIdx;

    if (!GetExitCodeProcess(handles[firstIdx], &children[firstIdx].exitCode)) {
        fprintf(stderr, "Warning: GetExitCodeProcess failed (%lu).\n", (unsigned long)GetLastError());
    }

    WaitForSingleObject(handles[secondIdx], INFINITE);
    if (!GetExitCodeProcess(handles[secondIdx], &children[secondIdx].exitCode)) {
        fprintf(stderr, "Warning: GetExitCodeProcess failed (%lu).\n", (unsigned long)GetLastError());
    }

    CloseHandle(pi1.hProcess);
    CloseHandle(pi1.hThread);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);

    unsigned long parent_PID = (unsigned long)GetCurrentProcessId();
    unsigned long first_child_pid = (unsigned long)children[0].pid;
    unsigned long second_child_pid = (unsigned long)children[1].pid;
    unsigned long first_arg = (unsigned long)atoi(children[0].arg);
    unsigned long second_arg = (unsigned long)atoi(children[1].arg);
    unsigned long first_exit = (unsigned long)children[0].exitCode;
    unsigned long second_exit = (unsigned long)children[1].exitCode;
    unsigned long result = first_exit + second_exit;

    printf("%lu\t%lu\t%lu\t%lu\n", parent_PID, first_child_pid, first_arg, first_exit);
    printf("%lu\t%lu\t%lu\t%lu\n", parent_PID, second_child_pid, second_arg, second_exit);
    printf("%lu\t\t\t%lu\n\n", parent_PID, result);

    return (int)result;
}