#include <string.h>

#include "images.h"

#include "images\ibutton.h"
#include "images\ihelp.h"
#include "images\iplayer.h"
#include "images\ifile.h"
#include "images\inotespc.h"
#include "images\inotedot.h"


TXTBMP *imgBtnRel;
TXTBMP *imgBtnPush;
TXTBMP *imgHelp;
TXTBMP *imgPlayer;
TXTBMP *imgFile;
TXTBMP *imgNoteSpc;
TXTBMP *imgNoteDot;


void decode_line(unsigned int *linebuff, unsigned total, unsigned char *src)
{
    unsigned int pos;
    unsigned int color;
    unsigned int symb, attr, count;
    unsigned char *ptr;
    int i;

    pos = 0;
    ptr = src;
    while (pos < total)
    {
        symb = *ptr++;
        attr = *ptr++;
        count = 1;
        if ((attr & 0xC0) == 0xC0)
        {
            count = attr & 0x3F;
            attr = *ptr++;
        }
        color = (attr << 8) | symb;
        for (i = 0; i < count; i++)
        {
            linebuff[pos++] = color;
        }
    }
}

/*
  выделяет память и распаковывает в нее один битмап
*/

TXTBMP *img_UnpackImage(unsigned char *src)
{
    unsigned char *cbuff;
    TXTBMP *bmp;
    unsigned int size;
    unsigned int width, height;

    width = *src++;
    height = *src++;
    size = width*height;
    cbuff = new unsigned char [sizeof(TXTBMP)+size*2];
    bmp = (TXTBMP *) cbuff;
    if (bmp)
    {
        bmp->Width = width;
        bmp->Height = height;
        cbuff = &bmp->Data[0];
        decode_line((unsigned int *) cbuff, size, src);
    }
    return bmp;
}


/*
  распаковывает все битмапы
*/
char img_Unpack(void)
{
    if (((imgBtnRel  = img_UnpackImage(packBtnRel)) == NULL) ||
        ((imgBtnPush = img_UnpackImage(packBtnPush)) == NULL) ||
        ((imgHelp    = img_UnpackImage(packHelp)) == NULL) ||
        ((imgPlayer  = img_UnpackImage(packPlayer)) == NULL) ||
        ((imgFile    = img_UnpackImage(packFile)) == NULL) ||
        ((imgNoteSpc = img_UnpackImage(packNoteSpc)) == NULL) ||
        ((imgNoteDot = img_UnpackImage(packNoteDot)) == NULL))
        return 0;
    return -1;
}

/*
    if ((imgBtnRel  = img_UnpackImage(packBtnRel)) != NULL)
      if ((imgBtnPush = img_UnpackImage(packBtnPush)) != NULL)
        if ((imgHelp    = img_UnpackImage(packHelp)) != NULL)
          if ((imgPlayer  = img_UnpackImage(packPlayer)) != NULL)
            if ((imgFile    = img_UnpackImage(packFile)) != NULL)
              if ((imgNoteSpc = img_UnpackImage(packNoteSpc)) != NULL)
                if ((imgNoteDot = img_UnpackImage(packNoteDot)) != NULL)
                    return -1;
    return 0;
*/
