#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

enum
{
    SHM_SIZE = 101,
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
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <dest_file> <mapping_name>\n", argv[0]);
        return 1;
    }

    const char* dst_path = argv[1];
    const char* map_name = argv[2];

    HANDLE hMap = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        map_name
    );

    if (hMap == NULL)
    {
        die("OpenFileMapping failed");
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

    volatile unsigned char* ctrl = base;
    unsigned char* buf = base + 1;

    int fd = _open(dst_path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
        _S_IREAD | _S_IWRITE);
    if (fd == -1)
    {
        fprintf(stderr, "cannot open dest file: %s\n", dst_path);

        UnmapViewOfFile(base);
        CloseHandle(hMap);
        return 1;
    }

    printf("shared memory attached, ready to receive\n");

    *ctrl = CTRL_HELLO;

    for (;;)
    {
        while (*ctrl == CTRL_EMPTY || *ctrl == CTRL_HELLO)
        {
        }

        if (*ctrl == CTRL_EOF)
        {
            printf("copying finished\n");

            *ctrl = CTRL_R_DONE;

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

            printf("shared memory detached\n");
            return 0;
        }

        unsigned int len = (unsigned int)(*ctrl);

        int wr = _write(fd, buf, len);
        if (wr < 0)
        {
            fprintf(stderr, "write failed\n");

            UnmapViewOfFile(base);
            CloseHandle(hMap);
            _close(fd);
            return 1;
        }
        if ((unsigned int)wr != len)
        {
            fprintf(stderr, "short write\n");

            UnmapViewOfFile(base);
            CloseHandle(hMap);
            _close(fd);
            return 1;
        }

        printf("%u byte(s) received...\n", len);

        *ctrl = CTRL_EMPTY;
    }
}