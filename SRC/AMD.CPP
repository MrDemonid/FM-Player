#include <string.h>

#include "fileio.h"
#include "player.h"
#include "protrack.h"
#include "amd.h"



typedef struct
{
    char id[9];
    unsigned char version;
    enum {offset = 1062};
} HEADER;

const unsigned char convfx[10]  = {0,1,2,9,17,11,13,18,3,14};
const unsigned char convvol[64] = {0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03,
                                   0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07,
                                   0x07, 0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0c,
                                   0x0c, 0x0d, 0x0e, 0x0e, 0x0f, 0x10, 0x10, 0x11,
                                   0x12, 0x13, 0x14, 0x14, 0x15, 0x16, 0x17, 0x18,
                                   0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x21,
                                   0x22, 0x23, 0x25, 0x26, 0x28, 0x29, 0x2b, 0x2d,
                                   0x2e, 0x30, 0x32, 0x35, 0x37, 0x3a, 0x3c, 0x3f};


bool CamdLoader::load(char *filename)
{
    int f;
    int i, j, k, t, numtrax, maxi = 0;
    int len;
    unsigned char buf, buf2, buf3;
    unsigned int bufi;
    HEADER header;
    unsigned int vol;
    Tracks *trk;

    CPlayer::load(filename);
    if ((f = io_fopen(filename, O_RDONLY)) == 0)
        return FALSE;

    // file validation section
    if (io_filesize(f) < header.offset + sizeof(header))
    {
        io_fclose(f);
        return FALSE;
    }
    io_fseek(f, header.offset, SEEK_SET);
    io_fread(&header.id, sizeof(header.id), f);
    if(strncmp(header.id, "<o\xefQU\xeeRoR", sizeof(header.id)) && strncmp(header.id, "MaDoKaN96", sizeof(header.id)))
    {
        io_fclose(f);
        return FALSE;
    }
    io_fread(&header.version, sizeof(header.version), f);

    // load section
//    memset(inst, 0, sizeof(*inst));       // лишнее
    io_fseek(f, 0, SEEK_SET);
    // title + author
    io_fread(songname, sizeof(songname), f);
    io_fread(author, sizeof(author), f);
    // instruments
    for (i = 0; i < 26; i++)
    {
        io_fread(instname[i], 23, f);
        io_fread(&inst[i].data, 11, f);
    }
    // order length + # of patterns
    io_fread(&length, 1, f);
    io_fread(&nop, 1, f);
    nop++;
    if (length > 128 || nop > 64)
    {
        io_fclose(f);
        return FALSE;
    }

    // order list
    io_fread(order, 128, f);
    // invalid pattern number in order list?
    for (i = 0; i < length; i++)
        if ((order[i] & 0x7f) >= 64)
        {
            // should be < nop
            io_fclose(f);
            return FALSE;
        }

    io_fseek(f, 10, SEEK_CUR);          // f->ignore(10);



    // track data
    if (header.version == 0x10)
    {
        // unpacked module
        init_trackord();
        maxi = nop * 9;

        for (t = 0; t < maxi && !io_feof(f); t += 9)
        {
            for (j = 0; j < 64; j++)
                for (i = t; i < t+9; i++)
                {
                    trk = &tracks[i][j];
                    io_fread(&buf, 1, f);
                    buf = buf & 0x7f;
                    trk->param1 = buf / 10;
                    trk->param2 = buf % 10;

                    io_fread(&buf, 1, f);
                    trk->command = buf & 0x0f;
                    trk->inst = buf >> 4;

                    io_fread(&buf, 1, f);
                    trk->inst += (buf & 1) << 4;
                    if (buf >> 4)  // fix bug in AMD save routine
                        trk->note = ((buf & 0x0e) >> 1) * 12 + (buf >> 4);
                    else
                        trk->note = 0;
                }
        }
    } else {
        // packed module
        for (i = 0; i < nop; i++)
            for (j = 0; j < 9; j++)
            {
                io_fread(&bufi, 2, f);
                bufi++;
                if (bufi > 64 * 9)
                    bufi = 0; // or fail?
                trackord[i][j] = bufi;
            }
        io_fread(&numtrax, 2, f);    // numtrax = f->readInt(2);
        for (k = 0; k < numtrax; k++)
        {
            io_fread(&bufi, 2, f);       // i = f->readInt(2);
            i = min(bufi, (64 * 9 - 1)); // fix corrupted modules
            maxi = max(i + 1, maxi);

            for (int j = 0; j < 64; j++)
            {
                io_fread(&buf, 1, f);  // buf = f->readInt(1);

                if (buf & 0x80)
                {
                    // packed block of empty events
                    len = min(buf & 0x7f, 64 - j);
                    memset(&tracks[i][j], 0, len * sizeof(tracks[i][j]));
                    j += len - 1; // adjust for increment in loop header
                    continue;
                }

                // normal data, same as above
                trk = &tracks[i][j];
                trk->param1 = buf / 10;
                trk->param2 = buf % 10;

                io_fread(&buf, 1, f);     // buf = f->readInt(1);
                trk->command = buf & 0x0f;
                trk->inst = buf >> 4;

                io_fread(&buf, 1, f);      // buf = f->readInt(1);
                trk->inst += (buf & 1) << 4;
                if(buf >> 4)    // fix bug in AMD save routine
                    trk->note = ((buf & 0x0e) >> 1) * 12 + (buf >> 4);
                else
                    trk->note = 0;
            }
        }
    }
    io_fclose(f);


    // convert to protracker replay data
    bpm = 50;
    restartpos = 0;
    flags = Decimal;
    for (i = 0; i < 26; i++)
    {
        Instrument *pinst = &inst[i];
        char *pname = &instname[i][0];
        // convert instruments
        buf = pinst->data[0];
        buf2 = pinst->data[1];
        pinst->data[0] = pinst->data[10];
        pinst->data[1] = buf;
        buf = pinst->data[2];
        pinst->data[2] = pinst->data[5];
        buf3 = pinst->data[3];
        pinst->data[3] = buf;
        buf = pinst->data[4];
        pinst->data[4] = pinst->data[7];
        pinst->data[5] = buf3;
        buf3 = pinst->data[6];
        pinst->data[6] = pinst->data[8];
        pinst->data[7] = buf;
        pinst->data[8] = pinst->data[9];
        pinst->data[9] = buf2;
        pinst->data[10] = buf3;
        for (j = 0; j < 23; j++)   // convert names
            if (pname[j] == '\xff')
                pname[j] = '\x20';
    }
    // convert patterns to protracker replay data
    for (i = 0; i < maxi; i++)
        // convert patterns
        for (j = 0; j < 64; j++)
        {
            trk = &tracks[i][j];
            if (trk->command < sizeof(convfx))
                trk->command = convfx[trk->command];
            else
                trk->command = 0; // ignore invalid commands

            // extended command
            if (trk->command == 14)
            {
                if (trk->param1 == 2)
                {
                    trk->command = 10;
                    trk->param1 = trk->param2;
                    trk->param2 = 0;
                }
                if (trk->param1 == 3)
                {
                    trk->command = 10;
                    trk->param1 = 0;
                }
            }

            // fix volume
            if (trk->command == 17)
            {
                vol = trk->param1 * 10 + trk->param2;

                if (vol < sizeof(convvol))
                    vol = convvol[vol];
                else
                    vol = 63;

                trk->param1 = vol / 10;
                trk->param2 = vol % 10;
            }
        }

    rewind(0);
    return TRUE;
}

/*
//////////////////////////////////////////////////////
    char dmpfile[256];
    char *ext;
    strcpy(dmpfile, filename);
    ext = strrchr(dmpfile, '.');
    if (!ext)
    {
        strcat(dmpfile, ".dmp");

    } else {
        strcpy(ext, ".dmp");
    }
    f = io_fcreate(dmpfile, C_NORMAL);
    for (i = 0; i < MAX_INSTRUMENTS; i++)
    {
        io_write(instname[i], MAX_INST_NAME, f);
        io_write(&inst[i].data, 11, f);
    }

    for (i = 0; i < 64*9; i++)
        for (j = 0; j < 64; j++)
        {
            io_write(&tracks[i][j].note, 1, f);
            io_write(&tracks[i][j].command, 1, f);
            io_write(&tracks[i][j].inst, 1, f);
            io_write(&tracks[i][j].param2, 1, f);
            io_write(&tracks[i][j].param1, 1, f);
        }
    io_fclose(f);
    return FALSE;
//////////////////////////////////////////////////////
*/





char *CamdLoader::gettitle()
{
    if (strlen(songname))
        return &songname[0];
    else
        return &fname[0];
}

