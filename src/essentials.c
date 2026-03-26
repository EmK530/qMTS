#include <Windows.h>

#include "essentials.h"

void print(const char* str) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str, lstrlenA(str), &written, NULL);
}