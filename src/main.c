#include <Windows.h>
#include <string.h>

#include "BufferFile.h"
#include "BufferWrite.h"
#include "LinkedList.h"
#include "essentials.h"
#include "types.h"

#ifndef VERSION
  #define VERSION "(null)"
#endif

int TextSearch(char text[]){
    unsigned char* str = ReadRange(wstrlen(text));
    int res = wstrcmp(str,text);
    wfree(str);
    return res;
}

int __main()
{
    print("qMTS - Quick MIDI Track Splitter\nversion ");
    print(VERSION);
    print("\n\n");
    
    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    char path[260];
    char fixedPath[260];
    char* midipath = fixedPath;

    if(argc <= 1) {
        print_usage();
        while(TRUE){
            OPENFILENAME ofn;
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFilter = "MIDI Files\0*.mid\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = sizeof(path);
            ofn.lpstrTitle = "Select a MIDI file";
            ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            path[0] = '\0';
            if(!GetOpenFileNameA(&ofn)){
                print("No file selected.\n");
                continue;
            }
            removeSymbol(path, '\"', fixedPath);
            if(GetFileAttributesA(fixedPath) != INVALID_FILE_ATTRIBUTES){
                break;
            } else {
                print("Invalid path\n");
                ZeroMemory(path, sizeof(path));
            }
        }
    } else {
        WideCharToMultiByte(CP_ACP, 0, argv[1], -1, fixedPath, sizeof(fixedPath), NULL, NULL);
    }

    print("\nLoading: "); print(midipath); print("\n");
    BOOL res = BufferInit(midipath, 0, 256);
    if(!res) {
        print("[ERROR] BufferInit failure, does the file exist?");
        return 1;
    }

    if(TextSearch("MThd")!=0){
        print("Not a valid MIDI file!\n");
        return 1;
    }
    Skip(4); // header size
    uint16_t format = ReadFast() * 256u + ReadFast();
    uint16_t fakeTracks = ReadFast() * 256u + ReadFast();
    uint16_t ppq = ReadFast() * 256u + ReadFast();

    print("\n[STEP 1/4] Indexing tracks...\n");
    LinkedList* trackPointers = LinkedList_create();
    uint32_t realTracks = 0;
    
    progressBar(0, 0, fakeTracks);
    while(1)
    {
        if(fileEnded)
            break;
        if(TextSearch("MTrk")!=0)
            break;
        uint32_t size = (ReadFast() * 16777216u) + (ReadFast() * 65536u) + (ReadFast() * 256u) + ReadFast();
        realTracks++;
        progressBar((double)realTracks / (double)fakeTracks, realTracks, fakeTracks);

        TrackPointer* tptr = wmalloc(sizeof(TrackPointer));
        tptr->Position = pos - 8;
        tptr->Length = size;
        LinkedList_append(trackPointers, tptr);
        Seek(pos + size);
    }
    print("\nFound "); print_uint(realTracks); print(" tracks.\n");
    print("\n[STEP 2/4] Sweeping for synth events...\n");
    LinkedList* synthEvents = LinkedList_create();
    res = BufferInit(midipath, 0, 67108864);
    if(!res)
        exit(1);

    char* tracksHaveNotes = (char*)wmalloc(realTracks * sizeof(char));

    progressBar(0, 0, realTracks);
    uint16_t count = 0;
    uint32_t notes = 0;
    LinkedList_foreach(trackPointers, node) {
        char hasNotes = 0;
        TrackPointer* tptr = (TrackPointer*)node->item;
        count++;
        Seek(tptr->Position);
        if(TextSearch("MTrk")!=0)
            return 1;
        Skip(4);

        BOOL doloop = TRUE;
        uint64_t totalPos = 0;
        u_char tempPrev = 0;
        while(doloop)
        {
            totalPos += ReadVLQ();
            u_char readEvent = ReadFastInline();
            if(readEvent < 0x80)
            {
                pos--;
                readEvent = tempPrev;
            }
            tempPrev = readEvent;
            uint32_t trackEvent = readEvent & 0b11110000;
            if (trackEvent == 0x90)
            {
                Skip(2);
                notes++;
                hasNotes = 1;
            }
            else if (trackEvent == 0x80)
                Skip(2);
            else if (trackEvent == 0xA0 || trackEvent == 0xE0 || trackEvent == 0xB0)
            {
                u_char note = ReadFastInline();
                u_char vel = ReadFastInline();

                SynthEvent* sev = wmalloc(sizeof(SynthEvent));
                sev->tick = totalPos;
                sev->track = count;
                sev->size_data = 3;
                u_char* newData = (u_char*)wmalloc(3);
                newData[0] = readEvent; newData[1] = note; newData[2] = vel;
                sev->allocated_data = newData;
                LinkedList_append(synthEvents, sev);
            }
            else if (trackEvent == 0xC0 || trackEvent == 0xD0)
            {
                u_char idk = ReadFastInline();

                SynthEvent* sev = wmalloc(sizeof(SynthEvent));
                sev->tick = totalPos;
                sev->track = count;
                sev->size_data = 2;
                u_char* newData = (u_char*)wmalloc(2);
                newData[0] = readEvent; newData[1] = idk;
                sev->allocated_data = newData;
                LinkedList_append(synthEvents, sev);
            }
            else if (trackEvent == 0)
                break;
            else
            {
                switch (readEvent)
                {
                    case 0b11110000: // SysEx
                    {
                        uint32_t size = 64;
                        u_char* newData = (u_char*)wmalloc(size);
                        newData[0] = 0xF0;
                        uint32_t sysexLen = 1;
                        u_char c;
                        do {
                            c = ReadFastInline();
                            newData[sysexLen++] = c;
                            if(sysexLen >= size)
                            {
                                size *= 2;
                                newData = (u_char*)wrealloc(newData, size);
                            }
                        } while(c != 0b11110111);

                        SynthEvent* sev = wmalloc(sizeof(SynthEvent));
                        sev->tick = totalPos;
                        sev->track = count;
                        sev->size_data = sysexLen;
                        sev->allocated_data = newData;
                        LinkedList_append(synthEvents, sev);
                        break;
                    }
                    case 0b11110010:
                        //print("Warning! Unhandled E1\n");
                        Skip(2);
                        break;
                    case 0b11110011:
                        //print("Warning! Unhandled E2\n");
                        Skip(1);
                        break;
                    case 0xFF: // Meta Event
                        {
                            readEvent = ReadFastInline();
                            switch (readEvent)
                            {
                                case 0x51: // Tempo Event
                                    SynthEvent* sev = wmalloc(sizeof(SynthEvent));
                                    sev->tick = totalPos;
                                    sev->track = count;
                                    sev->size_data = 6;
                                    u_char* newData = (u_char*)wmalloc(6);
                                    newData[0] = 0xFF; newData[1] = 0x51; newData[2] = ReadFastInline();
                                    newData[3] = ReadFastInline(); newData[4] = ReadFastInline(); newData[5] = ReadFastInline();
                                    sev->allocated_data = newData;
                                    LinkedList_append(synthEvents, sev);
                                    break;
                                case 0x2F: // End of Track
                                    doloop = FALSE;
                                    break;
                                default:
                                    //print("Warning! Unhandled Meta Event: "); print_uint(readEvent); print("\n");
                                    Skip(ReadVLQ());
                                    break;
                            }
                            break;
                        }
                    default:
                        //print("Warning! Unhandled byte: "); print_uint(readEvent); print("\n");
                        break;
                }
            }
        }
        tracksHaveNotes[count-1] = hasNotes;
        progressBar((double)count / (double)realTracks, count, realTracks);
    }

    if(synthEvents->size == 0)
    {
        print("\nThis MIDI has no synth events?\n");
        return 1;
    }

    print("\nLoaded "); print_uint(synthEvents->size); print(" synth events.\n");
    print("Detected "); print_uint(notes); print(" notes. If this is wrong there is a parser error.\n");

    print("\n[STEP 3/4] Sorting synth events...\n");
    uint32_t synthCount = synthEvents->size;
    SynthEvent** arr = wmalloc(sizeof(SynthEvent*) * synthCount);
    SynthEvent** temp = wmalloc(sizeof(SynthEvent*) * synthCount);
    uint32_t i = 0;
    LinkedList_foreach(synthEvents, node)
        arr[i++] = (SynthEvent*)node->item;
    Sort(synthCount, arr, temp);

    // If an odd number of passes occurred, result is in temp, copy back
    uint32_t passes = 0;
    uint32_t width = 1;
    while (width < synthCount) { width *= 2; passes++; }
    if (passes % 2 == 1)
        memcpy(arr, temp, sizeof(SynthEvent*) * synthCount);

    LinkedList_dispose(&synthEvents, NULL);
    wfree(temp);

    SynthEvent* s_arr = wmalloc(sizeof(SynthEvent) * synthCount);
    for(int i = 0; i < synthCount; i++)
    {
        SynthEvent* ev = *(arr+i);
        s_arr[i].tick = ev->tick;
        s_arr[i].track = ev->track;
        s_arr[i].size_data = ev->size_data;
        s_arr[i].allocated_data = ev->allocated_data;
        wfree(ev);
    }
    wfree(arr);

    print("\n[STEP 4/4] Generating isolated tracks...\n");
    res = BufferInit(midipath, 0, 67108864);
    if(!res)
        exit(1);

    char base[256];
    char dir[256];
    int lastSlash = -1;
    for(int i = 0; midipath[i] != '\0'; i++)
        if(midipath[i] == '\\') lastSlash = i;
    lstrcpyA(dir, midipath);
    dir[lastSlash + 1] = '\0';
    lstrcpyA(base, midipath + lastSlash + 1);
    int baseLen = lstrlenA(base);
    if(baseLen > 4 && base[baseLen-4] == '.')
        base[baseLen - 4] = '\0';
    char folder[256];
    concat(folder, dir, base);
    concat(folder, folder, "_MTS");
    CreateDirectoryA(folder, NULL);

    count = 0;
    LinkedList_foreach(trackPointers, node) {
        TrackPointer* tptr = (TrackPointer*)node->item;
        progressBar((double)count / (double)realTracks, count, realTracks);
        count++;
        if(tracksHaveNotes[count-1] == 0)
            continue;

        Seek(tptr->Position);
        if(TextSearch("MTrk")!=0)
            return 1;
        Skip(4);

        char outpath[256];
        char integer[16];
        ZeroMemory(integer, sizeof(integer));
        u32_to_str(integer, count);

        outpath[0] = '\0';
        lstrcatA(outpath, folder);
        lstrcatA(outpath, "\\");
        lstrcatA(outpath, base);
        lstrcatA(outpath, "_MTSTrack");
        lstrcatA(outpath, integer);
        lstrcatA(outpath, ".mid");

        HANDLE outfile = wfcreate(outpath);

        BufferWrite bw;
        BufferWrite_Init(&bw, outfile, 4 * 1024 * 1024);
        
        BufferWrite_WriteBuffer(&bw, FilePrefix, sizeof(FilePrefix)-1);
        BufferWrite_WriteByte(&bw, (ppq >> 8) & 0xFF);
        BufferWrite_WriteByte(&bw, ppq & 0xFF);
        BufferWrite_WriteBuffer(&bw, TrackPrefix, sizeof(TrackPrefix)-1);

        SynthEvent* synthEnd = s_arr + synthCount;
        SynthEvent* event = s_arr;

        BOOL doNotReadAgain = FALSE;
        uint64_t totalPos = 0;
        uint64_t curPos = 0;
        u_char tempPrev = 0;
        BOOL doloop = TRUE;
        while(doloop)
        {
            if(!doNotReadAgain)
                curPos = ReadVLQ();
            if(event != synthEnd)
            {
                uint64_t sTick = event->tick - totalPos;
                if(sTick < curPos || (sTick == curPos && event->track <= count))
                {
                    BufferWrite_WriteVLQ(&bw, sTick);
                    totalPos += sTick;
                    curPos -= sTick;
                    BufferWrite_WriteBuffer(&bw, event->allocated_data, event->size_data);
                    event++;
                    doNotReadAgain = TRUE;
                    continue;
                } else {
                    doNotReadAgain = FALSE;
                }
            } else {
                doNotReadAgain = FALSE;
            }
            totalPos += curPos;

            u_char readEvent = ReadFastInline();
            if(readEvent < 0x80)
            {
                pos--;
                readEvent = tempPrev;
            }
            tempPrev = readEvent;
            uint32_t trackEvent = readEvent & 0b11110000;
            if (trackEvent == 0x90 || trackEvent == 0x80)
            {
                BufferWrite_WriteVLQ(&bw, curPos);
                BufferWrite_WriteByte(&bw, readEvent);
                uint8_t noteData[2];
                noteData[0] = ReadFastInline();
                noteData[1] = ReadFastInline();
                BufferWrite_WriteBuffer(&bw, noteData, 2);
            } else if(trackEvent == 0xA0 || trackEvent == 0xE0 || trackEvent == 0xB0)
                Skip(2);
            else if(trackEvent == 0xC0 || trackEvent == 0xD0)
                Skip(1);
            else if(trackEvent == 0)
                break;
            else
            {
                switch (readEvent)
                {
                    case 0b11110000:
                        while ((ReadFastInline()) != 0b11110111) { }
                        break;
                    case 0b11110010:
                        Skip(2);
                        break;
                    case 0b11110011:
                        Skip(1);
                        break;
                    case 0xFF:
                        {
                            readEvent = ReadFastInline();
                            switch(readEvent)
                            {
                                case 0x51:
                                    Skip(4);
                                    break;
                                case 0x2F:
                                    doloop = FALSE;
                                    break;
                                default:
                                    Skip(ReadVLQ());
                                    break;
                            }
                            break;
                        }
                    default:
                        print("Warning! Unhandled byte\n");
                        break;
                }
            }
        }

        /* Append the last events if you want :shrug:
        while(event != synthEnd)
        {
            BufferWrite_WriteVLQ(&bw, event->tick - totalPos);
            totalPos = event->tick;
            BufferWrite_WriteBuffer(&bw, event->allocated_data, event->size_data);
            event++;
        }
        */

        BufferWrite_WriteBuffer(&bw, TrackSuffix, sizeof(TrackSuffix)-1);

        BufferWrite_Flush(&bw);

        uint32_t trackSize = (uint32_t)(wfsize(outfile) - 22);
        wfseeki64(outfile, 18, SEEK_SET);
        uint8_t buf[4];
        buf[0] = (trackSize >> 24) & 0xFF;
        buf[1] = (trackSize >> 16) & 0xFF;
        buf[2] = (trackSize >> 8) & 0xFF;
        buf[3] = trackSize & 0xFF;
        DWORD written;
        WriteFile(outfile, buf, 4, &written, NULL);
    
        BufferWrite_Close(&bw);
        wfclose(outfile);
    }

    print("\n\nTrack splitting complete!");
    exit(0);
}