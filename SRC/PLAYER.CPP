#include <string.h>

#include "opl.h"
#include "player.h"

unsigned long CPlayer::refresh_ms;  // скорость воспроизведения (в Гц * 1000)
unsigned long CPlayer::speed_ms;    // текущее время одного цикла воспроизведения (в мсек * 1000)


CPlayer::CPlayer()
{
    opl_reset();                      // reset data's
    fname[0] = '\0';
}

CPlayer::~CPlayer()
{
}

bool CPlayer::load(char *filename)
{
    char *pos;

    pos = strrchr(filename, '\\');
    if (pos)
    {
        pos++;
    } else {
        pos = filename;
    }
    if (strlen(pos) < sizeof(fname))
    {
        strcpy(fname, pos);
    } else {
        memcpy(fname, pos, sizeof(fname)-1);
        fname[sizeof(fname)-1] = 0;
    }
    return TRUE;
}


void CPlayer::seek(unsigned long ms)
{
    unsigned long pos = 0;

    rewind();
    ms *= 1000;
    while (pos < ms && update())           // seek to new position
        pos += 1000000000L / getrefresh();
}

unsigned long CPlayer::songlength(int subsong)
{
    unsigned long slength;

    opl_setnowrite(TRUE);               // disable write()
    // get song length
    rewind(subsong);
    slength = 0;
    while (update_spd() && (slength < 900000000L)) // song length limit: 15 minutes
        slength += speed_ms;                       // 1000000000L / getrefresh();

    opl_setnowrite(FALSE);              // enable write()
    rewind(subsong);
    return (unsigned long) slength / 1000;
}
