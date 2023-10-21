#ifndef H_PROTRACK
#define H_PROTRACK

#pragma option -a-

#include "opl.h"
#include "player.h"


#define MAX_INSTRUMENTS        250      // макс. количество инструментов
#define MAX_INST_NAME           32      // макс. длина имени инструмента
//#define MAX_INST_NAME           34      // макс. длина имени инструмента

#define SPECIALARPLEN          256     // Standard length of special arpeggio lists


class CmodPlayer: public CPlayer
{
  public:
    CmodPlayer();
    virtual ~CmodPlayer();

    virtual bool update();              // executes replay code for 1 tick
    virtual bool update_spd();          // fast executes replay code for 1 tick
    virtual void rewind(int subsong);// rewinds to specified subsong

    virtual unsigned int getpatterns()      // returns number of patterns
        { return nop; }
    virtual unsigned int getpattern()       // returns currently playing pattern
        { return order[ord]; }
    virtual unsigned int getorders()        // returns size of orderlist
        { return length; }
    virtual unsigned int getorder()         // returns currently playing song position
        { return ord; }
    virtual unsigned int getrow()           // returns currently playing row
        { return rw; }
    virtual unsigned int getspeed()         // returns current song speed
        { return speed; }

  protected:
    enum Flags
    {
        Standard = 0,
        Decimal = 1 << 0,
        Faust = 1 << 1,
        NoKeyOn = 1 << 2,
        Opl3 = 1 << 3,
        Tremolo = 1 << 4,
        Vibrato = 1 << 5,
        Percussion = 1 << 6
    };

    struct Instrument
    {
        unsigned char data[11],arpstart,arpspeed,arppos,arpspdcnt,misc;
        signed char slide;
    } inst[MAX_INSTRUMENTS];

    struct Tracks
    {
        unsigned char note,command,inst,param2,param1;
    } *tracks[64*18];

    //unsigned int  trackord[64][18];
    unsigned int  trackord[64][32]; // fasts!
    unsigned char order[128];
    unsigned char isarp;
    unsigned char arplist[SPECIALARPLEN];
    unsigned char arpcmd[SPECIALARPLEN];

    unsigned char initspeed;
    unsigned int tempo, bpm, nop;
    unsigned int length, restartpos; // long to int
    unsigned char activechan[32];  // флаги активности каналов (activechan[0] == chan 0)
    //unsigned long activechan;    // флаги активности каналов (31 bit == chan 0)
    int curchip;
    unsigned char flags;

    struct Channel
    {
        unsigned int freq,nextfreq;
        unsigned char oct,vol1,vol2,inst,fx,info1,info2,key,nextoct;
        unsigned char note,portainfo,vibinfo1,vibinfo2,arppos,arpspdcnt;
        signed char trigger;
    } channel[18];

    char instname[MAX_INSTRUMENTS][MAX_INST_NAME]; // массив имен инструментов


    void init_trackord();
    bool realloc_patterns(unsigned int pats, unsigned int rows, unsigned int chans);

  private:
    //static const unsigned int vibratotab[32];

    unsigned char speed, del, songend, regbd;
    unsigned int rows;
    unsigned int rw, ord, nrows, npats, nchans;  // long to int

    void setvolume(unsigned char chan);
    void setfreq(unsigned char chan);
    void playnote(unsigned char chan);
    void setnote(unsigned char chan, int note);
    void tone_portamento(unsigned char chan, unsigned char info);
    void vibrato(unsigned char chan, unsigned char speed, unsigned char depth);

    void slide_down(Channel *chan, int amount);
    void slide_up(Channel *chan, int amount);
    void vol_up(Channel *chan, int amount);
    void vol_down(Channel *chan, int amount);
    void vol_up_alt(Channel *chan, int amount);
    void vol_down_alt(Channel *chan, int amount);

    void dealloc_patterns();
    bool resolve_order();
    unsigned char set_opl_chip(unsigned char chan);

    virtual void set_tempo(unsigned int newtempo);
};

#endif
