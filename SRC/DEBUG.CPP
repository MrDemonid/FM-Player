#include <stdio.h>
#include <stdarg.h>



void dprintf(const char *fmt, ...)
{
    FILE *log;
    va_list argptr;

    va_start(argptr, fmt);
    log = fopen("C:\\debug.log","at");
    if (!log)
    {
        vfprintf(stderr, fmt, argptr);
    } else {
        vfprintf(log, fmt, argptr);
        fflush(log);
        va_end(argptr);
    }

    if (log)
        fclose(log);
}



/*
//======================================== pattern rows =======================
#include "video.h"

#define VID_HEIGHT   9
#define VID_CWIDTH   6

char noteNam[]  = { 'C','#','D','-','D','#','E','-','F','-','F','#','G','-','G','#','A','-','A','#','B','-','C','-','X','X'};


unsigned char vidbuf[50][VID_CWIDTH*9];


void vid_scroll(void)
{
    int i;

    for (i = VID_HEIGHT-1; i > 0; i--)
        memcpy(&vidbuf[i][0], &vidbuf[i-1][0], VID_CWIDTH*9);
}

void vid_show(char ord)
{
    int i, y;

    char patn[2];

    y = 31 + 2;

    for (i = VID_HEIGHT-1; i >= 0; i--)
    {
        vid_PutText(3, y, VID_CWIDTH*9, &vidbuf[i][0]);
        y++;
    }
    patn[0] = ord + '0';
    patn[1] = 0;
    vid_PutText(VID_CWIDTH*9+10, 31 + 2, 2, &patn[0]);
}


......
    vid_scroll();
......

            if (note < 0x0E) // is note present
            {
                memcpy(&vidbuf[0][realchan*VID_CWIDTH], &noteNam[note*2], 2);
                vidbuf[0][realchan*VID_CWIDTH+2] = pat->rows[row][chan].oct + '0';
                memcpy(&vidbuf[0][realchan*VID_CWIDTH+3], " | ", 3);
            } else {
                // note is empty
                memcpy(&vidbuf[0][realchan*VID_CWIDTH], "    | ", 6);
            }

......
    vid_show(npattern);

*/