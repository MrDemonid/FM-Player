#include <string.h>

#include "fileio.h"
#include "player.h"
#include "protrack.h"
#include "visual.h"

#include "debug.h"


#define JUMPMARKER      0x80    // Orderlist jump marker


// SA2 compatible vibrato rate table
const unsigned int vibratotab[32] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

/*** public methods *************************************/

CmodPlayer::CmodPlayer(): CPlayer(), initspeed(6),
    nop(0), flags(Standard), curchip(opl_getchip()), isarp(0),
    nrows(0), npats(0), nchans(0), length(0), restartpos(0)
{
    memset(activechan, 0xff, sizeof(activechan));
    memset(order, 0, sizeof(order));
    memset(arplist, 0, sizeof(arplist));
    memset(arpcmd, 0, sizeof(arpcmd));
    memset(inst, 0, sizeof(inst));        // reset instruments
    memset(instname, 0, sizeof(instname));
    realloc_patterns(64, 64, 9);
    opl_setnotetable(NOTE_SA2);
}

CmodPlayer::~CmodPlayer()
{
    dealloc_patterns();
}



bool CmodPlayer::update()
{
    unsigned char pattbreak=0, donote, pattnr, chan, oplchan, info1, info2, info, pattern_delay;
    unsigned char tflags = flags;
    unsigned int  track;
    unsigned int row;
    Channel *tchan;

    unsigned char note, tnote, tvol, tport, t_arpcmd;
    unsigned int charppos;
    unsigned int tdiv, tmod;

    if (!speed)            // song full stop
        return FALSE;      // return !songend

    // effect handling (timer dependant)
    tchan = &channel[0];
    for (chan = 0; chan < nchans; chan++)
    {
        oplchan = set_opl_chip(chan);
        //if (arplist && arpcmd && inst[tchan->inst].arpstart) // special arpeggio
        if (isarp && inst[tchan->inst].arpstart) // special arpeggio
        {
            charppos = tchan->arppos;
            t_arpcmd = arpcmd[charppos];

            if (tchan->arpspdcnt)
                tchan->arpspdcnt--;
            else if (t_arpcmd != 255)
            {
                switch (t_arpcmd)
                {
                    case 252: tvol = arplist[charppos]; // set volume
                              if (tvol > 63)
                                  tvol = 63;
                              tchan->vol2 = tchan->vol1 = tvol;
                              setvolume(chan);
                              break;
                    case 253: tchan->key = 0; // release sustaining note
                              setfreq(chan);
                              break;
                    case 254: charppos = tchan->arppos = arplist[charppos]; // arpeggio loop
                              t_arpcmd = arpcmd[charppos];
                              break;
                    default:  if (t_arpcmd)
                              {
                                  tport = op_table[oplchan];
                                  asm {
                                      xor ah, ah
                                      mov al, [t_arpcmd]
                                      mov cx, 10
                                      div cx
                                      mov [tmod], dx
                                      mov [tdiv], ax
                                  }
                                  //tdiv = t_arpcmd / 10;
                                  //tmod = t_arpcmd % 10;
                                  if (tdiv)
                                      opl_write(0xe3 + tport, tdiv - 1);
                                  if (tmod)
                                      opl_write(0xe0 + tport, (tmod) - 1);
                                  if (t_arpcmd < 10)       // ?????
                                      opl_write(0xe0 + tport, t_arpcmd - 1);
                              }
                } // switch

                if (t_arpcmd != 252)
                {
                    tvol = arplist[charppos];
                    if (tvol <= 96)
                        setnote(chan, tchan->note + tvol);
                    if (tvol >= 100)
                        setnote(chan, tvol - 100);
                } else {
                    setnote(chan,tchan->note);
                }
                setfreq(chan);
                if (t_arpcmd != 255)
                {
                    tchan->arppos++;
                    // charppos = tchan->arppos;
                }

                tchan->arpspdcnt = inst[tchan->inst].arpspeed - 1;
            }
        }

        _DL = tflags;
        info2 = _CH = tchan->info2;
        info1 = _AL = tchan->info1;
        if (_DL & Decimal)
            info = _AL * 10 + _CH;
        else
            info = (_AL << 4) + _CH;
        /*
        info1 = tchan->info1;
        info2 = tchan->info2;
        if (flags & Decimal)
            info = info1 * 10 + info2;
        else
            info = (info1 << 4) + info2;
        */
        switch (tchan->fx)
        {
            case 0:  if (info) // arpeggio
                     {
                         if (tchan->trigger < 2)
                             tchan->trigger++;
                         else
                             tchan->trigger = 0;
                         tnote = tchan->note;
                         switch (tchan->trigger)
                         {
                             case 1: tnote += info1; break;
                             case 2: tnote += info2;
                         }
                         setnote(chan, tnote);
                         setfreq(chan);
                     }
                     break;
            case 1:  slide_up(tchan, info);   // slide up
                     setfreq(chan);
                     break;
            case 2:  slide_down(tchan, info); // slide down
                     setfreq(chan);
                     break;
            case 3:  tone_portamento(chan, tchan->portainfo); // tone portamento
                     break;
            case 4:  vibrato(chan,tchan->vibinfo1, tchan->vibinfo2); // vibrato
                     break;
            case 5:      // tone portamento & volume slide
            case 6:  if (tchan->fx == 5)  // vibrato & volume slide
                         tone_portamento(chan, tchan->portainfo);
                     else
                         vibrato(chan, tchan->vibinfo1, tchan->vibinfo2);
            case 10: if (del & 0x03)   // (del % 4) - SA2 volume slide
                         break;
                     if (info1)
                         vol_up(tchan,info1);
                     else
                         vol_down(tchan,info2);
                     setvolume(chan);
                     break;
            case 14: if (info1 == 3)     // retrig note
                         if (!(del % (info2+1)))
                             playnote(chan);
                     break;
            case 16: if (del & 0x03)    // (del % 4) - AMD volume slide
                         break;
                     if (info1)
                         vol_up_alt(tchan,info1);
                     else
                         vol_down_alt(tchan,info2);
                     setvolume(chan);
                     break;
            case 20: if (info < 50)      // RAD volume slide
                         vol_down_alt(tchan,info);
                     else
                         vol_up_alt(tchan,info - 50);
                     setvolume(chan);
                     break;
            case 26: if (info1)          // volume slide
                         vol_up(tchan,info1);
                     else
                         vol_down(tchan,info2);
                     setvolume(chan);
                     break;
            case 28: if (info1)
                     {
                         slide_up(tchan, 1);
                         tchan->info1--;
                     }
                     if (info2)
                     {
                         slide_down(tchan, 1);
                         tchan->info2--;
                     }
                     setfreq(chan);
                     break;
        } // switch
        tchan++;
    } // for

    if (del)         // speed compensation
    {
        del--;
        return !songend;
    }

    // arrangement handling
    if (!resolve_order())
        return !songend;

    pattnr = order[ord];


    // play row
    pattern_delay = 0;
    row = rw;
    tchan = &channel[0];
    for (chan = 0; chan < nchans; chan++)
    {
        oplchan = set_opl_chip(chan);

        if (!activechan[chan] || !(track = trackord[pattnr][chan]))
        {
            tchan++;
            continue;
        }
        track--;

        //tchan = &channel[chan];
        Tracks *ptrack = &tracks[track][row];
        tnote = ptrack->note;

        donote = 0;
        if (ptrack->inst)
        {
            tchan->inst = _CX = ptrack->inst - 1;
            if (!(tflags & Faust))
            {
                tchan->vol1 = 63 - (inst[_CX].data[10] & 63);
                tchan->vol2 = 63 - (inst[_CX].data[9] & 63);
                /*
                tchan->vol1 = 63 - (inst[tchan->inst].data[10] & 63);
                tchan->vol2 = 63 - (inst[tchan->inst].data[9] & 63);
                */
                setvolume(chan);
            }

            /******************************************
              задаем инструмент для панели визуализации
             ******************************************/
            chanInst[oplchan] = instname[tchan->inst];
        }

        if (tnote && ptrack->command != 3) // no tone portamento
        {
            tchan->note = tnote;
            setnote(chan,tnote);

            _DH = inst[tchan->inst].arpstart;
            _CX = tchan->freq;
            _DL = tchan->oct;
            tchan->nextfreq = _CX;
            tchan->nextoct = _DL;
            tchan->arppos = _DH;
            tchan->arpspdcnt = 0;
            if (tnote != 127)        // handle key off
                donote = 1;
            /*
            tchan->nextfreq = tchan->freq;
            tchan->nextoct = tchan->oct;
            tchan->arppos = inst[tchan->inst].arpstart;
            tchan->arpspdcnt = 0;
            if (tnote != 127)        // handle key off
                donote = 1;
            */
        }

        // command handling (row dependant)
        _DH = tflags;
        _DL = ptrack->command;
        _CH = ptrack->param2;
        _CL = ptrack->param1;

        tchan->fx = _DL;
        info2 = tchan->info2 = _CH;
        info1 = tchan->info1 = _AL = _CL;

        if (_DH & Decimal)
            info = _AL * 10 + _CH;
        else
            info = (_AL << 4) + _CH;
        if (donote)
            playnote(chan);

        /*
        tchan->fx = ptrack->command;
        tchan->info1 = ptrack->param1;
        tchan->info2 = ptrack->param2;
        if (donote)
            playnote(chan);

        // command handling (row dependant)
        info1 = tchan->info1;
        info2 = tchan->info2;
        if (tflags & Decimal)
            info = info1 * 10 + info2;
        else
            info = (info1 << 4) + info2;
        */



        /*
          обновляем ноты/октавы и инструменты для канала
        */
        note = tnote;
        if (note)
        {

            if (note >= 0x7F)
            {
                chanOctave[oplchan] |= 0x80;
            } else
            {
                chanOctave[oplchan] = ((note-1) / 12 + 1) & 0x07;
                note = (note-1) % 12;
                if (note > 0)
                    note--;
                else
                    note = 11;
                chanNote[oplchan] = note;
                opl_setnotespc(oplchan, note*8+chanOctave[oplchan]);
            }
        }

        switch (tchan->fx)
        {
            case  3: if (tnote) // tone portamento
                     {
                         if (tnote < 13)
                             tchan->nextfreq = note_table[tnote - 1];
                         else if ((tmod = tnote % 12) > 0)
                             tchan->nextfreq = note_table[(tmod) - 1];
                         else
                             tchan->nextfreq = note_table[11];
                         if (tnote == 127) // handle key off
                         {
                             tchan->nextfreq = tchan->freq;
                             tchan->nextoct = tchan->oct;
                         } else {
                             tchan->nextoct = (tnote - 1) / 12;
                         }
                     }
                     if (info)  // remember vars
                         tchan->portainfo = info;
                     break;

            case  4: if (info)  // vibrato (remember vars)
                     {
                         tchan->vibinfo1 = info1;
                         tchan->vibinfo2 = info2;
                     }
                     break;

            case 7:  set_tempo(info);          // set tempo
                     break;

            case 8:  tchan->key = 0; // release sustaining note
                     setfreq(chan);
                     break;

            case 9:  if (info1)     // set carrier/modulator volume
                         tchan->vol1 = info1 * 7;
                     else
                         tchan->vol2 = info2 * 7;
                     setvolume(chan);
                     break;

            case 11: pattbreak = 1; // position jump
                     rw = 0;
                     if (info <= ord)
                         songend = 1;
                     ord = info;
                     break;

            case 12: if (info > 63)
                         info = 63;
                     tchan->vol1 = info; // set volume
                     tchan->vol2 = info;
                     setvolume(chan);
                     break;

            case 13: if (!pattbreak) // pattern break
                     {
                         pattbreak = 1;
                         rw = info < nrows ? info : 0;  // rw = info;
                         ord++;
                     }
                     break;

            case 14: switch (info1) // extended command
                     {
                         case 0: if (info2)     // define cell-tremolo
                                     regbd |= 128;
                                 else
                                     regbd &= 127;
                                 opl_write(0xbd, regbd);
                                 break;
                         case 1: if (info2)    // define cell-vibrato
                                     regbd |= 64;
                                 else
                                     regbd &= 191;
                                 opl_write(0xbd, regbd);
                                 break;
                         case 4: vol_up_alt(tchan,info2); // increase volume fine
                                 setvolume(chan);
                                 break;
                         case 5: vol_down_alt(tchan,info2); // decrease volume fine
                                 setvolume(chan);
                                 break;
                         case 6: slide_up(tchan,info2); // manual slide up
                                 setfreq(chan);
                                 break;
                         case 7: slide_down(tchan,info2); // manual slide down
                                 setfreq(chan);
                                 break;
                         case 8: pattern_delay = info2 * speed; // pattern delay (rows)
                                 break;
                     }
                     break;

            case 15: if (info <= 0x1f) // SA2 set speed
                         speed = info;
                     if (info >= 0x32)
                         set_tempo(info);
                     if (!info)
                         songend = 1;
                     break;

            case 17:
                     if (info > 63)
                         info = 63;
                     tchan->vol1 = info; // alternate set volume
                     if (inst[tchan->inst].data[0] & 1)
                     {
                         tchan->vol2 = info;
                     }
                     setvolume(chan);
                     break;

            case 18: if(info <= 31 && info > 0) // AMD set speed
                         speed = info;
                     if (info > 31 || !info)
                         set_tempo(info);
                     break;

            case 19: speed = (info ? info : info + 1); // RAD/A2M set speed
                     break;

            case 21: if (info > 63)        // set modulator volume
                         info = 63;
                     tchan->vol2 = info;
                     setvolume(chan);
                     break;

            case 22: if (info > 63)  // set carrier volume
                         info = 63;
                     tchan->vol1 = info;
                     setvolume(chan);
                     break;

            case 23: slide_up(tchan,info); // fine frequency slide up
                     setfreq(chan);
                     break;

            case 24: slide_down(tchan,info); // fine frequency slide down
                     setfreq(chan);
                     break;

            case 25: if (info1 != 0x0f) // set carrier/modulator waveform
                         opl_write(0xe3 + op_table[oplchan],info1);
                     if (info2 != 0x0f)
                         opl_write(0xe0 + op_table[oplchan],info2);
                     break;

            case 27: if (info1) // set chip tremolo/vibrato
                         regbd |= 128;
                     else
                         regbd &= 127;
                     if (info2)
                         regbd |= 64;
                     else
                         regbd &= 191;
                     opl_write(0xbd, regbd);
                     break;

            case 29: pattern_delay = info; // pattern delay (frames)
                     break;
        } // switch (tchan->fx)
        tchan++;
    } // for

    // speed compensation
    del = speed - 1 + pattern_delay;

    if (!pattbreak)           // next row (only if no manual advance)
    {
        rw++;
        if (rw >= nrows)
        {
            rw = 0;
            ord++;
        }
    }

    resolve_order();      // so we can report songend right away
    return !songend;
}


bool CmodPlayer::update_spd()
{
    unsigned char pattbreak=0, pattnr, chan, info1, info2, info, pattern_delay;
    unsigned int  track;
    unsigned int row;
    Tracks *ptrack;

    if (!speed)            // song full stop
        return !songend;
    if (del)         // speed compensation
    {
        del--;
        return !songend;
    }
    // arrangement handling
    if (!resolve_order())
        return !songend;

    // play row
    pattnr = order[ord];
    pattern_delay = 0;
    row = rw;
    for (chan = 0; chan < nchans; chan++)
    {
        if (!activechan[chan] || !(track = trackord[pattnr][chan]))
            continue;

        track--;

        // command handling (row dependant)
        ptrack = &tracks[track][row];
        _DL = flags;
        info2 = _CH = ptrack->param2;
        info1 = _AL = ptrack->param1;

        if (_DL & Decimal)
            info = _AL * 10 + _CH;
        else
            info = (_AL << 4) + _CH;

        /*
        ptrack = &tracks[track][row];
        info1 = ptrack->param1;
        info2 = ptrack->param2;

        if (flags & Decimal)
            info = info1 * 10 + info2;
        else
            info = (info1 << 4) + info2;
        */
        switch (ptrack->command)
        {
            case 7:  set_tempo(info);        // set tempo
                     break;

            case 11: pattbreak = 1; // position jump
                     rw = 0;
                     if (info <= ord)
                         songend = 1;
                     ord = info;
                     break;

            case 13: if (!pattbreak) // pattern break
                     {
                         pattbreak = 1;
                         rw = info < nrows ? info : 0;  // rw = info;
                         ord++;
                     }
                     break;

            case 14: if (info1 == 8) // extended command
                         pattern_delay = info2 * speed; // pattern delay (rows)
                     break;

            case 15: if (info <= 0x1f) // SA2 set speed
                         speed = info;
                     if (info >= 0x32)
                         set_tempo(info);

                     if (!info)
                         songend = 1;
                     break;

            case 18: if(info <= 31 && info > 0) // AMD set speed
                         speed = info;
                     if (info > 31 || !info)
                         set_tempo(info);
                     break;

            case 19: speed = (info ? info : info + 1); // RAD/A2M set speed
                     break;

            case 29: pattern_delay = info; // pattern delay (frames)
                     break;
        } // switch
    } // for

    // speed compensation
    del = speed - 1 + pattern_delay;

    if (!pattbreak)           // next row (only if no manual advance)
    {
        rw++;
        if (rw >= nrows)
        {
            rw = 0;
            ord++;
        }
    }

    resolve_order();      // so we can report songend right away
    return !songend;
}


void CmodPlayer::set_tempo(unsigned int newtempo)
{
    unsigned long tmp;

    tempo = newtempo;
    tmp = tempo * 1000L;
    if (tempo)
        refresh_ms = tmp;                      // = (tempo * 1000000L) / 2500
    else
        refresh_ms = 18206L;
    speed_ms = 1000000000L / refresh_ms;       // 1000000000L / getrefresh();
}


/*
  Sets OPL chip according to channel number. Channels 0-8 are on first chip,
  channels 9-17 are on second chip. Returns corresponding OPL channel
  number.
*/
unsigned char CmodPlayer::set_opl_chip(unsigned char chan)
{
    int newchip = chan < 9 ? 0 : 1;

    if (newchip != curchip)
    {
        opl_setchip(newchip);
        curchip = newchip;
    }

    if (chan < 9)
        return chan;
    else
        return (chan - 9);
}


  /*
   * Resolves current orderlist entry, checking for jumps and loops.
   *
   * Returns true on correct processing, false if immediate recursive loop
   * has been detected.
   */
bool CmodPlayer::resolve_order()
{
    unsigned int neword;

    if (ord < length)
    {
        while (order[ord] >= JUMPMARKER) // jump to order
        {
            neword = order[ord] - JUMPMARKER;
            if (neword <= ord)
                songend = 1;
            if (neword == ord)
                return FALSE;
            ord = neword;
        }
    } else {
        songend = 1;
        ord = restartpos;
    }
    return TRUE;
}


#pragma argsused

void CmodPlayer::rewind(int subsong)
{
    unsigned int i;

    // Reset playing variables
    songend = del = ord = rw = regbd = 0;
    set_tempo(bpm);
    speed = initspeed;

    // Reset channel data
    memset(channel, 0, sizeof(Channel)*nchans);

    // Compute number of patterns, if needed
    if (!nop)
        for (i=0; i<length; i++)
            nop = (order[i] > nop ? order[i] : nop);

    opl_reset();         // Reset OPL chip
    opl_write(1, 32);    // Go to ym3812 mode

    // Enable OPL3 extensions if flagged
    if (flags & Opl3)
    {
        opl_setchip(1);
        opl_write(1, 32);
        opl_write(5, 1);
        opl_setchip(0);
    }

    // Enable tremolo/vibrato depth if flagged
    if (flags & Tremolo)
        regbd |= 128;
    if (flags & Vibrato)
        regbd |= 64;
    if (regbd)
        opl_write(0xbd, regbd);
}
#pragma argsused



void CmodPlayer::init_trackord()
{
    unsigned int i, j, k;


    /*
    for (i=0; i < npats*nchans; i++)
        trackord[i / nchans][i % nchans] = i + 1;
    */
    k = 1;
    for (i = 0; i < npats; i++)
        for (j = 0; j < nchans; j++)
        {
            trackord[i][j] = k;
            k++;
        }
    /*
    */
}



bool CmodPlayer::realloc_patterns(unsigned int pats, unsigned int rows, unsigned int chans)
{
    unsigned int i;

    dealloc_patterns();

    // set new number of tracks, rows and channels
    npats = pats;
    nrows = rows;
    nchans = chans;

    // alloc new patterns
    memset(tracks, 0, sizeof(tracks));
    memset(trackord, 0, sizeof(trackord));
    memset(channel, 0, sizeof(channel));

    for (i=0; i < pats*chans; i++)
    {
        tracks[i] = new Tracks[rows];
        if (tracks[i])
            memset(tracks[i],0 , sizeof(Tracks)*rows);
    }
    return TRUE;
}


void CmodPlayer::dealloc_patterns()
{
    unsigned int i;

    // dealloc everything previously allocated
    if (npats && nrows && nchans)
    {
        for (i=0; i < npats*nchans; i++)
            if (tracks[i])
                delete[] tracks[i];
    }
}




/*** private methods *************************************/

void CmodPlayer::setvolume(unsigned char chan)
{
    unsigned char oplchan = set_opl_chip(chan);
    unsigned char v1, v2;
    Channel *cn = &channel[chan];
    Instrument *ins = &inst[cn->inst];

    _CL = ins->data[10];
    _CH = ins->data[9];
    _DH = cn->vol2;
    _DL = cn->vol1;

    if (flags & Faust)
    {
        asm {
            mov     ax, 0x3F3F
            mov     bx, cx
            and     dx, ax              // DL = cn->vol1 & 63
                                        // DH = cn->vol2 & 63
            and     bx, 0xC0C0          // BL = d10 & 192
                                        // BH = d9 & 192
            sub     ax, dx              // AL = 63 - (cn->vol1 & 63)
                                        // AH = 63 - (cn->vol2 & 63)
            add     ax, cx              // AL += ins->data[10]
                                        // AH += ins->data[9]
            and     ax, 0x3E3F
            shr     ax, 1               // AL = (AL & 63) >> 1
                                        // AH = (AH & 63) >> 1
            add     ax, bx
            mov     v1, al
            mov     v2, ah
        }
        opl_write(0x40 + op_table[oplchan], v2);
        opl_write(0x43 + op_table[oplchan], v1);
        /*
        opl_write(0x40 + op_table[oplchan], (((63 - (cn->vol2 & 63)) + d9 & 63) >> 1) + (d9 & 192));
        opl_write(0x43 + op_table[oplchan], (((63 - (cn->vol1 & 63)) + d10 & 63) >> 1) + (d10 & 192));
        */
    } else {
        asm {
            mov     ax, 0x3F3F
            and     cx, 0xC0C0          // CL = d10 & 192
                                        // CH = d9 & 192
            sub     ax, dx              // AL = 63 - cn->vol1
                                        // AH = 63 - cn->vol2
            add     ax, cx
            mov     v1, al
            mov     v2, ah
        }
        opl_write(0x40 + op_table[oplchan], v2);
        opl_write(0x43 + op_table[oplchan], v1);
        /*
        opl_write(0x40 + op_table[oplchan], 63 - cn->vol2 + (d9 & 192));
        opl_write(0x43 + op_table[oplchan], 63 - cn->vol1 + (d10 & 192));
        */
    }
}

void CmodPlayer::setfreq(unsigned char chan)
{
    unsigned char oplchan = set_opl_chip(chan);
    unsigned char nfreq;
    Channel *cn = &channel[chan];

    _CX = cn->freq;
    _DL = cn->oct;
    _DH = cn->key;
    asm {
        shl     dl, 1
        mov     al, ch          // AL = (freq >> 8)
        shl     dl, 1
        and     al, 3
        add     al, dl          // AL = ((freq >> 8) & 0x03) + (oct << 2)
        or      dh, dh          // if (cn->key)
        jz      doopl
        or      al, 32          //     nfreq |= 32;
    }
    doopl:
    asm {
        mov     nfreq, al
    }
    opl_write(0xa0 + oplchan, _CL);
    opl_write(0xb0 + oplchan, nfreq);

    /*
    unsigned char oplchan = set_opl_chip(chan);
    Channel *cn = &channel[chan];
    unsigned char nfreq = ((cn->freq & 768) >> 8) + (cn->oct << 2);

    opl_write(0xa0 + oplchan, cn->freq & 255);
    if (cn->key)
        opl_write(0xb0 + oplchan, nfreq | 32);
    else
        opl_write(0xb0 + oplchan, nfreq);
    */
}


void CmodPlayer::playnote(unsigned char chan)
{
    unsigned char oplchan = set_opl_chip(chan);
    _SI = op_table[oplchan] + 0x20;
    Channel *cn = &channel[chan];
    Instrument *cinstr = &inst[cn->inst];

    if (!(flags & NoKeyOn))
        opl_write(0xb0 + oplchan, 0);      // stop old note

    // set instrument data
    opl_write(_SI, cinstr->data[1]);
    _SI+= 3;
    opl_write(_SI, cinstr->data[2]);
    _SI+= 0x60-0x23;
    opl_write(_SI, cinstr->data[3]);
    _SI += 3;
    opl_write(_SI, cinstr->data[4]);
    _SI += 0x80-0x63;
    opl_write(_SI, cinstr->data[5]);
    _SI += 3;
    opl_write(_SI, cinstr->data[6]);
    _SI += 0xE0-0x83;
    opl_write(_SI, cinstr->data[7]);
    _SI += 3;
    opl_write(_SI, cinstr->data[8]);
    opl_write(0xc0 + oplchan, cinstr->data[0]);
    opl_write(0xbd, cinstr->misc);   // set misc. register

    // set frequency, volume & play
    cn->key = 1;
    setfreq(chan);

    if (flags & Faust)
    {
        cn->vol1 = cn->vol2 = 63;
    }
    setvolume(chan);
}

void CmodPlayer::setnote(unsigned char chan, int note)
{
    Channel *cn = &channel[chan];

    if (note == 127)
    {
        // key off
        cn->key = 0;
        setfreq(chan);
        return;
    }

    if (note > 96)
        note = 96;
    else if (note < 1)
        note = 1;

  cn->oct = (note - 1) / 12;
  cn->freq = note_table[(note - 1) % 12] + inst[cn->inst].slide; // apply pre-slide
}



void CmodPlayer::tone_portamento(unsigned char chan, unsigned char info)
{
    Channel *cn = &channel[chan];
    unsigned int nextfreq = cn->nextfreq;
    unsigned char nextoct = cn->nextoct;
    unsigned int frqoct = nextfreq + (nextoct << 10);

    if (cn->freq + (cn->oct << 10) < frqoct)
    {
        slide_up(cn, info);
        if (cn->freq + (cn->oct << 10) > frqoct)
        {
            cn->freq = nextfreq;
            cn->oct = nextoct;
        }
    }
    if (cn->freq + (cn->oct << 10) > frqoct)
    {
        slide_down(cn, info);
        if (cn->freq + (cn->oct << 10) < frqoct)
        {
            cn->freq = nextfreq;
            cn->oct = nextoct;
        }
    }
    setfreq(chan);
}

void CmodPlayer::vibrato(unsigned char chan, unsigned char speed, unsigned char depth)
{
    Channel *cn = &channel[chan];
    int i;
    unsigned int tmp;
    signed char trigger;

    if (!speed || !depth)
        return;

    if (depth > 14)
        depth = 14;
    tmp = 16 - depth;

    trigger = cn->trigger;

    for (i = 0;  i < speed; i++)
    {
        _AL = ++trigger;
        while (_AL >= 64)
            _AL -= 64;
        trigger = _AL;
        if (_AL < 16)
        {
            slide_up(cn, vibratotab[_AL + 16] / tmp);
        } else if (_AL < 48)
        {
            slide_down(cn, vibratotab[_AL - 16] / tmp);
        } else {
            // trigger >= 48
            slide_up(cn, vibratotab[_AL - 48] / tmp);
        }
    }
    cn->trigger = trigger;
    setfreq(chan);
}

void CmodPlayer::slide_down(Channel *chan, int amount)
{
    chan->freq -= amount;
    if (chan->freq <= 342)
    {
        if(chan->oct)
        {
            chan->oct--;
            chan->freq <<= 1;
        } else
            chan->freq = 342;
    }
}

void CmodPlayer::slide_up(Channel *chan, int amount)
{
    chan->freq += amount;
    if (chan->freq >= 686)
    {
        if(chan->oct < 7)
        {
            chan->oct++;
            chan->freq >>= 1;
        } else
            chan->freq = 686;
    }
}


void CmodPlayer::vol_up(Channel *chan, int amount)
{
    int v1, v2;
    v1 = chan->vol1 + amount;
    v2 = chan->vol2 + amount;
    if (v1 > 63)
        v1 = 63;
    if (v2 > 63)
        v2 = 63;
    chan->vol1 = v1;
    chan->vol2 = v2;
}

void CmodPlayer::vol_down(Channel *chan, int amount)
{
    int v1, v2;
    v1 = chan->vol1 - amount;
    v2 = chan->vol2 - amount;
    if (v1 < 0)
        v1 = 0;
    if (v2 < 0)
        v2 = 0;
    chan->vol1 = v1;
    chan->vol2 = v2;
}

void CmodPlayer::vol_up_alt(Channel *chan, int amount)
{
    if (chan->vol1 + amount < 63)
        chan->vol1 += amount;
    else
        chan->vol1 = 63;
    if (inst[chan->inst].data[0] & 1)
    {
        if (chan->vol2 + amount < 63)
            chan->vol2 += amount;
        else
            chan->vol2 = 63;
    }
}

void CmodPlayer::vol_down_alt(Channel *chan, int amount)
{
    if (chan->vol1 - amount > 0)
        chan->vol1 -= amount;
    else
        chan->vol1 = 0;
    if (inst[chan->inst].data[0] & 1)
    {
        if(chan->vol2 - amount > 0)
            chan->vol2 -= amount;
        else
            chan->vol2 = 0;
    }
}


/*
*/
/*
*/

/*
*/
