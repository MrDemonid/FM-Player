#ifndef _IMAGES_H_
#define _IMAGES_H_


#include "video.h"


/*
  ����� ������ �� ������ �ந��뢠⥫�
*/
#define BTN_PLAY        0
#define BTN_STOP        1
#define BTN_PAUSE       2
#define BTN_PREV        3
#define BTN_NEXT        4
#define BTN_REPLAY      5

#define BTN_NUMKEYS         (BTN_REPLAY+1) // �ᥣ� ���� ������ � ���


/*
  �ᯠ������� ��⬠�� ������� � ������
*/
extern TXTBMP *imgBtnRel;       // ������
extern TXTBMP *imgBtnPush;      // ������ ������

extern TXTBMP *imgHelp;         // ������ �ࠢ��
extern TXTBMP *imgPlayer;       // ������ �ந��뢠⥫�
extern TXTBMP *imgFile;         // ������ 䠩������ ��������
extern TXTBMP *imgNoteSpc;      // ������ ���. ��䥪⮢
extern TXTBMP *imgNoteDot;      // ������ ���. ��䥪⮢



/*****************************************************************************
                                   �������
 *****************************************************************************/
/*
  �ᯠ���뢠�� �� ��⬠��
  �� ��室�:
      0 - �訡�� �뤥����� �����
*/
char img_Unpack(void);




/****************************************************************************
 ****************************************************************************
                             ���������� �������
 ****************************************************************************
 ****************************************************************************  */

#define IMG_HELP_X      0

#define IMG_PLAYER_X    0

#define IMG_FILE_X      0

#define IMG_NOTESPC_X   0



#endif

