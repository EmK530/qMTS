#include <stdint.h>
#include <Windows.h>

#include "essentials.h"
#include "types.h"

void* wmalloc(uint32_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}
void* wrealloc(void* ptr, uint32_t size)
{
    if(ptr == NULL)
        return HeapAlloc(GetProcessHeap(), 0, size);
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}
void* wcalloc(uint32_t count, uint32_t size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count * size);
}
void wfree(void* ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
void* memcpy(void* dst, const void* src, size_t n)
{
    unsigned char* d = dst;
    const unsigned char* s = src;

    while (n--)
        *d++ = *s++;

    return dst;
}
size_t wstrlen(const char* s)
{
    const char* p = s;
    while (*p) p++;
    return (size_t)(p - s);
}
int wstrcmp(const char* a, const char* b)
{
    while (*a && (*a == *b))
    {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

HANDLE wfopen(const char* path, const char* mode)
{
    DWORD access;
    DWORD creation;
    if (mode[0] == 'r')
    {
        access = GENERIC_READ;
        creation = OPEN_EXISTING;
    }
    else if (mode[0] == 'w')
    {
        access = GENERIC_WRITE;
        creation = CREATE_ALWAYS;
    }
    else
    {
        return NULL;
    }
    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return NULL;
    return h;
}
HANDLE wfcreate(const char* path)
{
    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return NULL;
    return h;
}
int wfclose(HANDLE file)
{
    if (!file)
        return -1;
    CloseHandle(file);
    return 0;
}
uint64_t wfsize(HANDLE file)
{
    LARGE_INTEGER size;

    if (!GetFileSizeEx(file, &size))
        return 0;

    return (uint64_t)size.QuadPart;
}
size_t wfread(void* ptr, size_t size, size_t count, HANDLE file)
{
    DWORD bytesRead;
    if (!ReadFile(file, ptr, (DWORD)(size * count), &bytesRead, NULL))
        return 0;
    return bytesRead / size;
}
size_t wfwrite(const void* ptr, size_t size, HANDLE file)
{
    DWORD bytesWritten;
    if (!WriteFile(file, ptr, (DWORD)size, &bytesWritten, NULL))
        return 0;
    return bytesWritten / size;
}
int wfseeki64(HANDLE file, int64_t offset, int origin)
{
    DWORD moveMethod;
    switch (origin)
    {
        case 0: moveMethod = FILE_BEGIN; break;
        case 1: moveMethod = FILE_CURRENT; break;
        case 2: moveMethod = FILE_END; break;
        default: return -1;
    }
    LONG lo = (LONG)(offset & 0xFFFFFFFF);
    LONG hi = (LONG)(offset >> 32);
    DWORD result = SetFilePointer(file, lo, &hi, moveMethod);
    if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
        return -1;
    return 0;
}
int64_t wftelli64(HANDLE file)
{
    LARGE_INTEGER zero;
    LARGE_INTEGER result;
    zero.QuadPart = 0;
    if (!SetFilePointerEx(file, zero, &result, FILE_CURRENT))
        return -1;
    return result.QuadPart;
}

void exit(int exit_code)
{
    ExitProcess(exit_code);
}
void print(const char* str) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str, lstrlenA(str), &written, NULL);
}
void print_usage()
{
    print("Usage:\n");
    print("  qmts <midifile>\n\n");
    print("No midifile input received, opening file dialog.\n");

    //Sleep(2000);
    //exit(0);
}
void removeSymbol(char text[], char symbol, char* clean){
    int i,j;
    for(i = 0, j = 0; i < lstrlenA(text); i++){
        if(text[i] != symbol){
            clean[j++] = text[i];
        }
    }
    clean[j] = '\0';
}
void print_uint(uint64_t v) {
    char buf[11];
    char *p = buf + 10;
    *p = 0;
    do {
        *--p = '0' + (v % 10);
        v /= 10;
    } while (v);
    print(p);
}
size_t u32_to_str(char* out, uint32_t value)
{
    char temp[10];
    size_t i = 0;
    if (value == 0)
    {
        out[0] = '0';
        return 1;
    }
    while (value > 0)
    {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }
    size_t len = i;
    for (size_t j = 0; j < i; j++)
        out[j] = temp[i - j - 1];
    return len;
}
void concat(char* out, const char* a, const char* b)
{
    char* p = out;
    while (*a)
        *p++ = *a++;
    while (*b)
        *p++ = *b++;
    *p = '\0';
}

static HANDLE hConsole;
static int lastPos = -1;
void progressBar(double progress, uint32_t trackProg, uint32_t fakeTracks)
{
    const int barWidth = 50;
    int pos = (int)(progress * barWidth);
    if(pos == lastPos)
    {
        if(!hConsole)
            hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hConsole, &info);
        COORD pos;
        pos.X = (SHORT)(barWidth+3);
        pos.Y = info.dwCursorPosition.Y;
        SetConsoleCursorPosition(hConsole, pos);
        print_uint((uint32_t)(progress * 100.0));
        print("% (");
        print_uint(trackProg);
        print("/");
        print_uint(fakeTracks);
        print(")");
        return;
    }

    print("\r[");
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) print("=");
        else if (i == pos) print(">");
        else print(" ");
    }
    lastPos = pos;
    print("] ");
    print_uint((uint32_t)(progress * 100.0));
    print("% (");
    print_uint(trackProg);
    print("/");
    print_uint(fakeTracks);
    print(")");
}

static inline int cmp(SynthEvent* a, SynthEvent* b)
{
    if (a->tick != b->tick)
        return (a->tick < b->tick) ? -1 : 1;
    if (a->track != b->track)
        return (a->track < b->track) ? -1 : 1;
    return 0;
}

SynthEvent* Sort(uint32_t count, SynthEvent* arr, SynthEvent* temp)
{
    for (uint32_t width = 1; width < count; width *= 2)
    {
        for (uint32_t i = 0; i < count; i += 2 * width)
        {
            uint32_t left = i;
            uint32_t mid = i + width;
            uint32_t right = i + 2 * width;

            if (mid > count) mid = count;
            if (right > count) right = count;

            uint32_t p = left;
            uint32_t l = left;
            uint32_t r = mid;

            while (l < mid && r < right)
            {
                if (cmp(&arr[l], &arr[r]) <= 0)
                    temp[p++] = arr[l++];
                else
                    temp[p++] = arr[r++];
            }

            while (l < mid)
                temp[p++] = arr[l++];

            while (r < right)
                temp[p++] = arr[r++];
        }

        SynthEvent* swap = arr;
        arr = temp;
        temp = swap;
    }

    return arr;
}

void* memset(void* dest, int c, size_t n)
{
    unsigned char* d = dest;

    while (n--)
        *d++ = (unsigned char)c;

    return dest;
}