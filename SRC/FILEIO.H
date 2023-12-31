#pragma option -a-

#ifndef _FILEIO_H_
#define _FILEIO_H_

// ��ਡ��� 䠩��� ��� findfirst() & findnext()
#define FA_NORMAL   0x00        /* Normal file, no attributes */
#define FA_RDONLY   0x01        /* Read only attribute */
#define FA_HIDDEN   0x02        /* Hidden file */
#define FA_SYSTEM   0x04        /* System file */
#define FA_LABEL    0x08        /* Volume label */
#define FA_DIREC    0x10        /* Directory */
#define FA_ARCH     0x20        /* Archive */

// ����. ����� ��⥩ ����� 䠩��
#define MAXPATH     80
#define MAXDRIVE    3
#define MAXDIR      66
#define MAXFILE     9
#define MAXEXT      5

// 䫠�� ०��� ������ 䠩��
#define O_RDONLY    0x00
#define O_WRONLY    0x01
#define O_RDWR      0x02

// 䫠�� ०��� ᮧ����� 䠩�� (����, ����� �������஢���)
#define C_NORMAL    0x00        // ����� 䠩�
#define C_RDONLY    0x01
#define C_HIDDEN    0x02
#define C_SYSTEM    0x04


// 䫠�� ����樮��஢����
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2



//typedef struct {
//    unsigned int    handle;         // ����� 䠩��
//    int             bufsize;        // ࠧ��� ����
//    char far       *buffer;         // ����
//    char far       *curr;           // ⥪. ������ � ����
//} FILE;


typedef struct {
    char            reserved[21];   // Microsoft reserved - do not change
    char            attr;           // attribute byte for matched file
    unsigned int    time;           // time of last write to file
    unsigned int    date;           // date of last write to file
    long            size;           // size of file
    char            name[13];       // asciiz name of matched file
} DTA;

#ifdef __cplusplus
  extern "C" {
#endif

/*
  �����頥� ����� ⥪�饣� ��᪠
*/
int io_getcurdisk(void);
/*
  �����祭�� ���ன�⢠ ⥪�騬
  �� �室�:
    drive - 0 = A:, 1 = B:, 2 = C:
*/
void io_changedisk(char drive);

/*
  �ନ��� ��ப� � ������� ����㯭�� ��᪮� ����: " A  C  D "
  �� ��室�: ������⢮ ��᪮� (0..26)
*/
int io_disksavail(char *szdisks);


int io_getcurdir(char drive, char *direc);
int io_chdir(const char *path);

int io_findfirst(const char *pathname, unsigned int attrib, DTA *finfo);
int io_findnext(DTA *finfo);

int io_fopen(char *filename, char flags);
int io_fcreate(char *filename, int flags);
int io_fread(void *buff, unsigned int size, int handle);
int io_write(void *buff, unsigned int size, int handle);
unsigned long io_fseek(int handle, unsigned long offset, int whence);
unsigned long io_filesize(int handle);
char io_feof(int handle);
int io_fclose(int handle);

int io_gettime(int handle, unsigned int *date, unsigned int *time);


#ifdef __cplusplus
}
#endif

#endif
