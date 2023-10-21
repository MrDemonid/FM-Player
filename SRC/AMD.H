#ifndef H_ADPLUG_AMD
#define H_ADPLUG_AMD

#pragma option -a-

#include "opl.h"
#include "player.h"
#include "protrack.h"

class CamdLoader: public CmodPlayer
{
public:
    CamdLoader(): CmodPlayer() { };

    bool load(char *filename);

    char *gettype() { return "AMUSIC Adlib Tracker"; };
    char *gettitle();
    char *getauthor()
        { return &author[0]; };
    unsigned int getinstruments()
        { return 26; };
    char *getinstrument(unsigned int n)
        { return instname[n]; };

  private:
    char songname[24];
    char author[24];
//    char instname[26][23];
};




#endif
