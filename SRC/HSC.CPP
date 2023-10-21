#include <string.h>

#include "fileio.h"
#include "player.h"
#include "hsc.h"
#include "visual.h"


#define PATTERN_SIZE                    (64*9*sizeof(hscnote))

ChscPlayer::ChscPlayer(): CPlayer(), mtkmode(0)
{
    opl_setnotetable(NOTE_STANDART);
    refresh_ms = 18206;
    speed_ms = 1000000000L / 18206;
}

ChscPlayer::~ChscPlayer()
{
}


/*** public methods **************************************/


bool ChscPlayer::load(char *filename)
{
    unsigned long total_patterns;
    int           i;
    int           handle;

    CPlayer::load(filename);
    if ((handle = io_fopen(filename, O_RDONLY)) == 0)
        return FALSE;

    total_patterns = io_filesize(handle);
    if ( (total_patterns > 59187+1) || (total_patterns < 1587+PATTERN_SIZE) )
    {
        io_fclose(handle);
        return FALSE;
    }
    total_patterns = (total_patterns - 1587) / (PATTERN_SIZE);

    // load instruments
    io_fread(&instr, 128*12, handle);

    // correct instruments
    for (i = 0; i < 128; i++)
    {
        instr[i][2] ^= (instr[i][2] & 0x40) << 1;
        instr[i][3] ^= (instr[i][3] & 0x40) << 1;
        instr[i][11] >>= 4;         // slide
    }

    // load tracklist
    io_fread(song, 51, handle);

    // correct tracklist
    for(i=0;i<51;i++)
    {
        // if out of range, song ends here
        if ( ((song[i] & 0x7F) >= total_patterns) || ((song[i] & 0x7F) > 0x31) )
            song[i] = 0xFF;
    }

    // load patterns
    for (i = 0; i < 50; i++)
        io_fread(patterns[i], PATTERN_SIZE, handle);

    io_fclose(handle);
    rewind(0);                                      // rewind module
    return TRUE;
}






bool ChscPlayer::update()
{
    // general vars
    unsigned char  chan,pattnr,note,effect,eff_op,inst,vol,Okt,db;
    unsigned int Fnr;
    unsigned long pattoff;

    del--;                                          // player speed handling
    if (del)
        return !songend;                            // nothing done

    if (fadein)                                     // fade-in handling
        fadein--;

    pattnr = song[songpos];
    // 0xff indicates song end, but this prevents a crash for some songs that
    // use other weird values, like 0xbf
    if (pattnr >= 0xB2)                             // arrangement handling
    {
        songend = 1;                                // set end-flag
        songpos = 0;
        pattnr = song[songpos];
    } else
        if ((pattnr & 128) && (pattnr <= 0xb1))     // goto pattern "nr"
        {
            songpos = song[songpos] & 127;
            pattpos = 0;
            pattnr = song[songpos];
            songend = 1;
        }

    pattoff = pattpos*9;
    for (chan=0; chan<9; chan++)                    // handle all channels
    {
        note = patterns[pattnr][pattoff].note;
        effect = patterns[pattnr][pattoff].effect;
        pattoff++;

        if (note & 0x80)                            // set instrument
        {
            setinstr(chan,effect);
            continue;
        }
        eff_op = effect & 0x0f;
        inst = channel[chan].inst;
        if (note)
        {
            channel[chan].slide = 0;

            if (note >= 0x7F)
                chanOctave[chan] |= 0x80;
            else {
                chanOctave[chan] = ((note-1) / 12) & 0x07;
                chanNote[chan] = (note-1) % 12;
                opl_setnotespc(chan, chanNote[chan]*8+chanOctave[chan]);
            }
        }

        // effect handling
        switch (effect & 0xf0)
        {
            case 0x00:
               /* The following fx are unimplemented on purpose:
                * 02 - Slide Mainvolume up
                * 03 - Slide Mainvolume down (here: fade in)
                * 04 - Set Mainvolume to 0
                *
                * This is because i've never seen any HSC modules using the fx this way.
                * All modules use the fx the way, i've implemented it.
                */
                switch (eff_op)                 // global effect
                {
                    case 1: pattbreak++; break; // jump to next pattern
                    case 3: fadein = 31; break; // fade in (divided by 2)
                    case 5: mode6 = 1;   break; // 6 voice mode on
                    case 6: mode6 = 0;   break; // 6 voice mode off
                }
                break;

            case 0x20:
            case 0x10:                          // manual slides
                if (effect & 0x10)
                {
                    channel[chan].freq += eff_op;
                    channel[chan].slide += eff_op;
                } else {
                    channel[chan].freq -= eff_op;
                    channel[chan].slide -= eff_op;
                }
                if (!note)
                    setfreq(chan,channel[chan].freq);
                break;

            case 0x50:                          // set percussion instrument (unimplemented)
                break;

            case 0x60:                          // set feedback
                opl_write(0xc0 + chan, (instr[channel[chan].inst][8] & 1) + (eff_op << 1));
                break;

            case 0xa0:                          // set carrier volume
                vol = eff_op << 2;
                opl_write(0x43 + op_table[chan], vol | (instr[channel[chan].inst][2] & ~63));
                break;

            case 0xb0:                          // set modulator volume
                vol = eff_op << 2;
                if (instr[inst][8] & 1)
                    opl_write(0x40 + op_table[chan], vol | (instr[channel[chan].inst][3] & ~63));
                else
                    opl_write(0x40 + op_table[chan],vol | (instr[inst][3] & ~63));
                break;

            case 0xc0:                          // set instrument volume
                db = eff_op << 2;
                opl_write(0x43 + op_table[chan], db | (instr[channel[chan].inst][2] & ~63));
                if (instr[inst][8] & 1)
                    opl_write(0x40 + op_table[chan], db | (instr[channel[chan].inst][3] & ~63));
                break;

            case 0xd0:                          // position jump
                pattbreak++;
                songpos = eff_op;
                songend = 1;
                break;

            case 0xf0:                          // set speed
                speed = eff_op;
                del = ++speed;
                break;
        } // switch

        if (fadein)                             // fade-in volume setting
            setvolume(chan, fadein*2, fadein*2);

        if (!note)                              // note handling
            continue;
        note--;

        if ((note == 0x7f-1) || ((note/12) & ~7))   // pause (7fh)
        {
            adl_freq[chan] &= ~32;                  // выкл. ноту
            opl_write(0xb0 + chan,adl_freq[chan]);
            continue;
        }

        // play the note
        if (mtkmode)                                // imitate MPU-401 Trakker bug
            note--;
        Okt = ((note/12) & 7) << 2;
        Fnr = note_table[(note % 12)] + instr[inst][11] + channel[chan].slide;
        channel[chan].freq = Fnr;
        if (!mode6 || chan < 6)
            adl_freq[chan] = Okt | 32;
        else
            adl_freq[chan] = Okt;                   // never set key for drums
        opl_write(0xb0 + chan, 0);
        setfreq(chan, Fnr);
        if (mode6)
        {
            switch (chan)                           // play drums
            {
                case 6:                             // bass drum
                    opl_write(0xbd, bd & ~16);
                    bd |= 48;
                    break;
                case 7:                             // hihat
                    opl_write(0xbd, bd & ~1);
                    bd |= 33;
                    break;
                case 8:                             // cymbal
                    opl_write(0xbd, bd & ~2);
                    bd |= 34;
                    break;
            }
            opl_write(0xbd, bd);
        }
    } // for

    del = speed;                                    // player speed-timing
    if (pattbreak)                                  // do post-effect handling
    {
        pattpos=0;                                  // pattern break!
        pattbreak=0;
        songpos++;
        songpos %= 50;
        if (!songpos)
            songend = 1;
    } else {
        pattpos++;
        pattpos &= 63;                              // advance in pattern data
        if (!pattpos)
        {
            songpos++;
            songpos %= 50;
            if (!songpos)
                songend = 1;
        }
    }
    return !songend;                                // still playing
}


bool ChscPlayer::update_spd()
{
    // general vars
    unsigned char  chan,pattnr,note,effect,eff_op;
    unsigned int pattoff;

    del--;                                          // player speed handling
    if (del)
        return !songend;                            // nothing done

    pattnr = song[songpos];
    if (pattnr >= 0xB2)                             // arrangement handling
    {
        songend = 1;                                // set end-flag
        songpos = 0;
        pattnr = song[songpos];
    } else
        if ((pattnr & 128) && (pattnr <= 0xb1))     // goto pattern "nr"
        {
            songpos = song[songpos] & 127;
            pattpos = 0;
            pattnr = song[songpos];
            songend = 1;
        }

    pattoff = pattpos*9;
    for (chan=0; chan<9; chan++)                    // handle all channels
    {
        note = patterns[pattnr][pattoff].note;
        effect = patterns[pattnr][pattoff].effect;
        pattoff++;

        if (note & 128)                             // set instrument
            continue;

        eff_op = effect & 0x0f;

        switch (effect & 0xf0)                      // effect handling
        {
            case 0x00: if (eff_op == 1)              // global effect
                           pattbreak++;              // jump to next pattern
                    break;
            case 0x20:
            case 0x10:                              // manual slides
            case 0x50:                              // set percussion instrument (unimplemented)
            case 0x60:                              // set feedback
            case 0xa0:                              // set carrier volume
            case 0xb0:                              // set modulator volume
            case 0xc0:                              // set instrument volume
                    break;
            case 0xd0:                              // position jump
                    pattbreak++;
                    songpos = eff_op;
                    songend = 1;
                    break;
            case 0xf0:                              // set speed
                    speed = eff_op;
                    del = ++speed;
                    break;
        } // switch

        if (!note)                                  // note handling
            continue;
        note--;

        if ((note == 0x7f-1) || ((note/12) & ~7))   // pause (7fh)
        {
            continue;
        }

        // play the note
        if (mtkmode)                                // imitate MPU-401 Trakker bug
            note--;
    } // for

    del = speed;                                    // player speed-timing
    if (pattbreak)                                  // do post-effect handling
    {
        pattpos=0;                                  // pattern break!
        pattbreak=0;
        songpos++;
        songpos %= 50;
        if (!songpos)
            songend = 1;
    } else {
        pattpos++;
        pattpos &= 63;                              // advance in pattern data
        if (!pattpos)
        {
            songpos++;
            songpos %= 50;
            if (!songpos)
                songend = 1;
        }
    }
    return !songend;                                // still playing
}


#pragma argsused

void ChscPlayer::rewind(int subsong)
{
    unsigned char i;                                // counter
    // rewind HSC player
    speed = 2;
    pattpos = 0;
    songpos = 0;
    pattbreak = 0;
    del = 1;
    songend = 0;
    mode6 = 0;
    bd = 0;
    fadein = 0;

    opl_reset();                                    // reset OPL chip
    opl_write(1, 32);
    opl_write(8, 128);
    opl_write(0xbd, 0);

    for (i=0; i<9; i++)
        setinstr(i, i);                             // init channels
}
#pragma argsused

unsigned int ChscPlayer::getpatterns()
{
    unsigned char poscnt, pattcnt=0;

    // count patterns
    for (poscnt=0; poscnt<51 && song[poscnt] != 0xff; poscnt++)
        if (song[poscnt] > pattcnt)
            pattcnt = song[poscnt];
    return (pattcnt+1);
}

unsigned int ChscPlayer::getorders()
{
    unsigned char poscnt;

    // count positions
    for (poscnt=0; poscnt<51; poscnt++)
        if (song[poscnt] == 0xff)
            break;
    return poscnt;
}

unsigned int ChscPlayer::getinstruments()
{
    unsigned char instcnt, i, instnum=0;
    bool          isinst;

    // count instruments
    for (instcnt=0; instcnt<128; instcnt++)
    {
        isinst = FALSE;
        for (i=0; i<12; i++)
            if (instr[instcnt][i])
                isinst = TRUE;
        if (isinst)
            instnum++;
    }
    return instnum;
}

/*** private methods *************************************/

void ChscPlayer::setfreq(unsigned char chan, unsigned int freq)
{
    adl_freq[chan] = (adl_freq[chan] & ~3) | (freq >> 8);
    opl_write(0xa0 + chan, freq & 0xff);
    opl_write(0xb0 + chan, adl_freq[chan]);
}

void ChscPlayer::setvolume(unsigned char chan, int volc, int volm)
{
    unsigned char *ins = instr[channel[chan].inst];
    char          op = op_table[chan];

    opl_write(0x43 + op, volc | (ins[2] & ~63));
    if (ins[8] & 1)                                 // carrier
        opl_write(0x40 + op, volm | (ins[3] & ~63));
    else
        opl_write(0x40 + op, ins[3]);               // modulator
}

void ChscPlayer::setinstr(unsigned char chan, unsigned char insnr)
{
    unsigned char *ins = instr[insnr & 0x7F];
    char          op = op_table[chan];

    channel[chan].inst = insnr & 0x7F;  // set internal instrument
    opl_write(0xb0 + chan, 0);          // stop old note

    // set instrument
    opl_write(0xc0 + chan, ins[8]);
    opl_write(0x23 + op, ins[0]);       // carrier
    opl_write(0x20 + op, ins[1]);       // modulator
    opl_write(0x63 + op, ins[4]);       // bits 0..3 = decay; 4..7 = attack
    opl_write(0x60 + op, ins[5]);
    opl_write(0x83 + op, ins[6]);       // 0..3 = release; 4..7 = sustain
    opl_write(0x80 + op, ins[7]);
    opl_write(0xe3 + op, ins[9]);       // bits 0..1 = Wellenform
    opl_write(0xe0 + op, ins[10]);
    setvolume(chan, ins[2] & 63, ins[3] & 63);
}
