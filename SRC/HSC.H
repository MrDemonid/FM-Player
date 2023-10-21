#ifndef H_ADPLUG_HSCPLAYER
#define H_ADPLUG_HSCPLAYER

#pragma option -a-

#include "opl.h"
#include "player.h"

class ChscPlayer: public CPlayer
{
  public:
    ChscPlayer();
    virtual ~ChscPlayer();

    virtual bool load(char *filename);
    virtual bool update();
    virtual bool update_spd();
    virtual void rewind(int subsong);

    virtual char *gettype() { return "HSC Adlib Composer / HSC-Tracker"; }
    virtual unsigned int getpatterns();
    virtual unsigned int getpattern() { return song[songpos]; }
    virtual unsigned int getorders();
    virtual unsigned int getorder() { return songpos; }
    virtual unsigned int getrow() { return pattpos; }
    virtual unsigned int getspeed() { return speed; }
    virtual unsigned int getinstruments();

  protected:
    struct hscnote                      // note type in HSC pattern
    {
        unsigned char note;
        unsigned char effect;
    };

    struct hscchan                      // HSC channel data
    {
        unsigned char inst;             // current instrument
        signed char   slide;            // used for manual slide-effects
        unsigned int  freq;             // actual replaying frequency
    };
    unsigned char instr[128][12];       // instrument data
    hscchan       channel[9];           // player channel-info
    hscnote       patterns[50][64*9];   // pattern data
    unsigned char song[0x80];           // song-arrangement (MPU-401 Trakker enhanced)
    unsigned char pattpos,songpos;      // various bytes & flags
    unsigned char pattbreak,songend;
    unsigned char mode6,bd,fadein;
    unsigned int  speed,del;
    unsigned char adl_freq[9];          // adlib frequency registers
    int           mtkmode;              // flag: MPU-401 Trakker mode on/off

 private:
  void setfreq(unsigned char chan, unsigned int freq);
  void setvolume(unsigned char chan, int volc, int volm);
  void setinstr(unsigned char chan, unsigned char insnr);
};

#endif
