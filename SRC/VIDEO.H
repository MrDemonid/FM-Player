#ifndef _VIDEO_H_
#define _VIDEO_H_

#ifdef __cplusplus
  extern "C" {
#endif


typedef struct {
    int Width;
    int Height;
    unsigned char Data[];
} TXTBMP;




char vid_IsEGA(void);
void vid_SetVideoMode(void);
void vid_RestoreMode(void);

void vid_DosStr(char *str);
char vid_UpChr(char symb);


int vid_ScrWidth(void);
int vid_ScrHeight(void);
unsigned int vid_GetAddress(int x, int y);

void vid_SetFont(char *pFont, unsigned char fntHeight, unsigned char nFirst, unsigned int nChars);
//void vid_GetFont(char *pFont, unsigned char fntHeight, unsigned char nFirst, unsigned int nChars);



/*
  вывод "битмапа"
*/
void vid_PutSprite(TXTBMP *image, int x, int y);

void vid_FillRect(int x, int y, int width, int height, int attr);

/*
  вывод текста в поле заданной длины:
    x,y - координаты поля вывода
    len - длина поля (в символах)
    text - строка
*/
void vid_PutText(int x, int y, int len, char *text);
void vid_GetText(int x, int y, int max, char *text);

//void vid_PutString(int x, int y, char *string);
void vid_XorColor(int x, int y, int len, unsigned char mask);

void vid_ClrInfo(int x, int y);
void vid_PutInfo(int x, int y, char *name, char attr, long size, int date, int time);
void vid_PutTime(int x, int y, long time);

#ifdef __cplusplus
}
#endif

#endif
