#ifndef H_ADPLUG_RAD
#define H_ADPLUG_RAD

#pragma option -a-

#include "opl.h"
#include "player.h"
#include "protrack.h"

class CradLoader: public CmodPlayer
{
  public:
    CradLoader(): CmodPlayer()
        { *desc = '\0'; };

    bool load(char *filename);

    char *gettype()
        { return "Reality ADlib Tracker"; };
    char *getdesc()
        { return &desc[0]; };

private:
    unsigned char version, radflags;
    char desc[80*22];
};


#endif H_ADPLUG_RAD
