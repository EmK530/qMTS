#pragma once

#include <windows.h>
#include <stdint.h>

typedef struct
{
    HANDLE file;
    uint8_t* buffer;
    uint32_t bufferSize;
    uint32_t bufferPos;
} BufferWrite;

int BufferWrite_Init(BufferWrite* bw, HANDLE file, uint32_t bufferSize);

void BufferWrite_InternalFlush(BufferWrite* bw);
static inline void BufferWrite_WriteByte(BufferWrite* bw, uint8_t value)
{
    if (bw->bufferPos >= bw->bufferSize)
        BufferWrite_InternalFlush(bw);

    bw->buffer[bw->bufferPos++] = value;
}
void BufferWrite_WriteVLQ(BufferWrite* bw, uint64_t value);

void BufferWrite_WriteBuffer(
    BufferWrite* bw,
    const void* data,
    uint32_t size);

void BufferWrite_Flush(BufferWrite* bw);

void BufferWrite_Close(BufferWrite* bw);