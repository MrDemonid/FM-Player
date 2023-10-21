#include <string.h>

#include "fileio.h"
#include "player.h"
#include "s3m.h"
#include "visual.h"

// #include "debug.h"

signed char chnresolv[] =    // S3M -> adlib channel conversion
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,-1,-1,-1,-1,-1,-1,-1};


unsigned char vibratotab[32] =                // vibrato rate table
  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};


unsigned char keyoff[9];


/*** public methods *************************************/


s3mpattype *Cs3mPlayer::alloc_pattern()
{
    int j,k;
    s3mpattype *p;

    p = new s3mpattype;
    if (p)
    {
        memset(p, 255, sizeof(s3mpattype));
        // setup pattern
        for (j = 0; j < 64; j++)
            for (k = 0; k < 32 ;k++)
            {
                p->rows[j][k].instrument = 0;
                p->rows[j][k].info = 0;
            }
    }
    return p;
}

void Cs3mPlayer::free_patterns()
{
    int i;

    for (i = 0; i < 99; i++)
    {
        if (pattern[i])
            delete pattern[i];
    }
}


Cs3mPlayer::Cs3mPlayer(): CPlayer()
{
    memset(pattern, 0, sizeof(pattern));
    memset(orders, 255, sizeof(orders));
    opl_setnotetable(NOTE_S3M);

    memset(keyoff, 1, sizeof(keyoff));
}

Cs3mPlayer::~Cs3mPlayer()
{
    free_patterns();
}


bool Cs3mPlayer::load(char *filename)
{
    int            f;
    unsigned int   insptr[99],pattptr[99];
    int            i,row;
    unsigned char  bufval,bufval2;
    unsigned int   ppatlen;
    unsigned long  pattpos;
    s3mheader      checkhead;
    bool           adlibins=FALSE;
    s3mpattype    *pat;

    CPlayer::load(filename);
    if ((f = io_fopen(filename, O_RDONLY)) == 0)
        return FALSE;

    // load header
    io_fread(&header, sizeof(header), f);
    if (header.kennung != 0x1a || header.typ != 16 || header.patnum > 99 ||
        header.ordnum > 256 || header.insnum > 99 || strncmp(header.scrm,"SCRM",4))
    {
        io_fclose(f);
        return FALSE;
    }
    // load section
    io_fread(&orders, header.ordnum, f);     // read orders
    io_fread(&insptr, header.insnum*2, f);   // instrument offsets
    io_fread(&pattptr, header.patnum*2, f);  // pattern offsets

    // load instruments
    adlibins = 0;
    for (i = 0; i < header.insnum; i++)
    {
        io_fseek(f, insptr[i]*16, SEEK_SET);  // f->seek(insptr[i]*16);
        io_fread(&inst[i], sizeof(s3minst), f);
        if (inst[i].type >= 2)
        {
            adlibins++;
            if (strncmp(inst[i].scri, "SCRI", 4) && strncmp(inst[i].scri, "SCRS", 4))
            {
                io_fclose(f);
                return FALSE;
            }
        }
    }

    if (!adlibins)
    {
        // no adlib instrument found
        io_fclose(f);
        return FALSE;
    }

    for (i = 0; i < header.patnum; i++)
    {
        // depack patterns
        io_fseek(f, pattptr[i]*16, SEEK_SET); // f->seek(pattptr[i]*16);
        io_fread(&ppatlen, 2, f);             // ppatlen = f->readInt(2);
        pattpos = io_fseek(f, 0, SEEK_CUR);   // pattpos = f->pos();
        pat = alloc_pattern();
        if (!pat)
        {
            //free_patterns();
            io_fclose(f);
            return FALSE;
        }
        pattern[i] = pat;

        for (row = 0; (row < 64) && (pattpos-pattptr[i]*16 <= ppatlen); row++)
            do
            {
                io_fread(&bufval, 1, f);      // bufval = f->readInt(1);
                if (bufval & 32)
                {
                    io_fread(&bufval2, 1, f); // bufval2 = f->readInt(1);
                    pat->rows[row][bufval & 31].note = bufval2 & 15;
                    pat->rows[row][bufval & 31].oct = (bufval2 & 240) >> 4;
                    io_fread(&bufval2, 1, f);
                    pat->rows[row][bufval & 31].instrument = bufval2; // pattern[i][row][bufval & 31].instrument = f->readInt(1);
                }
                if (bufval & 64)
                {
                    io_fread(&bufval2, 1, f);
                    pat->rows[row][bufval & 31].volume = bufval2; // pattern[i][row][bufval & 31].volume = f->readInt(1);
                }

                if (bufval & 128)
                {
                    io_fread(&bufval2, 1, f);
                    pat->rows[row][bufval & 31].command = bufval2;   // pattern[i][row][bufval & 31].command = f->readInt(1);
                    io_fread(&bufval2, 1, f);
                    pat->rows[row][bufval & 31].info = bufval2;      // pattern[i][row][bufval & 31].info = f->readInt(1);
                }
            } while (bufval);
    }
    io_fclose(f);
    rewind(0);
    return TRUE;
}




bool Cs3mPlayer::update()
{
    unsigned char pattbreak=0,donote;     // remember vars
    unsigned char pattnr,chan,row,info;   // cache vars
    signed char   realchan;
    s3mpattype   *pat;
    s3mchan      *curchan;
    unsigned char note;


    // effect handling (timer dependant)
    curchan = &channel[0];  // 39 replaced!!!!!!!!!

    for(realchan=0; realchan<9; realchan++)
    {
        info = curchan->info;      // fill infobyte cache
        switch (curchan->fx)
        {
            case 11:
            case 12:
                if (curchan->fx == 11) // dual command: H00 and Dxy
                    vibrato(realchan,curchan->dualinfo);
                else                            // dual command: G00 and Dxy
                    tone_portamento(realchan,curchan->dualinfo);
            case 4:
                if (info <= 0x0f)               // volume slide down
                {
                    if (curchan->vol - info >= 0)
                        curchan->vol -= info;
                    else
                        curchan->vol = 0;
                }
                if ((info & 0x0f) == 0)         // volume slide up
                {
                    if (curchan->vol + (info >> 4) <= 63)
                        curchan->vol += info >> 4;
                    else
                        curchan->vol = 63;
                }
                setvolume(realchan);
                break;
            case 5:                             // slide down
                if (info == 0xf0 || info <= 0xe0)
                {
                    slide_down(realchan,info);
                    setfreq(realchan);
                }
                break;
            case 6:                             // slide up
                if (info == 0xf0 || info <= 0xe0)
                {
                    slide_up(realchan,info);
                    setfreq(realchan);
                }
                break;
            case 7:                             // tone portamento
                tone_portamento(realchan,curchan->dualinfo);
                break;
            case 8:                             // vibrato
                vibrato(realchan,curchan->dualinfo);
                break;
            case 10:                            // arpeggio
                unsigned int tinfo;
                curchan->nextfreq = curchan->freq;
                curchan->nextoct = curchan->oct;
                switch (curchan->trigger)
                {
                    case 0:
                        curchan->freq = note_table[curchan->note];
                        break;
                    case 1:
                        if ((tinfo = curchan->note + ((info & 0xf0) >> 4)) < 12)
                            curchan->freq = note_table[tinfo];
                        else {
                            curchan->freq = note_table[tinfo - 12];
                            curchan->oct++;
                        }
                        break;
                    case 2:
                        if ((tinfo = curchan->note + (info & 0x0f)) < 12)
                            curchan->freq = note_table[tinfo];
                        else {
                            curchan->freq = note_table[tinfo - 12];
                            curchan->oct++;
                        }
                        break;
                }
                if (curchan->trigger < 2)
                    curchan->trigger++;
                else
                    curchan->trigger = 0;
                setfreq(realchan);
                curchan->freq = curchan->nextfreq;
                curchan->oct = curchan->nextoct;
                break;
            case 21:                            // fine vibrato
                vibrato(realchan,(unsigned char) (info / 4));
                break;
        } // switch (curchan->fx)
        curchan++;
    }

    if (del)      // speed compensation
    {
        del--;
        return !songend;
    }

    // arrangement handling
    pattnr = orders[ord];
    if (pattnr == 0xff || ord > header.ordnum) // "--" end of song
    {
        songend = 1;                           // set end-flag
        ord = 0;
        pattnr = orders[ord];
        if (pattnr == 0xff)
            return !songend;
    }
    if (pattnr == 0xfe)                        // "++" skip marker
    {
        ord++;
        pattnr = orders[ord];
    }

    pat = pattern[pattnr];
    // play row
    row = crow;                                // fill row cache
    for (chan = 0; chan < 32; chan++)
    {
        s3mpatnote *prch= &pat->rows[row][chan];

        if (!(header.chanset[chan] & 128))     // resolve S3M -> AdLib channels
            realchan = chnresolv[header.chanset[chan] & 127];
        else
            realchan = -1;            // channel disabled
        if (realchan != -1)           // channel playable?
        {
            curchan = &channel[realchan]; // 44 replaced!!!!!!!!!!
            // set channel values
            note = prch->note;
            register unsigned char command = prch->command;
            register unsigned char fx = curchan->fx;
            donote = 0;
            if (note < 14)
            {
                // tone portamento
                if (command == 7 || command == 12)
                {
                    curchan->nextfreq = note_table[note];
                    curchan->nextoct = prch->oct;
                } else {
                    // normal note
                    curchan->note = note;
                    curchan->freq = note_table[note];
                    curchan->oct = prch->oct;
                    curchan->key = 1;
                    donote = 1;
                }
            }
            if (note == 14)
            {
                // key off (is 14 here, cause note is only first 4 bits)
                curchan->key = 0;
                setfreq(realchan);
            }
            if ((fx != 8 && fx != 11) && (command == 8 || command == 11))
            {
                // vibrato begins
                curchan->nextfreq = curchan->freq;
                curchan->nextoct = curchan->oct;
            }
            if (note >= 14)
                if ((fx == 8 || fx == 11) && (command != 8 && command != 11))
                    {
                        // vibrato ends
                        curchan->freq = curchan->nextfreq;
                        curchan->oct = curchan->nextoct;
                        setfreq(realchan);
                    }
            if (prch->instrument)
            {
                // set instrument
                curchan->inst = prch->instrument - 1;
                if (inst[curchan->inst].volume < 64)
                    curchan->vol = inst[curchan->inst].volume;
                else
                    curchan->vol = 63;
                if (command != 7)
                    donote = 1;
                /******************************************
                  задаем инструмент для панели визуализации
                 ******************************************/
                chanInst[realchan] = &inst[curchan->inst].name[0];
            }
            if (prch->volume != 255)
            {
                if (prch->volume < 64)  // set volume
                    curchan->vol = prch->volume;
                else
                    curchan->vol = 63;
            }
            fx = curchan->fx = command; // set command

            if (prch->info)             // set infobyte
                curchan->info = prch->info;

            // some commands reset the infobyte memory
            _AL = fx;
            if ((_AL >= 1 && _AL <= 3) || (_AL == 20))
            {
                    curchan->info = prch->info;
            }

            // play note
            if (donote)
                playnote(realchan);
            if (prch->volume != 255)      // set volume
                setvolume(realchan);


    //////////////// обновляем буфер нот и октав ////////////////////
            if ((note == 0x0E) || (curchan->key == 0))
            {
                // key off
                chanOctave[realchan] |= 0x80;
                keyoff[realchan] = 0;
            } else if (note < 0x0E)
            {
                // note is present
                /*
                  hsc    C#   D-   D#   E-   F-   F#   G-   G#   A-   A#   B-   C-
                  s3m    C-   C#   D-   D#   E-   F-   F#   G-   G#   A-   A#   B-
                */
                chanOctave[realchan] = prch->oct & 0x07;
                if (note > 0)
                    note--;
                else
                    note = 11;
                chanNote[realchan] = note;
                keyoff[realchan] = tempo / 16;
                opl_setnotespc(realchan, note*8+chanOctave[realchan]);
            } else {
                // "гасим" ноты
                if (keyoff[realchan])
                {
                    keyoff[realchan]--;
                    if (!keyoff[realchan])
                    {
                        chanOctave[realchan] |= 0x80;
                    }


                }
            }
    /////////////////////////////////////////////////////////////////


            // command handling (row dependant)
            info = curchan->info;    // fill infobyte cache
            unsigned char loinfo = info & 0x0f;
            unsigned char hiinfo = info & 0xf0;
            switch (fx)
            {
                case 1:                 // set speed
                    speed = info;
                    break;
                case 2:                 // jump to order
                    if (info <= ord)
                        songend = 1;
                    ord = info;
                    crow = 0;
                    pattbreak = 1;
                    break;
                case 3:                 // pattern break
                    if (!pattbreak)
                    {
                        crow = info;
                        ord++;
                        pattbreak = 1;
                    }
                    break;
                case 4:
                    if(info > 0xf0)
                    {
                        // fine volume down
                        if (curchan->vol - loinfo >= 0)
                            curchan->vol -= loinfo;
                        else
                            curchan->vol = 0;
                    }
                    if (loinfo == 0x0f && info >= 0x1f)
                    {
                        // fine volume up
                        if (curchan->vol + (hiinfo >> 4) <= 63)
                            curchan->vol += hiinfo >> 4;
                        else
                            curchan->vol = 63;
                    }
                    setvolume(realchan);
                    break;
                case 5:
                    if (info > 0xf0)    // fine slide down
                    {
                        slide_down(realchan,loinfo);
                        setfreq(realchan);
                    }
                    if (info > 0xe0 && info < 0xf0)
                    {                   // extra fine slide down
                        slide_down(realchan, loinfo >> 2);
                        setfreq(realchan);
                    }
                    break;
                case 6:
                    if (info > 0xf0)    // fine slide up
                    {
                        slide_up(realchan,(unsigned char) (loinfo));
                        setfreq(realchan);
                    }
                    if (info > 0xe0 && info < 0xf0)
                    {                   // extra fine slide up
                        slide_up(realchan, loinfo >> 2);
                        setfreq(realchan);
                    }
                    break;
                case 7:                                                                                                           // tone portamento
                case 8:                 // vibrato (remember info for dual commands)
                    if ((fx == 7 || fx == 8) && prch->info)
                        curchan->dualinfo = info;
                    break;
                case 10:                // arpeggio (set trigger)
                    curchan->trigger = 0;
                    break;
                case 19:
                    if (info == 0xb0)   // set loop start
                        loopstart = row;
                    if (info > 0xb0 && info <= 0xbf)
                    {
                        // pattern loop
                        if (!loopcnt)
                        {
                            loopcnt = loinfo;
                            crow = loopstart;
                            pattbreak = 1;
                        } else if(--loopcnt > 0) {
                            crow = loopstart;
                            pattbreak = 1;
                        }
                    }
                    if (hiinfo == 0xe0)  // patterndelay
                        del = speed * loinfo - 1;
                    break;
                case 20:                // set tempo
                    set_tempo(info);
                    break;
            }                           // switch (fx)
        }
    }                                   // for (chan = 0; chan < 32; chan++)

    if (!del)
        del = speed - 1;            // speed compensation
    if (!pattbreak)
    {
        // next row (only if no manual advance)
        crow++;
        if (crow > 63)
        {
            crow = 0;
            ord++;
            loopstart = 0;
        }
    }
    return !songend;              // still playing
}


bool Cs3mPlayer::update_spd()
{
    unsigned char pattbreak=0;            // remember vars
    unsigned char pattnr,chan,row,info;   // cache vars
    signed char   realchan;
    s3mpattype   *pat;
    unsigned long tmp;


    if (del)      // speed compensation
    {
        del--;
        return !songend;
    }

    // arrangement handling
    pattnr = orders[ord];
    if (pattnr == 0xff || ord > header.ordnum) // "--" end of song
    {
        songend = 1;                           // set end-flag
        ord = 0;
        pattnr = orders[ord];
        if (pattnr == 0xff)
            return !songend;
    }
    if (pattnr == 0xfe)                        // "++" skip marker
    {
        ord++;
        pattnr = orders[ord];
    }

    pat = pattern[pattnr];
    // play row
    row = crow;                                // fill row cache
    for (chan = 0; chan < 32; chan++)
    {

        if (!(header.chanset[chan] & 128))     // resolve S3M -> AdLib channels
            realchan = chnresolv[header.chanset[chan] & 127];
        else
            realchan = -1;            // channel disabled
        if (realchan != -1)           // channel playable?
        {
            s3mchan    *curchan = &channel[realchan];
            s3mpatnote *prch= &pat->rows[row][chan];

            curchan->fx = prch->command; // set command
            if (prch->info)             // set infobyte
                curchan->info = prch->info;

            // some commands reset the infobyte memory
            switch (curchan->fx)
            {
                case 1:
                case 2:
                case 3:
                case 20:
                    curchan->info = prch->info;
                    break;
            }

            // command handling (row dependant)
            info = curchan->info;    // fill infobyte cache
            switch (curchan->fx)
            {
                case 1:                 // set speed
                    speed = info;
                    break;
                case 2:                 // jump to order
                    if (info <= ord)
                        songend = 1;
                    ord = info;
                    crow = 0;
                    pattbreak = 1;
                    break;
                case 3:                 // pattern break
                    if (!pattbreak)
                    {
                        crow = info;
                        ord++;
                        pattbreak = 1;
                    }
                    break;
                case 19:
                    if (info == 0xb0)   // set loop start
                        loopstart = row;
                    if (info > 0xb0 && info <= 0xbf)
                    {
                        // pattern loop
                        if (!loopcnt)
                        {
                            loopcnt = info & 0x0f;
                            crow = loopstart;
                            pattbreak = 1;
                        } else if(--loopcnt > 0) {
                            crow = loopstart;
                            pattbreak = 1;
                        }
                    }
                    if ((info & 0xf0) == 0xe0)  // patterndelay
                        del = speed * (info & 0x0f) - 1;
                    break;
                case 20:                // set tempo
                    set_tempo(info);
                    break;
            }                           // switch (curchan->fx)
        }
    }                                   // for (chan = 0; chan < 32; chan++)

    if (!del)
        del = speed - 1;            // speed compensation
    if (!pattbreak)
    {
        // next row (only if no manual advance)
        crow++;
        if (crow > 63)
        {
            crow = 0;
            ord++;
            loopstart = 0;
        }
    }
    return !songend;              // still playing
}


void Cs3mPlayer::set_tempo(unsigned char newtempo)
{
    unsigned long tmp;

    tempo = newtempo;
    // предрасчитываем скорость воспроизведения
    tmp = tempo * 1000000L;
    refresh_ms = tmp / 2500;
    speed_ms = 1000000000L / refresh_ms;
}


#pragma argsused

void Cs3mPlayer::rewind(int subsong)
{
    unsigned long tmp;

    // set basic variables
    songend = 0;
    ord = 0;
    crow = 0;
    tempo = header.it;
    speed = header.is;
    del = 0;
    loopstart = 0;
    loopcnt = 0;
    // задаем начальную скорость воспроизведения
    set_tempo(tempo);

    memset(channel,0,sizeof(channel));

    opl_reset();                        // reset OPL chip
    opl_write(1, 32);                   // Go to ym3812 mode
    opl_write(8, 128);
    opl_write(0xbd, 0);
}

#pragma argsused


char *Cs3mPlayer::gettype()
{
    strcpy(s3mtype, "Scream Tracker ");
    switch (header.cwtv)                // determine version number
    {
        case 0x1300: strcat(s3mtype,"3.00"); break;
        case 0x1301: strcat(s3mtype,"3.01"); break;
        case 0x1303: strcat(s3mtype,"3.03"); break;
        case 0x1320: strcat(s3mtype,"3.20"); break;
        default: strcat(s3mtype,"3.??");
    }
    return &s3mtype[0];
}

char *Cs3mPlayer::gettitle()
{
    if (strlen(header.name))
        return header.name;
    else
        return &fname[0];
}


/*** private methods *************************************/



void Cs3mPlayer::setvolume(unsigned char chan)
{
    unsigned char op;
    unsigned int c, m, v;

    s3mchan *cn = &channel[chan];
    s3minst * pinst = &inst[cn->inst];

    op = op_table[chan];
    c = 63 - (pinst->d03 & 63);
    v = ((c * cn->vol) / 63) & 63;
    m = (63 - v) | (pinst->d03 & 192);
    opl_write(0x43 + op, m);
    if (pinst->d0a & 1)
    {
        c = 63 - (pinst->d02 & 63);
        v = ((c * cn->vol) / 63) & 63;
        m = (63 - v) | (pinst->d02 & 192);
        opl_write(0x40 + op, m);
    }
}


void Cs3mPlayer::setfreq(unsigned char chan)
{
    s3mchan *cn = &channel[chan];
    unsigned int tmp = ((cn->freq & 768) >> 8) + (cn->oct << 2);

    opl_write(0xa0 + chan, cn->freq & 255);
    if (cn->key)
        tmp |= 32;

    opl_write(0xb0 + chan, tmp);
}

void Cs3mPlayer::playnote(unsigned char chan)
{
    unsigned char op;
    unsigned char vol, cvol, mvol;
    unsigned int c, v;

    s3mchan *cn = &channel[chan];
    s3minst * pinst = &inst[cn->inst];

    vol = cn->vol;


    c = 63 - (pinst->d03 & 63);
    v = ((c * vol) / 63) & 63;
    cvol = (63 - v) | (pinst->d03 & 192);
    if (pinst->d0a & 1)
    {
        c = 63 - (pinst->d02 & 63);
        v = ((c * vol) / 63) & 63;
        mvol = (63 - v) | (pinst->d02 & 192);
    } else {
        mvol = pinst->d02;
    }

    opl_write(0xb0 + chan, 0);   // stop old note

    op = op_table[chan];
    // set instrument data
    opl_write(0x20 + op, pinst->d00);
    opl_write(0x23 + op, pinst->d01);
    opl_write(0x40 + op, mvol);
    opl_write(0x43 + op, cvol);
    opl_write(0x60 + op, pinst->d04);
    opl_write(0x63 + op, pinst->d05);
    opl_write(0x80 + op, pinst->d06);
    opl_write(0x83 + op, pinst->d07);
    opl_write(0xe0 + op, pinst->d08);
    opl_write(0xe3 + op, pinst->d09);
    opl_write(0xc0 + chan, pinst->d0a);


    // set frequency & play
    cn->key = 1;
    setfreq(chan);
}
/*
    op = op_table[chan];
    insnr = channel[chan].inst;
    c = 63 - (inst[insnr].d03 & 63);
    v = ((c * channel[chan].vol) / 63) & 63;
    m = (63 - v) | (inst[insnr].d03 & 192);
    opl_write(0x43 + op, m);
    if (inst[insnr].d0a & 1)
    {
        c = 63 - (inst[insnr].d02 & 63);
    }
*/


void Cs3mPlayer::slide_down(unsigned char chan, unsigned char amount)
{
    s3mchan *cn = &channel[chan];

    if (cn->freq - amount > 340)
        cn->freq -= amount;
    else
        if (cn->oct > 0)
        {
            cn->oct--;
            cn->freq = 684;
        } else
            cn->freq = 340;
}


void Cs3mPlayer::slide_up(unsigned char chan, unsigned char amount)
{
    s3mchan *cn = &channel[chan];

    if (cn->freq + amount < 686)
        cn->freq += amount;
    else
        if (cn->oct < 7)
        {
            cn->oct++;
            cn->freq = 341;
        } else
            cn->freq = 686;
}


void Cs3mPlayer::vibrato(unsigned char chan, unsigned char info)
{
    unsigned char i,speed;
    unsigned int depth;
    s3mchan *cn = &channel[chan];

    speed = info >> 4;
    depth = 16 - (info & 0x0f) / 2;

    for (i = 0; i < speed; i++)
    {
        cn->trigger++;
        while (cn->trigger >= 64)
            cn->trigger -= 64;
        if (cn->trigger >= 16 && cn->trigger < 48)
            slide_down(chan,(unsigned char) (vibratotab[cn->trigger - 16] / depth));
        if (cn->trigger < 16)
            slide_up(chan,(unsigned char) (vibratotab[cn->trigger + 16] / depth));
        if(cn->trigger >= 48)
            slide_up(chan,(unsigned char) (vibratotab[cn->trigger - 48] / depth));
    }
    setfreq(chan);
}

void Cs3mPlayer::tone_portamento(unsigned char chan, unsigned char info)
{
    s3mchan *cn = &channel[chan];

    if (cn->freq + (cn->oct << 10) < cn->nextfreq + (cn->nextoct << 10))
        slide_up(chan,info);
    if (cn->freq + (cn->oct << 10) > cn->nextfreq + (cn->nextoct << 10))
        slide_down(chan,info);
    setfreq(chan);
}




