#ifndef H_ADPLUG_LDS
#define H_ADPLUG_LDS

#pragma option -a-

#include "opl.h"
#include "player.h"



class CldsPlayer: public CPlayer
{
  public:
    CldsPlayer();
    virtual ~CldsPlayer();

    bool load(char *filename);
    virtual bool update();
    virtual bool update_spd();
    virtual void rewind(int subsong);

    char *gettype() { return "LOUDNESS Sound System"; }
    unsigned int getorders() { return numposi; }
    unsigned int getorder() { return posplay; }
    unsigned int getrow() { return pattplay; }
    unsigned int getspeed() { return speed; }
    unsigned int getinstruments() { return numpatch; }

  private:
    typedef struct
    {
        unsigned char  mod_misc, mod_vol, mod_ad, mod_sr, mod_wave,
                       car_misc, car_vol, car_ad, car_sr, car_wave,
                       feedback, keyoff, portamento, glide, finetune,
                       vibrato, vibdelay, mod_trem, car_trem, tremwait,
                       arpeggio, arp_tab[12];
        unsigned short start, size;
        unsigned char  fms;
        unsigned short transp;
        unsigned char  midinst, midvelo, midkey, midtrans, middum1, middum2;
    } SoundBank;

    typedef struct
    {
        unsigned short gototune, lasttune, packpos;
        unsigned char  finetune, glideto, portspeed, nextvol, volmod, volcar,
                       vibwait, vibspeed, vibrate, trmstay, trmwait, trmspeed,
                       trmrate, trmcount, trcwait, trcspeed, trcrate, trccount,
                       arp_size, arp_speed, keycount, vibcount, arp_pos,
                       arp_count, packwait, arp_tab[12];
        struct
        {
            unsigned char  chandelay, sound;
            unsigned short high;
        } chancheat;
    } Channel;

    typedef struct
    {
        unsigned short patnum;
        unsigned char  transpose;
    } Position;

    //static const unsigned short   frequency[];
    //static const unsigned char    vibtab[], tremtab[];
    //static const unsigned short   maxsound, maxpos;

    SoundBank      *soundbank;
    Channel        channel[9];
    Position       *positions;
    unsigned char  fmchip[0xff], jumping, fadeonoff, allvolume, hardfade,
                   tempo_now, pattplay, tempo, regbd, chandelay[9], mode,
                   pattlen;
    unsigned short posplay, jumppos, *patterns, speed;
    bool           playing, songlooped;
    unsigned int   numpatch, numposi, patterns_size, mainvolume;


    void playsound(int inst_number, int channel_number, int tunehigh);
    void setregs(unsigned char reg, unsigned char val);
    void setregs_adv(unsigned char reg, unsigned char mask, unsigned char val);
    void set_freq(unsigned short chan, unsigned short val);
};

#endif
