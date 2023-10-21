#ifndef H_ADPLUG_S3M
#define H_ADPLUG_S3M

#pragma option -a-

#include "opl.h"
#include "player.h"

    struct s3mheader
    {
        char name[28];                              // song name
        unsigned char kennung,typ,dummy[2];
        unsigned int ordnum,insnum,patnum,flags,cwtv,ffi;
        char scrm[4];
        unsigned char gv,is,it,mv,uc,dp,dummy2[8];
        unsigned int special;
        unsigned char chanset[32];
    };

    struct s3mpatnote
    {
      unsigned char note,oct,instrument,volume,command,info;
    };

    struct s3mpattype
    {
      s3mpatnote rows[64][32];
    };

    struct s3minst
    {
        unsigned char type;
        char filename[15];
        unsigned char d00,d01,d02,d03,d04,d05,d06,d07,d08,d09,d0a,d0b,volume,dsk,dummy[2];
        unsigned long c2spd;
        char dummy2[12], name[28],scri[4];
    };

    struct s3mchan
    {
        unsigned short freq,nextfreq;
        unsigned char oct,vol,inst,fx,info,dualinfo,key,nextoct,trigger,note;
    };


class Cs3mPlayer: public CPlayer
{
  public:
    Cs3mPlayer();
    ~Cs3mPlayer();

    bool load(char *filename);
    bool update();
    bool update_spd();
    void rewind(int subsong);
    char *gettype();
    char *gettitle();
    unsigned int getpatterns()
        { return header.patnum; };
    unsigned int getpattern()
        { return orders[ord]; };
    unsigned int getorders()
        { return (header.ordnum-1); };
    unsigned int getorder()
        { return ord; };
    unsigned int getrow()
        { return crow; };
    unsigned int getspeed()
        { return speed; };
    unsigned int getinstruments()
        { return header.insnum; };
    char *getinstrument(unsigned int n)
        { return &inst[n].name[0]; };

  protected:
    s3minst inst[99];  // 7821 bytes
    s3mpattype *pattern[99];
    // pattern[99][64][32]; // 1 216 512 bytes!!!! (1 pattern = 12288 bytes)
    s3mchan channel[9]; // 126 bytes
    s3mheader header;
    unsigned char orders[256];
    unsigned char crow,ord,speed,tempo,del,songend,loopstart,loopcnt;

    s3mpattype *alloc_pattern();
    void free_patterns();

  private:

    void setvolume(unsigned char chan);
    void setfreq(unsigned char chan);
    void playnote(unsigned char chan);
    void slide_down(unsigned char chan, unsigned char amount);
    void slide_up(unsigned char chan, unsigned char amount);
    void vibrato(unsigned char chan, unsigned char info);
    void tone_portamento(unsigned char chan, unsigned char info);
    void set_tempo(unsigned char newtempo);

    char s3mtype[28];
};


#endif
