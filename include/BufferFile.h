#pragma once

#include <string.h>
#include <stdint.h>

extern uint8_t* data;
extern uint64_t fileSize;
extern uint64_t pos;

extern int Pushback;
extern int fileEnded;

int  BufferInit(char path[], unsigned long seek, unsigned int bufSizee);
void BufferFree(void);

void Seek(long long pos);
void Skip(uint64_t count);

unsigned char ReadFast();
uint64_t ReadVLQ();

unsigned char* ReadRange(int size);

int IsEOF(void);

static inline unsigned char ReadFastInline(void)
{
    return data[pos++];
}

static inline uint32_t ReadU32BE(void)
{
    uint32_t value;
    memcpy(&value, data + pos, sizeof(value));
    pos += sizeof(value);
    return __builtin_bswap32(value);
}