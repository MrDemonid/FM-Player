#pragma option -a-

#ifndef _FILE_H_
#define _FILE_H_

#include "fileio.h"

/*
  ���� �����ন������ 䠩���
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
  ���᮪ 䠩���
*/
typedef struct FList
{
    DTA             dta;        //
    char           *dir;        // ��⠫�� (�� ����� 66 ᨬ�����)
    struct FList   *next;       // ᫥���騩 � ᯨ᪥
    struct FList   *prev;       // �।��騩 � ᯨ᪥
} FLIST;



/*
  �������� 䠩���. �� ��室� ᯨ᮪ 䠩��� ��� �ந��뢠���.
*/
int file_Init(void);
FLIST *file_Run(void);


void file_NextCursor(int dir);


/*
  ��।���� ⨯ 䠩��
  �� �室�:  ��� 䠩��
  �� ��室�: ⨯ 䠩�� (����⠭�� FILE_XXX )
*/
int file_Type(char *fname);


/*
  �뢮� ��ப� � ������ �������� 䠩���
*/
void file_PutString(char *str);


/*
  �����頥� ����� ���� � ⥪�饬� ��⠫���
*/
void file_GetCurDir(char *path);

int file_SetCurDir(char *path);

void file_MakeFullName(char *dir, char *fname, char *res);

#endif
