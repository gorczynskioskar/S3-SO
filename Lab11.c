#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>

typedef struct
{
    long first;
    long size;
    int mode;      // 0 = SUM, 1 = VAR
    double mean;
} Args;

static float* data = NULL;
static double gsum = 0.0;
static double gvar = 0.0;

static HANDLE mutex;

double gettime(void)
{
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

DWORD WINAPI worker(LPVOID p)
{
    Args* a = (Args*)p;
    DWORD tid = GetCurrentThreadId();

    if (a->mode == 0)
    {
        printf("SUM: Thread %lx first=%ld size=%ld\n", tid, a->first, a->size);
    }
    else
    {
        printf("VAR: Thread %lx first=%ld size=%ld\n", tid, a->first, a->size);
    }

    double local = 0.0;

    if (a->mode == 0)
    {
        for (long i = 0; i < a->size; i++)
        {
            local += data[a->first + i];
        }
    }
    else
    {
        for (long i = 0; i < a->size; i++)
        {
            double d = (double)data[a->first + i] - a->mean;
            local += d * d;
        }
    }

    WaitForSingleObject(mutex, INFINITE);

    if (a->mode == 0)
    {
        gsum += local;
    }
    else
    {
        gvar += local;
    }

    ReleaseMutex(mutex);

    if (a->mode == 0)
    {
        printf("SUM: Thread %lx sum=%.17lf\n", tid, local);
    }
    else
    {
        printf("VAR: Thread %lx var=%.17lf\n", tid, local);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Error: wrong arguments.\n");
        return 1;
    }

    long n = atol(argv[1]);
    long w = atol(argv[2]);

    if (n < 1 || n > 100000000 || w < 1 || w > 16 || w > n)
    {
        fprintf(stderr, "Error: invalid argument values.\n");
        return 1;
    }

    data = (float*)malloc(sizeof(float) * n);

    if (data == NULL)
    {
        fprintf(stderr, "Error: malloc failed.\n");
        return 1;
    }

    srand(0);

    for (long i = 0; i < n; i++)
    {
        data[i] = rand() * 100.f / (float)RAND_MAX;
    }

    long base = n / w;
    long rem = n % w;

    mutex = CreateMutex(NULL, FALSE, NULL);

    double t1 = gettime();

    HANDLE* threads = (HANDLE*)malloc(sizeof(HANDLE) * w);
    Args* args = (Args*)malloc(sizeof(Args) * w);

    long pos = 0;

    // --- SUMA ---
    for (int i = 0; i < w; i++)
    {
        long sz = base + ((i == w - 1) ? rem : 0);

        args[i].first = pos;
        args[i].size = sz;
        args[i].mode = 0;
        args[i].mean = 0.0;

        threads[i] = CreateThread(NULL, 0, worker, &args[i], 0, NULL);

        pos += sz;
    }

    WaitForMultipleObjects(w, threads, TRUE, INFINITE);

    for (int i = 0; i < w; i++)
    {
        CloseHandle(threads[i]);
    }

    double mean = gsum / (double)n;

    // --- WARIANCJA ---
    gvar = 0.0;
    pos = 0;

    for (int i = 0; i < w; i++)
    {
        long sz = base + ((i == w - 1) ? rem : 0);

        args[i].first = pos;
        args[i].size = sz;
        args[i].mode = 1;
        args[i].mean = mean;

        threads[i] = CreateThread(NULL, 0, worker, &args[i], 0, NULL);

        pos += sz;
    }

    WaitForMultipleObjects(w, threads, TRUE, INFINITE);

    for (int i = 0; i < w; i++)
    {
        CloseHandle(threads[i]);
    }

    double std_threads = sqrt(gvar / (double)n);

    double t2 = gettime();
    double time_threads = t2 - t1;

    // --- SEKWENCYJNIE ---
    double s1 = gettime();

    double ssum = 0.0;

    for (long i = 0; i < n; i++)
    {
        ssum += data[i];
    }

    double smean = ssum / (double)n;

    double svar = 0.0;

    for (long i = 0; i < n; i++)
    {
        double d = (double)data[i] - smean;
        svar += d * d;
    }

    double std_seq = sqrt(svar / (double)n);

    double s2 = gettime();
    double time_seq = s2 - s1;

    printf("w/Threads StdDev=%.17lf time=%.6lf\n", std_threads, time_threads);
    printf("wo/Threads StdDev=%.17lf time=%.6lf\n", std_seq, time_seq);

    free(args);
    free(threads);
    free(data);
    CloseHandle(mutex);

    return 0;
}
