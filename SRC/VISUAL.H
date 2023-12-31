#ifndef _VISUAL_H_
#define _VISUAL_H_

#ifdef __cplusplus
  extern "C" {
#endif

/*
  �����   (� �ଠ�: ���� x 1000)
*/
#define NORMALIZE        60000L    // Timer frequency to normalize to

#define EQUAL_REFRESH    30000L    // ���� ���������� ����� ���������� (30 fps)
#define COUNTER_REFRESH  16000L    // ���� ���������� ���稪� ����� (16 fps)
#define NOTESPEC_REFRESH 60000L    // ���� ���������� Note spectrum
#define TAPE_REFRESH     10000L    // ���� ���������� ����⭨��



/*
  ������ �����
*/
extern char helpVisible;           // 䫠� ������ ������ �����
/*
  ���稪 ����� �����
*/
extern char cnt_direction;      // ���ࠢ����� ���: 0-㢥��祭��, 1-㬥��襭��
/*
  ���न��� ������� �� Y
*/
extern int panHelpY;
extern int panPlayerY;
extern int panFileY;





/*
  ���樠������ �����
  ��⬠�� 㦥 ������ ���� �ᯠ������
*/
void vis_Initialize(void);

/*
  ��砫� ���㠫���樨 �������
  �� �室�:
    title     - �������� �������
    totaltime - ����� ������� � ��
*/
void vis_Begin(char *title, unsigned long totaltime);
/*
  �����襭�� ���㠫���樨 �������
*/
void vis_Stop(void);
/*
  ����ன�� ���㠫����� �� �������� ����� ���뢠��� � ����� ��모
*/
void vis_SetRate(unsigned long rate);


/*
  ���������� ������ ���㠫쭮� ��� �ந��뢠⥫�
  �� �室�:
    time - �६� �� ��砫� �ந��뢠��� � ��
*/
void vis_Update(unsigned long time);

// ������ ����ᮢ��
void vis_ReDraw(void);

// ���ᮢ�� �६���
void vis_ShowTime(void);

// ���ᮢ�� ������
void vis_PushButton(unsigned char key);
void vis_ReleaseButton(unsigned char key);

/*
  ��४��砥� ���ࠢ����� ���稪� ����� (�� 㢥��祭��/㬥��襭�� �����)
*/
void vis_ToggleDirectCnt(void);

/*
  ��⠭���� ॣ���஢ ⥬�� � �஬����
*/
void vis_SetTempo(unsigned char tempo);
void vis_SetVolume(unsigned char volume);

/*
  �⮡ࠦ���/��뢠�� �ࠢ��
*/
void vis_ToggleHelp(void);

/*
  ��४��砥� NoteSpectrum � NoteDots
*/
void vis_SwitchNotes(void);




unsigned long getTickCount(void);



#ifdef __cplusplus
  }
#endif

#endif
