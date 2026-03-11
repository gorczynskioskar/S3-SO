#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

enum
{
    SHM_SIZE = 101,  // 1 bajt steruj¹cy + 100 bajtów danych
    CTRL_EMPTY = 0,
    CTRL_HELLO = 250,
    CTRL_EOF = 254,
    CTRL_R_DONE = 252
};

static void die(const char* msg)
{
    fprintf(stderr, "%s (GetLastError=%lu)\n", msg, (unsigned long)GetLastError());
    ExitProcess(1);
}

int main(int argc, char* argv[])
{
    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "Usage: %s <source_file> <mapping_name>\n", argv[0]);
        return 1;
    }

    const char* src_path = argv[1];
    const char* map_name = argv[2];

    HANDLE hMap = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        SHM_SIZE,
        map_name
    );

    if (hMap == NULL)
    {
        die("CreateFileMapping failed");
    }

    unsigned char* base = (unsigned char*)MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHM_SIZE
    );

    if (base == NULL)
    {
        die("MapViewOfFile failed");
    }

    volatile unsigned char* ctrl = base;     // 1 bajt steruj¹cy
    unsigned char* buf = base + 1;           // 100 bajtów danych

    *ctrl = CTRL_EMPTY;

    int fd = _open(src_path, _O_RDONLY | _O_BINARY);
    if (fd == -1)
    {
        fprintf(stderr, "cannot read file: %s\n", src_path);

        UnmapViewOfFile(base);
        CloseHandle(hMap);
        return 1;
    }

    printf("shared memory attached\n");
    printf("waiting for receiver...\n");

    while (*ctrl != CTRL_HELLO)
    {
    }

    *ctrl = CTRL_EMPTY;

    for (;;)
    {
        while (*ctrl != CTRL_EMPTY)
        {
        }

        int r = _read(fd, buf, 100);
        if (r < 0)
        {
            fprintf(stderr, "read failed\n");

            _close(fd);
            UnmapViewOfFile(base);
            CloseHandle(hMap);
            return 1;
        }

        if (r == 0)
        {
            *ctrl = CTRL_EOF;
            break;
        }

        *ctrl = (unsigned char)r;

        printf("%d byte(s) sent...\n", r);
    }

    while (*ctrl != CTRL_R_DONE)
    {
    }

    if (_close(fd) == -1)
    {
        fprintf(stderr, "close failed\n");
    }

    if (!UnmapViewOfFile(base))
    {
        die("UnmapViewOfFile failed");
    }

    if (!CloseHandle(hMap))
    {
        die("CloseHandle failed");
    }

    printf("copying finished, receiver finished\n");
    printf("shared memory detached\n");

    return 0;
}
