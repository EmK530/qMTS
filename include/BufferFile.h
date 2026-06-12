#pragma once

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

void Copy(unsigned char* target,
          unsigned long int offset,
          unsigned long int size);

int IsEOF(void);

static inline unsigned char ReadFastInline(void)
{
    return data[pos++];
}