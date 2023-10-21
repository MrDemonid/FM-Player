#ifndef _IMAGES_H_
#define _IMAGES_H_


#include "video.h"


/*
  номера кнопок на панели проигрывателя
*/
#define BTN_PLAY        0
#define BTN_STOP        1
#define BTN_PAUSE       2
#define BTN_PREV        3
#define BTN_NEXT        4
#define BTN_REPLAY      5

#define BTN_NUMKEYS         (BTN_REPLAY+1) // всего шесть кнопок в ряду


/*
  распакованные битмапы панелей и кнопок
*/
extern TXTBMP *imgBtnRel;       // кнопки
extern TXTBMP *imgBtnPush;      // нажатые кнопки

extern TXTBMP *imgHelp;         // панель справки
extern TXTBMP *imgPlayer;       // панель проигрывателя
extern TXTBMP *imgFile;         // панель файлового менеджера
extern TXTBMP *imgNoteSpc;      // панель виз. эффектов
extern TXTBMP *imgNoteDot;      // панель виз. эффектов



/*****************************************************************************
                                   ФУНКЦИИ
 *****************************************************************************/
/*
  распаковывает все битмапы
  на выходе:
      0 - ошибка выделения памяти
*/
char img_Unpack(void);




/****************************************************************************
 ****************************************************************************
                             КООРДИНАТЫ ПАНЕЛЕЙ
 ****************************************************************************
 ****************************************************************************  */

#define IMG_HELP_X      0

#define IMG_PLAYER_X    0

#define IMG_FILE_X      0

#define IMG_NOTESPC_X   0



#endif

