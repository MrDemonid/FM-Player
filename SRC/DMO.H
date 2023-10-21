#ifndef H_ADPLUG_DMO
#define H_ADPLUG_DMO

#pragma option -a-

#include "opl.h"
#include "player.h"
#include "s3m.h"



class CdmoLoader: public Cs3mPlayer
{
  public:


    CdmoLoader() : Cs3mPlayer()
      { };

    bool load(char *filename);

    char *gettype() { return "TwinTeam (packed S3M)"; };
    char *getauthor() {return "Benjamin GERARDIN";};

  private:

    bool decrypt(unsigned char *buf, long len);
    long unpack(unsigned char *ibuf, unsigned char *obuf, unsigned long outputsize);

    unsigned short brand(unsigned short range);
    short unpack_block(unsigned char *ibuf, long ilen, unsigned char *obuf);

    unsigned long bseed;
    unsigned char *oend;
};

#endif H_ADPLUG_DMO
