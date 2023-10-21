#pragma option -a-

#ifndef _FILE_H_
#define _FILE_H_

#include "fileio.h"

/*
  Типы поддерживаемых файлов
*/
#define FILE_UNKNOWN    0x00
#define FILE_HSC        0x01
#define FILE_RAD        0x02
#define FILE_AMD        0x03
#define FILE_SA2        0x04
#define FILE_S3M        0x05
#define FILE_DMO        0x06
#define FILE_LDS        0x07

//#define FILE_MSC        0x08
//#define FILE_MIDI       0x09


/*
  Список файлов
*/
typedef struct FList
{
    DTA             dta;        //
    char           *dir;        // каталог (не более 66 символов)
    struct FList   *next;       // следующий в списке
    struct FList   *prev;       // предыдущий в списке
} FLIST;



/*
  Менеджер файлов. На выходе список файлов для проигрывания.
*/
int file_Init(void);
FLIST *file_Run(void);


void file_NextCursor(int dir);


/*
  Определяет тип файла
  на входе:  имя файла
  на выходе: тип файла (константы FILE_XXX )
*/
int file_Type(char *fname);


/*
  Вывод строки в панель менеджера файлов
*/
void file_PutString(char *str);


/*
  Возвращает полный путь к текущему каталогу
*/
void file_GetCurDir(char *path);

int file_SetCurDir(char *path);

void file_MakeFullName(char *dir, char *fname, char *res);

#endif
