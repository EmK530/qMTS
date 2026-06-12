#include "BufferFile.h"
#include "essentials.h"
#include <windows.h>
#include <string.h>

uint8_t* data = NULL;
uint64_t fileSize = 0;
uint64_t pos = 0;

int Pushback = -1;
int fileEnded = 0;

static HANDLE hFile = NULL;
static HANDLE hMap = NULL;

int BufferInit(char path[], unsigned long seek, unsigned int bufSizee)
{
    (void)bufSizee;

    Pushback = -1;
    fileEnded = 0;
    pos = seek;

    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    DWORD high;
    DWORD low = GetFileSize(hFile, &high);

    fileSize = ((uint64_t)high << 32) | low;

    hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap) return 0;

    data = (uint8_t*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

    return data != NULL;
}

void BufferFree(void)
{
    if (data) {
        UnmapViewOfFile(data);
        data = NULL;
    }

    if (hMap) {
        CloseHandle(hMap);
        hMap = NULL;
    }

    if (hFile) {
        CloseHandle(hFile);
        hFile = NULL;
    }
}

void Seek(long long newPos)
{
    if (newPos < 0) newPos = 0;
    if ((uint64_t)newPos > fileSize) newPos = fileSize;

    pos = (uint64_t)newPos;
}

void Skip(uint64_t count)
{
    pos += count;
}

unsigned char ReadFast()
{
    return data[pos++];
}

uint64_t ReadVLQ()
{
    uint64_t val = 0;
    uint8_t c;

    do {
        c = data[pos++];
        val = (val << 7) | (c & 0x7F);
    } while (c & 0x80);

    return val;
}

unsigned char* ReadRange(int size)
{
    unsigned char* range = (unsigned char*)wmalloc(size + 1);

    memcpy(range, data + pos, size);
    range[size] = 0;

    pos += size;
    return range;
}

void Copy(unsigned char* target,
          unsigned long int offset,
          unsigned long int size)
{
    if (size == 0)
        size = (unsigned long int)fileSize;

    memcpy(target + offset, data + pos, size);
    pos += size;
}

int IsEOF()
{
    return pos >= fileSize;
}