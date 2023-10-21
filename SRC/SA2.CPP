#include <string.h>

#include "fileio.h"
#include "player.h"
#include "protrack.h"
#include "sa2.h"



enum SAT_TYPE
{
    HAS_ARPEGIOLIST = (1 << 7),
    HAS_V7PATTERNS = (1 << 6),
    HAS_ACTIVECHANNELS = (1 << 5),
    HAS_TRACKORDER = (1 << 4),
    HAS_ARPEGIO = (1 << 3),
    HAS_OLDBPM = (1 << 2),
    HAS_OLDPATTERNS = (1 << 1),
    HAS_UNKNOWN127 = (1 << 0)
};


const unsigned char convfx[16] = {0,1,2,3,4,5,6,255,8,255,10,11,12,13,255,15};



bool Csa2Loader::load(char *filename)
{
    int f;
    //insts_t insts;
    unsigned char buf;
    unsigned char tmpbuf[8];
    int i,j, k, notedis = 0;
    unsigned char sat_type;
    unsigned int tmp;
    Tracks *trk;

    CPlayer::load(filename);
    if ((f = io_fopen(filename, O_RDONLY)) == 0)
        return FALSE;

    // read header
    io_fread(&header, 5, f);

    // file validation section
    if (strncmp(header.sadt,"SAdT",4))
    {
        io_fclose(f);
        return FALSE;
    }

    switch (header.version)
    {
        case 1:
            notedis = +0x18;
            sat_type = HAS_UNKNOWN127 | HAS_OLDPATTERNS | HAS_OLDBPM;
            break;
        case 2:
            notedis = +0x18;
            sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
            break;
        case 3:
            notedis = +0x0c;
            sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
            break;
        case 4:
            notedis = +0x0c;
            sat_type = HAS_ARPEGIO | HAS_OLDPATTERNS | HAS_OLDBPM;
            break;
        case 5:
            notedis = +0x0c;
            sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
            break;
        case 6:
            sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
            break;
        case 7:
            sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_V7PATTERNS;
            break;
        case 8:
            sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_TRACKORDER;
            break;
        case 9:
            sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_TRACKORDER | HAS_ACTIVECHANNELS;
            break;
        default:      /* unknown */
            io_fclose(f);
            return FALSE;
    }

    // load section
    // instruments
    for (i = 0; i < 31; i++)
    {
        memset(&inst[i], 0, sizeof(inst[i]));
        if (sat_type & HAS_ARPEGIO)
        {
            io_fread(&inst[i], 15, f);
        } else {
            io_fread(&inst[i], 11, f);
        }
    }

    // instrument names
    for (i = 0; i < 29; i++)
    {
        io_fread(&buf, 1, f);                   // skip byte
        io_fread(instname[i], 16, f);           // f->readString(instname[i], 17);
    }

    io_fseek(f, 3, SEEK_CUR);                   // f->ignore(3);         // dummy bytes

    io_fread(order, 128, f);

    if (sat_type & HAS_UNKNOWN127)
        io_fseek(f, 127, SEEK_CUR);             // f->ignore(127);

    // infos
    io_fread(&nop, 2, f);                       // nop = f->readInt(2);
    io_fread(&length, 1, f);                    // = f->readInt(1);
    io_fread(&restartpos, 1, f);                // = f->readInt(1);

    // checks
    if (nop < 1 || nop > 64 || length < 1 || length > 128 || restartpos >= length)
    {
        io_fclose(f);
        return FALSE;
    }
    // check order
    for (i = 0; i < length; i++)
        if (order[i] >= nop)      /* or 64 */
        {
            io_fclose(f);
            return FALSE;
        }

    // bpm
    io_fread(&bpm, 2, f);                       // = f->readInt(2);
    if (sat_type & HAS_OLDBPM)
    {
        bpm = bpm * 125 / 50;
    }

    if (sat_type & HAS_ARPEGIOLIST)
    {
        isarp = 1;
        io_fread(arplist, SPECIALARPLEN, f);    // arpeggio list
        io_fread(arpcmd, SPECIALARPLEN, f);     // arpeggio commands
    }

    for (i = 0; i < 64; i++)                    // track orders
    {
        for (j = 0; j < 9; j++)
        {
            if (sat_type & HAS_TRACKORDER)
            {
                // the value read should be < 9 * nop, but can't cause invalid accesses
                io_fread(&buf, 1, f);           // f->readInt(1);
                trackord[i][j] = buf;
            } else
                trackord[i][j] = i * 9 + j;
        }
    }

    if (sat_type & HAS_ACTIVECHANNELS)
    {
        memset(activechan, 0, sizeof(activechan));
        io_fread(&tmp, 2, f);
        for (i = 0; i < 16; i++)
        {
            if (tmp & (1 << (15-i)))
                activechan[i] = 0xff;
        }
        //activechan = (unsigned long)tmp << 16;           // active channels
    }

    // track data
    if (sat_type & HAS_OLDPATTERNS)
    {
        i = 0;
        while (i < 64 * 9 && !io_feof(f))
        {
            for (j = 0; j < 64; j++)
            {
                for (k = 0; k < 9; k++)
                {
                    io_fread(tmpbuf, 5, f);
                    trk = &tracks[i+k][j];
                    trk->note = tmpbuf[0] ? (tmpbuf[0] + notedis) : 0;
                    trk->inst = tmpbuf[1];
                    trk->command = convfx[tmpbuf[2] & 0xf];
                    trk->param1 = tmpbuf[3];
                    trk->param2 = tmpbuf[4];
                }
            }
            i += 9;
        }
    } else if(sat_type & HAS_V7PATTERNS) {
        i = 0;
        while (i < 64 * 9 && !io_feof(f))
        {
            for (j = 0; j < 64; j++)
            {
                for (k = 0; k < 9; k++)
                {
                    io_fread(tmpbuf, 3, f);
                    trk = &tracks[i+k][j];
                    trk->note = tmpbuf[0] >> 1;
                    trk->inst = (tmpbuf[0] & 1) << 4;
                    trk->inst += tmpbuf[1] >> 4;
                    trk->command = convfx[tmpbuf[1] & 0x0f];
                    trk->param1 = tmpbuf[2] >> 4;
                    trk->param2 = tmpbuf[2] & 0x0f;
                }
            }
            i+=9;
        }
    } else {
        i = 0;
        while (i < 64 * 9 && !io_feof(f))
        {
            for (j = 0; j < 64; j++)
            {
                io_fread(tmpbuf, 3, f);
                trk = &tracks[i][j];
                trk->note = tmpbuf[0] >> 1;
                trk->inst = (tmpbuf[0] & 1) << 4;
                trk->inst += tmpbuf[1] >> 4;
                trk->command = convfx[tmpbuf[1] & 0x0f];
                trk->param1 = tmpbuf[2] >> 4;
                trk->param2 = tmpbuf[2] & 0x0f;
            }
            i++;
        }
    }
    io_fclose(f);

    // fix instrument names
    char *pinstname;
    for (i = 0; i < 29; i++)
    {
        pinstname = &instname[i][0];
        for (j = 0; j < 17; j++)
            if (!pinstname[j])
                pinstname[j] = ' ';
    }
    rewind(0);            // rewind module
    return TRUE;
}




char *Csa2Loader::gettype()
{
    char ver[2];

    ver[0] = header.version + '0';
    ver[1] = '\0';
    strcpy(sztype, "Surprise! Adlib Tracker 2 (version ");
    strcat(sztype, ver);
    return sztype;
}

char *Csa2Loader::gettitle()
{
    char bufinst[29*17],buf[18];
    int i,ptr;
    char *beg, *end;

    // parse instrument names for song name
    memset(bufinst,'\0', 29*17);
    for (i = 0; i < 29; i++)
    {
        buf[16] = ' ';
        buf[17] = '\0';
        memcpy(buf, &instname[i][1], 16);
        for (ptr = 16; ptr > 0; ptr--)
            if(buf[ptr] == ' ')
                buf[ptr] = '\0';
            else {
                if (ptr<16)
                    buf[ptr+1] = ' ';
                break;
            }
        strcat(bufinst, buf);
    }

    beg = strchr(bufinst,'"');

    if (beg)
    {
        end = strrchr(bufinst,'"');
        if (end > beg)
        {
            *end = '\0';
            strcpy(sztype, beg+1);
        } else
            strcpy(sztype, "");
    } else
        strcpy(sztype, "");

    if (strlen(sztype))
        return sztype;
    else
        return &fname[0];
}


void Csa2Loader::set_tempo(unsigned int newtempo)
{
    unsigned long tmp;

    tempo = newtempo;
    tmp = tempo * 1000000L;
    refresh_ms = tmp / 2500;                   // = (tempo * 1000000L) / 2500
    speed_ms = 1000000000L / refresh_ms;       // 1000000000L / getrefresh();
}
