//#include <stdio.h>          // printf()
#include <string.h>

#include "opl.h"
#include "video.h"
#include "keyb.h"
#include "fileio.h"
#include "file.h"

#include "visual.h"
#include "timer.h"


#include "images.h"

//#include "debug.h"


typedef struct
{
    FLIST      *list;               // список файлов
    FLIST      *first;              // пеpвый отобpажаемый файл
    FLIST      *curptr;             // файл под курсором
    int         numfiles;           // количество файлов в списке
    char        curvis;             // видимость курсора
    int         scrpos;             // позиция курсора на экране
    int         curpos;             // позиция курсора в списке
    int         psX;                // координаты панели
    int         psY;
    int         maxvis;             // макс. кол. отображаемых файлов в панели
    int         sbpos;              // новое положение Scroll bar
    int         sblast;             // последнее отрисованное положение Scroll bar
} PANEL;


typedef struct
{
    int imgx, imgy;                 // координаты панели файлового менеджера
    int infox, infoy;               // координаты информационной строки
    PANEL *files;                   // список файлов тек. директории
    PANEL *plays;                   // плейлист
    PANEL *current;                 // текущая панель
} FMNG;

FMNG fmng;                          // файловый менеджер
FLIST cfile;                        // одиночный файл для проигрывания



/*
  известные расширения файлов и их типы
*/
char musexts[][5] = {{".HSC"},{".RAD"},{".AMD"},{".SAT"},{".SA2"},{".S3M"},{".DMO"},{".LDS"},0};
char muscode[] = {FILE_HSC, FILE_RAD, FILE_AMD, FILE_SA2, FILE_SA2, FILE_S3M, FILE_DMO, FILE_LDS};


/*
  Вывод строки в панель менеджера файлов
*/
void file_PutString(char *str)
{
    vid_PutText(fmng.infox, fmng.infoy, 72, str);
}




static void far poll_time(void)
{
    vis_ShowTime();
}


void file_GetCurDir(char *path)
{
    path[0] = 'A' + io_getcurdisk();  // replace X with current drive letter
    path[1] = ':';
    path[2] = '\\';
    io_getcurdir(0, path+3);   // fill rest of string with current directory
}

int file_SetCurDir(char *path)
{
    char tmp[MAXPATH];
    int len;
    char *dir, drv;

    strcpy(tmp, path);
    len = strlen(tmp);
    if (tmp[len-1] == '\\')
    {
        tmp[len-1] = 0;
        len--;
    }
    dir = &tmp[0];
    if (len >= 3 && tmp[1] == ':' && tmp[2] == '\\')
    {
        // похоже на путь с диском
        drv = tmp[0];
        if (drv < 'A' || drv >= 'Z')
            return 0;
        io_changedisk(drv-'A');
        dir = &tmp[3];
        // dbg_Log("  -drv: %c ", drv);
    }
    io_chdir("\\");      // переходим в корневой каталог
    len = io_chdir(dir); // а теперь в заданный
    // dbg_Log("  - path: %s  = [%i]\n", dir, len);
    return len;
}

void file_MakeFullName(char *dir, char *fname, char *res)
{
    char *fn, drv;
    int  len;

    fn = res;
    if (dir)
    {
        len = strlen(dir);
        if (len >= 3 && dir[1] == ':' && dir[2] == '\\')
        {
            // похоже на путь с диском
            drv = dir[0];
//            if ((drv-'A') != io_getcurdisk() && drv >= 'A' && drv <= 'Z')
            if (drv >= 'A' && drv <= 'Z')
            {
                io_changedisk(drv-'A');
            }
        }
        strcpy(res, dir);
        // проверяем наличие слеша в конце
        if (res[len-1] != '\\')
        {
            res[len] = '\\';
            res[len+1] = 0;
        }
        fn += strlen(res);
    }

    strcpy(fn, fname);
}


FLIST *lst_Insert(FLIST *l, char *dir, DTA *obj, bool sort)
{
    FLIST *prev, *next, *t;

    // создаем новую запись
    t = new FLIST;
    if (!t)
        return l;      // облом, память закончилась
    t->dta.attr = obj->attr;
    t->dta.time = obj->time;
    t->dta.date = obj->date;
    t->dta.size = obj->size;
    memcpy(t->dta.name, obj->name, 13);
    t->dir = NULL;
    t->next = NULL;
    t->prev = NULL;
    if (dir)
    {
       t->dir = new char[strlen(dir)+2];
       if (t->dir)
           strcpy(t->dir, dir);
    }
    if (!l)
    {
        return t;
    }
    // ищем подходящую позицию
    next = l;
    prev = next->prev;
    while (next)
    {
        if (sort && (strcmp(t->dta.name, next->dta.name) < 0))
            break;
        prev = next;
        next = next->next;
    }
    t->prev = prev;
    t->next = next;
    if (!prev)         // в начало списка
    {
        next->prev = t;
        return t;
    } else if (!next)  // в конец списка
    {
        prev->next = t;
    } else {           // в середину списка
        prev->next = t;
        next->prev = t;
    }
    return l;
}

FLIST *lst_Concatenate(FLIST *dirs, FLIST *files)
{
    FLIST *p;

    if (!dirs)
        return files;
    if (!files)
        return dirs;
    p = dirs;
    while (p->next)
        p = p->next;
    p->next = files;
    files->prev = p;
    return dirs;
}

//
// Создает писок файлов в текущем каталоге
//

FLIST *lst_FindFiles(int *numfiles, char islist)
{
    char   dir[MAXDIR+2];
    DTA    ff;
    int    done;
    int    count = 0;
    FLIST *f = NULL;            // список файлов
    FLIST *d = NULL;            // список каталогов

    file_GetCurDir(dir);        // получаем текущий диск и текущую диpектоpию
    done = io_findfirst("*.*", FA_NORMAL | FA_RDONLY | FA_DIREC, &ff);
    while (!done)
    {
        // добавляем файл в список
        if (ff.attr & FA_DIREC)
        {
            if (strcmp(ff.name, "."))       // это текущий каталог ?
            {
                // не текущий, добавляем в список
                d = lst_Insert(d, NULL, &ff, TRUE);
                count++;
            }
        } else {
            // файл
            if (islist)
            {
                // добавляем файл в режиме загрузки плей-листа
                if (!file_Type(ff.name))
                {
                    strlwr(ff.name);
                    f = lst_Insert(f, dir, &ff, TRUE);
                    count++;
                }
            } else {
                // добавляем только разрешенные типы файлов
                if (file_Type(ff.name))
                {
                    strlwr(ff.name);
                    f = lst_Insert(f, dir, &ff, TRUE);
                    count++;
                }
            }

        }
        // ищем следующий
        done = io_findnext(&ff);
    }
    if (numfiles)
        *numfiles = count;
    return lst_Concatenate(d, f);
}

//
// очищает список файлов
//
void file_Free(PANEL *pan)
{
    FLIST *p, *t;

    if (!pan)
        return;
    t = pan->list;
    while (t)
    {
        p = t->next;
        if (t->dir)
        {
            delete[] t->dir;
        }
        delete t;
        t = p;
    }
    pan->list = NULL;
    pan->first = NULL;
    pan->curptr = NULL;
    pan->numfiles = 0;
    pan->scrpos = 0;
    pan->curpos = 0;
    pan->sblast = pan->sbpos;
    pan->sbpos = 0;
}


/*
  обсчет Scroll Bar панели (по pan->curpos)
*/
void file_SetScrollBar(PANEL *pan)
{
    unsigned long mod;

    if (pan->numfiles == 0)
    {
        pan->sbpos = 0;

    } else {
        mod = 4000L / pan->numfiles;
        if (pan->numfiles >= 4)
        {
            pan->sbpos = ((unsigned long) pan->curpos * mod) / 1000L;
        } else
        {
            pan->sbpos = ((unsigned long) pan->curpos * 150 * mod) / 100000L;
        }
    }
}


/*
  обновляет на экране Scroll Bar панели
*/                    //0xDA, 0x42
//#define SBMASK        0x51

void file_UpdateScrollBar(PANEL *pan)
{
    int x, y;

    if (pan->sblast != pan->sbpos)
    {
        // положение изменилось
        x = (pan->psX - 1) + (pan->maxvis / 6) * 14;
        y = pan->psY + 1;
        if (pan->sblast >= 0)
        {
            // удаляем со старого места
            vid_XorColor(x, y+pan->sblast, 1, 0xDA);
        }
        // рисуем в новом месте
        vid_XorColor(x, y+pan->sbpos, 1, 0xDA);
        pan->sblast = pan->sbpos;
    }
}


/*
  выводит в панель список файлов и обновляет Scroll Bar
*/
void file_ReDraw(PANEL *pan)
{
    int    i, x, y;
    FLIST *t;

    if (!pan)
        return;
    t = pan->first;
    x = pan->psX;
    y = pan->psY;
    for (i=0; i<pan->maxvis; i++)
    {
        if (t)
        {
            vid_PutText(x, y, 12, t->dta.name);
            t = t->next;
        } else {
            vid_PutText(x, y, 12, "");
        }
        y++;
        if (y == pan->psY+6)
        {
            y = pan->psY;
            x += 14;
        }

    }
    file_UpdateScrollBar(pan);
}


//
// выводит информацию о файле под курсором
//
void file_DrawInfo(PANEL *pan)
{
    FLIST *t;

    if (pan)
    {
        t = pan->curptr;
        if (t)
        {
            vid_PutInfo(fmng.infox, fmng.infoy, t->dta.name, t->dta.attr, t->dta.size, t->dta.date, t->dta.time);
        } else {
            vid_ClrInfo(fmng.infox, fmng.infoy);
        }
    }
}


void file_DrawCursor(PANEL *pan)
{
    int x, y;

    x = pan->psX + (pan->scrpos / 6) * 14;
    y = pan->psY + pan->scrpos % 6;
    vid_XorColor(x, y, 12, 0xEF);
}


void file_ShowCursor(PANEL *pan)
{
    if (!pan->curvis)
    {
        file_DrawCursor(pan);
        pan->curvis++;
    }
}

void file_HideCursor(PANEL *pan)
{
    if (pan->curvis)
    {
        file_DrawCursor(pan);
        pan->curvis--;
    }
}

void file_MoveCursor(PANEL *pan, int incr, char isInfo)
{
    int   scrpos, curpos;
    FLIST *first, *curptr;
    char  redraw = FALSE;

    if (!pan)
        return;
    first = pan->first;
    curptr = pan->curptr;
    scrpos = pan->scrpos;
    curpos = pan->curpos;
    if (!curptr)
        return;

    if (incr < 0)
    {
        while (incr != 0)
        {
            if (scrpos > 0)
            {
                scrpos--;
                curptr = curptr->prev;
            } else {
                if (!first->prev)
                    break;
                first = first->prev;
                curptr = first;
                redraw = TRUE;
            }
            curpos--;
            incr++;
        }
    } else {
        while (incr)
        {

            if (!curptr->next)
                break;
            scrpos++;
            curptr = curptr->next;
            if (scrpos >= pan->maxvis)
            {
                scrpos--;
                first = first->next;
                redraw = TRUE;
            }
            curpos++;
            incr--;
        }
    }

    file_HideCursor(pan);
    pan->first = first;
    pan->curptr = curptr;
    pan->scrpos = scrpos;
    pan->curpos = curpos;
    file_SetScrollBar(pan);
    if (redraw)
        file_ReDraw(pan);
    else
        file_UpdateScrollBar(pan);
    file_ShowCursor(pan);
    if (isInfo)
        file_DrawInfo(pan);
}


void file_InsertRec(PANEL *pan, FLIST *rec)
{
    char   dir[MAXDIR+2];

    if (rec->dta.attr & FA_DIREC)
        return;
    file_GetCurDir(dir);
    pan->list = lst_Insert(pan->list, dir, &rec->dta, FALSE);
    if (!pan->first)
    {
        pan->first = pan->list;
        pan->curptr = pan->list;
    }
    pan->numfiles++;
    file_SetScrollBar(pan);
    file_ReDraw(pan);
}


//
// удаляет файл из списка
//
void file_RemoveRec(PANEL *pan)
{
    FLIST *curptr;
    FLIST *nxt, *prv, *first, *newcur;

    if (!pan || !pan->curptr || !pan->list)
        return;
    first = pan->first;
    curptr = pan->curptr;
    nxt = curptr->next;
    prv = curptr->prev;

    file_HideCursor(fmng.current);
    // корректируем указатель первой видимой записи
    if (first == curptr)
    {
        if (nxt)
        {
            first = nxt;
        } else {
            first = prv;
        }
    }
    if (nxt)
    {
        newcur = nxt;
    } else {
        newcur = prv;
        if (pan->scrpos)
            pan->scrpos--;
        if (pan->curpos)
            pan->curpos--;
    }

    // освобождаем память записи
    if (prv)
        prv->next = nxt;
    if (nxt)
        nxt->prev = prv;
    if (curptr->dir)
        delete[] curptr->dir;
    delete curptr;

    // последняя корректировка
    if (!nxt && !prv)
    {
        pan->list = NULL;
        first = NULL;
        newcur = NULL;
        pan->scrpos = 0;
        pan->curpos = 0;
        pan->numfiles = 1;
    }
    pan->first = first;
    pan->curptr = newcur;
    pan->numfiles--;
    file_SetScrollBar(pan);
    file_ReDraw(pan);
    file_DrawInfo(pan);
    file_ShowCursor(fmng.current);
}


void file_ScanDir(PANEL* pan, char islist)
{
    pan->list = lst_FindFiles(&pan->numfiles, islist);
    pan->first = pan->list;
    pan->curptr = pan->list;
    pan->scrpos = 0;
    pan->curpos = 0;
    file_SetScrollBar(pan);
}


PANEL *file_NewPanel(int sx, int sy, int max)
{
    PANEL *rec;

    rec = new PANEL;
    if (rec)
    {
        rec->list = NULL;
        rec->first = NULL;
        rec->curptr = NULL;
        rec->numfiles = 0;
        rec->curvis = 0;
        rec->scrpos = 0;
        rec->curpos = 0;
        rec->sbpos = 0;
        rec->sblast = -1;
        rec->psX = sx;
        rec->psY = sy;
        rec->maxvis = max;
    }
    return rec;
}


void file_DoKey(PANEL *pan, char key)
{
    switch (key)
    {
        case KEY_UP:    file_MoveCursor(pan, -1, 1);
                        break;
        case KEY_DOWN:  file_MoveCursor(pan, 1, 1);
                        break;
        case KEY_LEFT:  file_MoveCursor(pan, -6, 1);
                        break;
        case KEY_RIGHT: file_MoveCursor(pan, 6, 1);
                        break;
        case KEY_HOME:  file_MoveCursor(pan, -pan->numfiles, 1);
                        break;
        case KEY_END:   file_MoveCursor(pan, pan->numfiles, 1);
                        break;
        case KEY_PGUP:  file_MoveCursor(pan, -pan->maxvis, 1);
                        break;
        case KEY_PGDN:  file_MoveCursor(pan, pan->maxvis, 1);
                        break;
    }
}

void file_NextCursor(int dir)
{
    if (dir > 0)
        file_DoKey(fmng.current, KEY_DOWN);
    else if (dir < 0)
        file_DoKey(fmng.current, KEY_UP);
}


void file_Rescan(char islst)
{
    file_Free(fmng.current);
    file_ScanDir(fmng.current, islst);
    file_ReDraw(fmng.current);
    file_ShowCursor(fmng.current);
    file_DrawInfo(fmng.current);
}



//
// Инициализация файлового менеджера
//
int file_Init(void)
{
    fmng.imgx = IMG_FILE_X;
    fmng.imgy = panFileY;
    fmng.infox = IMG_FILE_X + 3;
    fmng.infoy = panFileY + 2;
    fmng.files = file_NewPanel(IMG_FILE_X+3, panFileY+5, 18);
    fmng.plays = file_NewPanel(IMG_FILE_X+50, panFileY+5, 12);
    if ((!fmng.files) || (!fmng.plays))
        return 0;
    file_ScanDir(fmng.files, 0);
    fmng.current = fmng.files;
    return -1;
}


/*
  показ/скрытие курсора в строке информации
*/
void file_DrvCursor(int x)
{
    vid_XorColor(fmng.infox+x, fmng.infoy, 3, 0xCF);
}

/*
  выбор диска
*/
void file_ChangeDrive(void)
{
    char curdisk, newdisk;
    char numdisk;
    char drvlist[80];
    char *pos;
    int  key;
    int  x, maxx;
    bool running;

    vid_ClrInfo(fmng.infox, fmng.infoy);
    curdisk = io_getcurdisk() + 'A';
    numdisk = io_disksavail(&drvlist[0]);
    if (numdisk)
    {
        file_PutString(drvlist);
        // ищем в списке текущий диск
        pos = strchr(drvlist, curdisk);
        if (pos && (pos > &drvlist[0]))
        {
            // такой диск существует
            x = pos - &drvlist[0] - 1;
            maxx = (numdisk-1)*3;
            file_DrvCursor(x);
            // начинаем цикл выбора диска
            running = TRUE;
            while (running)
            {
                key = key_Get();
                if (key & 0xFF)
                {
                    switch (key & 0xFF)
                    {
                        case KEY_ENTER:
                            newdisk = drvlist[x+1];
                            if (newdisk != curdisk)
                            {
                                // сменился текущий диск
                                io_changedisk(newdisk-'A');
                                file_Free(fmng.files);
                                file_ScanDir(fmng.files, 0);
                                file_ReDraw(fmng.files);
                            }
                            running = FALSE;
                            break;
                        case KEY_ESC:
                            running = FALSE;
                            break;
                    }

                } else {
                    switch (key >> 8)
                    {
                        case KEY_LEFT:
                            if (x >= 3)
                            {
                                file_DrvCursor(x);
                                x -= 3;
                                file_DrvCursor(x);
                            }
                            break;
                        case KEY_RIGHT:
                            if (x < maxx)
                            {
                                file_DrvCursor(x);
                                x += 3;
                                file_DrvCursor(x);
                            }
                            break;
                    }
                }
            } // while (running)
        }
    } // if (numdisk)
    vid_XorColor(fmng.infox+x, fmng.infoy, 3, 0xCF);
    file_DrawInfo(fmng.current);
}

void file_SaveList(void)
{
    int f;
    FLIST *t;
    char buf[13];
    char fname[128];
    char *ch;

    if (fmng.plays->numfiles == 0)
        return;

    vid_ClrInfo(fmng.infox, fmng.infoy);
    vid_GetText(fmng.infox, fmng.infoy, sizeof(buf), buf);
    vid_ClrInfo(fmng.infox, fmng.infoy);
    f = io_fcreate(buf, C_NORMAL);
    if (f)
    {
        t = fmng.plays->list;
        while (t)
        {
            if ((t->dta.attr & FA_DIREC) == 0)
            {
                strcpy(fname, t->dir);
                if (strlen(t->dir))
                {
                    ch = &fname[strlen(t->dir)-1];
                    if (*ch != '\\')
                    {
                        strcat(fname, "\\");
                    }
                }
                strcat(fname, t->dta.name);
                strcat(fname, "\r\n");
                io_write(fname, strlen(fname), f);
            }
            t = t->next;
        }
        io_fclose(f);
    }

}



void split(char *str, char *path, char *name)
{
    char *p;

    p = strrchr(str, '\\');
    if (!p)
    {
        // только имя файла
        strcpy(name, str);
        file_GetCurDir(path);
    } else {
        // полный путь до файла
        p++;
        strcpy(name, p);
        *p = 0;
        strcpy(path, str);
    }
}


void loadpl(char *name)
{
    char   dir[MAXDIR+2];
    FLIST *fl;
    char *buf, *p, *str;
    unsigned long fs;
    int handle, th;
    DTA    ff;
    int    nfiles = 0;

    fl = NULL;

    if ((handle = io_fopen(name, O_RDONLY)) != 0)
    {
        fs = io_filesize(handle);
        if (fs > 0 && fs < 32768)
        {
            buf = new char[fs+2];
            if (buf)
            {
                io_fread(buf, fs, handle);
                p = buf;
                while (fs)
                {
                    // пропускаем пробелы
                    while (fs && (*p <= ' '))
                    {
                        fs--;
                        p++;
                    }
                    // считываем строку
                    str = p;
                    while (fs && (*p > ' '))
                    {
                        fs--;
                        p++;
                    }
                    *p = 0;
                    if (strlen(str))
                    {
                        th = io_fopen(str, O_RDONLY);
                        if (th)
                        {
                            // есть такой файл, заполняем структуру DTA
                            ff.attr = 0;
                            io_gettime(th, &ff.date, &ff.time);
                            ff.size = io_filesize(th);
                            io_fclose(th);
                            split(str, dir, ff.name);
                            fl = lst_Insert(fl, dir, &ff, FALSE);
                            nfiles++;
                        }
                    }
                } // while (fs)
                delete[] buf;
            }
        }
        io_fclose(handle);
    }
    if (fl)
    {
        file_Free(fmng.plays);
        fmng.plays->list = fl;
        fmng.plays->first = fl;
        fmng.plays->curptr = fl;
        fmng.plays->numfiles = nfiles;
        file_ReDraw(fmng.plays);
    }
}


void file_LoadList(void)
{
    FLIST *t;
    PANEL *old;
    unsigned char running;
    unsigned int  ch;

    running = TRUE;
    old = fmng.current;
    file_HideCursor(fmng.current);
    fmng.current = fmng.files;

    file_Rescan(1);
    file_PutString("Select list file and press [Enter]");

    while (running)
    {
        ch = key_Get();
        t = fmng.current->curptr;
        switch (ch & 0xFF)
        {
            case KEY_ENTER:
                if (t->dta.attr & FA_DIREC)
                {
                    // переходим в другой каталог
                    file_HideCursor(fmng.current);
                    io_chdir(t->dta.name);
                    file_Rescan(1);
                } else {
                    // загружаем плей лист
                    loadpl(t->dta.name);
                    running = FALSE;
                }
                break;

            case KEY_ESC:
                running = FALSE;
                break;

            default:
                file_DoKey(fmng.files, ch >> 8);
        } // switch
    } // while
    file_HideCursor(fmng.current);
    file_ScanDir(fmng.files, 0);
    file_ReDraw(fmng.files);
    fmng.current = old;
    file_DrawInfo(fmng.current);
    file_ShowCursor(fmng.current);
}


void file_swapRec(FLIST *one, FLIST *two, int incr)
{
    FLIST tmp;

    memcpy(&tmp, one, sizeof(FLIST));
    // меняем DTA
    memcpy(&one->dta, &two->dta, sizeof(DTA));
    memcpy(&two->dta, &tmp.dta, sizeof(DTA));
    // меняем *dir
    one->dir = two->dir;
    two->dir = tmp.dir;

    file_MoveCursor(fmng.current, incr, 0);
    file_ReDraw(fmng.current);
}


void file_MoveName(void)
{
    unsigned int  ch;
    FLIST *cur, *nxt;

    file_PutString("Move file to desirable position and press [Enter]");
    while (1)
    {
        cur = fmng.current->curptr;
        ch = key_Get();

        if ((ch & 0xFF) == KEY_ENTER)
        {
            vid_ClrInfo(fmng.infox, fmng.infoy);
            file_DrawInfo(fmng.current);
            return;
        } else {
            switch (ch >> 8)
            {
                case KEY_UP:
                    if (cur->prev)
                    {
                        file_swapRec(cur, cur->prev, -1);
                    }
                    break;
                case KEY_DOWN:
                    if (cur->next)
                    {
                        file_swapRec(cur, cur->next, 1);
                    }
                    break;
            }
        }
    } // while (1)
}



FLIST *file_Run(void)
{
    DTA   *dt;

    unsigned char running;
    unsigned int  ch;
    FLIST *t;

    tmr_Init(&poll_time, 65536);

    vid_PutSprite(imgFile, fmng.imgx, fmng.imgy);
    fmng.files->sblast = -1;
    fmng.plays->sblast = -1;
    file_ReDraw(fmng.plays);
    file_ReDraw(fmng.files);
    fmng.current->curvis = 0;
    file_ShowCursor(fmng.current);
    file_DrawInfo(fmng.current);

    running = TRUE;
    while (running)
    {
        ch = key_Get();
        t = fmng.current->curptr;
        if (ch & 0xFF)
        {
            switch (ch & 0xFF)
            {
                case KEY_ENTER:
                    if (t->dta.attr & FA_DIREC)
                    {
                        if (fmng.current == fmng.files)
                        {
                            file_HideCursor(fmng.current);
                            io_chdir(t->dta.name);
                            file_Rescan(0);
                        }
                    } else {
                        //file_HideCursor(fmng.current);

                        if (fmng.current == fmng.files)
                        {
                            memcpy(&cfile, t, sizeof(FLIST));
                            cfile.next = NULL;
                            cfile.prev = NULL;
                            tmr_Close();
                            //vid_ClrInfo(fmng.infox, fmng.infoy);
                            return &cfile;
                        } else {
                            if (fmng.plays->curptr)
                            {
                                tmr_Close();
                                //vid_ClrInfo(fmng.infox, fmng.infoy);
                                return fmng.plays->curptr;
                            }

                        }
                    }
                    break;
                case KEY_SPACE:
                    if (fmng.current == fmng.files)
                    {
                        file_InsertRec(fmng.plays, t);
                        file_DoKey(fmng.current, KEY_DOWN);
                    //} else {
                    //    file_RemoveRec(fmng.plays);
                    }
                    break;

                case KEY_ESC:
                    running = FALSE;
                    break;
                case KEY_TAB:
                    file_HideCursor(fmng.current);
                    if (fmng.current == fmng.files)
                    {
                        fmng.current = fmng.plays;
                    } else {
                        fmng.current = fmng.files;
                    }
                    file_ShowCursor(fmng.current);
                    file_DrawInfo(fmng.current);
                    break;
            } // switch
        } else {
            ch >>= 8;
            switch (ch)
            {
                case KEY_F1:
                    vis_ToggleHelp();
                    break;
                case KEY_DEL: // remove file from play list
                    if (fmng.current == fmng.plays)
                    {
                        file_RemoveRec(fmng.plays);
                    }
                    break;
                case KEY_ALT_M: // move file name
                    if (fmng.current == fmng.plays)
                    {
                        file_MoveName();
                    }
                    break;
                case KEY_ALT_D: // cange disk
                    file_HideCursor(fmng.current);
                    file_ChangeDrive();
                    file_ShowCursor(fmng.current);
                    break;
                case KEY_ALT_S: // save file list
                    file_HideCursor(fmng.current);
                    file_SaveList();
                    file_ShowCursor(fmng.current);
                    break;
                case KEY_ALT_L: // load file list
                    file_LoadList();
                    break;
                case KEY_ALT_X: // exit to DOS
                    running = FALSE;
                    break;
                default:
                    file_DoKey(fmng.current, ch);
            } // switch (ch)
        }

    }
    tmr_Close();
    vid_ClrInfo(fmng.infox, fmng.infoy);
    return NULL;
}

/*
  Определяет тип файла
  на входе:  имя файла
  на выходе: тип файла (константы FILE_XXX )
*/

int file_Type(char *fname)
{

    char *pos;
    int   i;

    pos = strchr(fname, '.');

    if (pos)
    {
        i = 0;
        while (musexts[i][0] != '\0')
        {
            if (!stricmp(pos, musexts[i]))
                return muscode[i];
            i++;
        }
    }
    return FILE_UNKNOWN;
}
