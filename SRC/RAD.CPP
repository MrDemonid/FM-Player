#include <string.h>

#include "fileio.h"
#include "player.h"
#include "protrack.h"
#include "rad.h"


bool CradLoader::load(char *filename)
{

    int f;
    char id[16];
    unsigned char insbuf[16];
    unsigned char buf,ch,b,inp;
    char bufstr[2] = " \0";
    unsigned int i,j;
    unsigned int patofs[32];
    const unsigned char convfx[16] = {255,1,2,3,255,5,255,255,255,255,20,255,17,0xd,255,19};

    Tracks *t;
    Instrument *it;

    CPlayer::load(filename);
    if ((f = io_fopen(filename, O_RDONLY)) == 0)
        return FALSE;
    // file validation section
    io_fread(&id, 16, f);     // f->readString(id, 16);
    io_fread(&version, 1, f); // f->readInt(1);

    if (strncmp(id, "RAD by REALiTY!!", 16) || version != 0x10)
    {
        io_fclose(f);
        return FALSE;
    }

    // load section
    io_fread(&radflags, 1, f);        // f->readInt(1);

    memset(desc, 0, sizeof(desc));
    if (radflags & 128)              // description
    {
        io_fread(&buf, 1, f);
        while (buf)
        {
            if (buf >= 2 && buf <= 0x1f)
            {
                for (i=0; i<buf; i++)
                    strcat(desc, " ");
            } else {
                if (buf == 1)
                    buf = '\n';
                bufstr[0] = buf;
                strcat(desc, bufstr);
            }
            io_fread(&buf, 1, f);
        }
    }

    io_fread(&buf, 1, f);
    while (buf)                      // instruments
    {
        buf--;
        it = &inst[buf];
        io_fread(insbuf, 11, f);
        it->data[2] = insbuf[0];
        it->data[1] = insbuf[1];
        it->data[10] = insbuf[2];
        it->data[9] = insbuf[3];
        it->data[4] = insbuf[4];
        it->data[3] = insbuf[5];
        it->data[6] = insbuf[6];
        it->data[5] = insbuf[7];
        it->data[0] = insbuf[8];
        it->data[8] = insbuf[9];
        it->data[7] = insbuf[10];
        io_fread(&buf, 1, f);
    }
    length = 0;
    io_fread(&length, 1, f);                       // f->readInt(1);
    io_fread(order, length, f);      // orderlist
    io_fread(&patofs, 32 * 2, f);
    init_trackord();                 // patterns
    for (i=0; i < 32; i++)
    {
        if (patofs[i])
        {
            io_fseek(f, patofs[i], SEEK_SET);      // f->seek(patofs[i]);
            do
            {
                io_fread(&buf, 1, f);              // f->readInt(1);
                b = buf & 127;
                do
                {
                    io_fread(&ch, 1, f);           // f->readInt(1);
                    io_fread(&inp, 1, f);          // f->readInt(1);
                    t = &tracks[i*9+(ch & 127)][b];
                    t->note = inp & 127;
                    t->inst = (inp & 128) >> 3;
                    io_fread(&inp, 1, f);          // f->readInt(1);
                    t->inst += inp >> 4;
                    t->command = inp & 15;
                    if (inp & 15)
                    {
                        io_fread(&inp, 1, f);      // f->readInt(1);
                        t->param1 = inp / 10;
                        t->param2 = inp % 10;
                    }
                } while (!(ch & 128));
            } while (!(buf & 128));
        } else
            memset(trackord[i],0,9*2);
    }
    io_fclose(f);

    // convert replay data
    for (i=0; i < 32*9; i++)   // convert patterns
        for (j=0; j < 64; j++)
        {
            t = &tracks[i][j];
            if (t->note == 15)
                t->note = 127;
            if (t->note > 16 && t->note < 127)
                t->note -= 4 * (t->note >> 4);
            if (t->note && t->note < 126)
                t->note++;
            t->command = convfx[t->command];
        }
    restartpos = 0;
    initspeed = radflags & 31;
    bpm = radflags & 64 ? 0 : 50;
    flags = Decimal;
    rewind(0);
    return TRUE;
}

