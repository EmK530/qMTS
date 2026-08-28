#pragma once

#include <stdint.h>

typedef struct {
    uint64_t Position;
    uint32_t Length;
} TrackPointer;

typedef struct {
    uint64_t tick;
    uint16_t track;
    uint16_t size_data;
    uint8_t isSysex;
    unsigned char fixedData[6];
    void* allocated_data;
} SynthEvent;

static char FilePrefix[] = "MThd\x00\x00\x00\x06\x00\x00\x00\x01";
static char TrackPrefix[] = "MTrkTEMP"; // TEMP gets overwritten post-write
static char TrackSuffix[] = "\x00\xFF\x2F\x00";