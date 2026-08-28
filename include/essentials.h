#pragma once

#include <stdint.h>
#include "types.h"

typedef void* HANDLE;
typedef int BOOL;
#define FALSE 0
#define TRUE 1

void* wmalloc(uint32_t size);
void* wrealloc(void* ptr, uint32_t size);
void* wcalloc(uint32_t count, uint32_t size);
void wfree(void* ptr);
void* memcpy(void* dst, const void* src, size_t n);
size_t wstrlen(const char* s);
int wstrcmp(const char* a, const char* b);

HANDLE wfopen(const char* path, const char* mode);
HANDLE wfcreate(const char* path);
size_t wfwrite(const void* ptr, size_t size, HANDLE file);
int wfclose(HANDLE file);
uint64_t wfsize(HANDLE file);
size_t wfread(void* ptr, size_t size, size_t count, HANDLE file);
int wfseeki64(HANDLE file, int64_t offset, int origin);
int64_t wftelli64(HANDLE file);

void exit(int exit_code);
void print(const char* str);
void print_usage();
void removeSymbol(char text[], char symbol, char* clean);
void print_uint(uint64_t v);
size_t u32_to_str(char* out, uint32_t value);
void concat(char* out, const char* a, const char* b);
void progressBar(double progress, uint32_t trackProg, uint32_t fakeTracks);
SynthEvent* Sort(uint32_t count, SynthEvent* arr, SynthEvent* temp);
void* memset(void* dest, int c, size_t n);