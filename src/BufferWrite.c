#include "BufferWrite.h"
#include "essentials.h"

void BufferWrite_InternalFlush(BufferWrite* bw)
{
    if (bw->bufferPos == 0)
        return;

    DWORD written = 0;

    WriteFile(
        bw->file,
        bw->buffer,
        bw->bufferPos,
        &written,
        NULL);

    bw->bufferPos = 0;
}

int BufferWrite_Init(
    BufferWrite* bw,
    HANDLE file,
    uint32_t bufferSize)
{
    bw->file = file;
    bw->bufferSize = bufferSize;
    bw->bufferPos = 0;
    bw->buffer = wmalloc(bufferSize);

    return bw->buffer != NULL;
}

void BufferWrite_WriteVLQ(BufferWrite* bw, uint64_t value)
{
    char buffer[5];
    int i = 4;
    buffer[i] = (char)(value & 0x7F);
    value >>= 7;
    while (value > 0)
    {
        buffer[--i] = (char)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    BufferWrite_WriteBuffer(bw, buffer + i, 5 - i);
}

void BufferWrite_WriteBuffer(
    BufferWrite* bw,
    const void* data,
    uint32_t size)
{
    const uint8_t* src = (const uint8_t*)data;

    while (size)
    {
        uint32_t remaining =
            bw->bufferSize - bw->bufferPos;

        if (remaining == 0)
        {
            BufferWrite_InternalFlush(bw);
            remaining = bw->bufferSize;
        }

        if (size >= bw->bufferSize)
        {
            BufferWrite_InternalFlush(bw);

            DWORD written;
            WriteFile(
                bw->file,
                src,
                size,
                &written,
                NULL);

            src += written;
            size -= written;
            continue;
        }

        uint32_t copySize =
            size < remaining ? size : remaining;

        memcpy(
            bw->buffer + bw->bufferPos,
            src,
            copySize);

        bw->bufferPos += copySize;
        src += copySize;
        size -= copySize;
    }
}

void BufferWrite_Flush(BufferWrite* bw)
{
    BufferWrite_InternalFlush(bw);
}

void BufferWrite_Close(BufferWrite* bw)
{
    BufferWrite_InternalFlush(bw);

    if (bw->buffer)
    {
        wfree(bw->buffer);
        bw->buffer = NULL;
    }
}