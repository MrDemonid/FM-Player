#ifndef H_ADPLUG_SA2
#define H_ADPLUG_SA2

#pragma option -a-

#include "opl.h"
#include "player.h"
#include "protrack.h"


class Csa2Loader: public CmodPlayer
{
  public:
    Csa2Loader(): CmodPlayer() { }

    bool load(char *filename);

    char *gettype();
    char *gettitle();

    unsigned int getinstruments()
        { return 31; }
    char *getinstrument(unsigned int n)
        {
            if (n < 29)
                return instname[n];
            else
                return "-broken-";
        }

  private:
    virtual void set_tempo(unsigned int newtempo);

    struct sa2header
    {
        char sadt[4];
        unsigned char version;
    } header;
//    char instname[29][17];
    char sztype[500];
};

#endif
