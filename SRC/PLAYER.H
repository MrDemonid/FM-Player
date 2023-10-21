#ifndef H_ADPLUG_PLAYER
#define H_ADPLUG_PLAYER

#include "opl.h"

#ifndef max
  #define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
  #define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif


class CPlayer {
public:
    CPlayer();
    virtual ~CPlayer();

  /***** Operational methods *****/
    void seek(unsigned long ms);
    virtual bool load(char *filename);      // loads file
    virtual bool update() = 0;              // executes replay code for 1 tick
    virtual bool update_spd() = 0;          // executes replay code for 1 tick
    virtual void rewind(int subsong = -1)=0;// rewinds to specified subsong
    virtual unsigned long getrefresh()      // returns needed timer refresh rate in fixed format (18200 equ 18.2)
        { return refresh_ms; }

  /***** Informational methods *****/
    virtual unsigned long songlength(int subsong);  // song length in ms
    virtual char *gettype()                 // returns file type
        { return ""; }
    virtual char *gettitle()                // returns song title
        { return &fname[0]; }
    virtual char *getauthor()               // returns song author name
        { return ""; }
    virtual char *getdesc()                 // returns song description
        { return ""; }
    virtual unsigned int getpatterns()      // returns number of patterns
        { return 0; }
    virtual unsigned int getpattern()       // returns currently playing pattern
        { return 0; }
    virtual unsigned int getorders()        // returns size of orderlist
        { return 0; }
    virtual unsigned int getorder()         // returns currently playing song position
        { return 0; }
    virtual unsigned int getrow()           // returns currently playing row
        { return 0; }
    virtual unsigned int getspeed()         // returns current song speed
        { return 0; }
    virtual unsigned int getsubsongs()      // returns number of subsongs
        { return 1; }
    virtual unsigned int getsubsong()       // returns current subsong
        { return 0; }
    virtual unsigned int getinstruments()   // returns number of instruments
        { return 0; }
    virtual char *getinstrument(unsigned int n)  // returns n-th instrument name
        { return ""; }

    char fname[14];
    static unsigned long refresh_ms;  // скорость воспроизведения (в Гц * 1000)
    static unsigned long speed_ms;    // текущее время одного цикла воспроизведения (в мсек * 1000)
};


#endif
