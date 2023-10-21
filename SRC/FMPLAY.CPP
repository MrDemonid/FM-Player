//#include <stdio.h>          // printf()
#include <string.h>

#include "opl.h"
#include "player.h"
#include "hsc.h"
#include "rad.h"
#include "amd.h"
#include "sa2.h"
#include "s3m.h"
#include "dmo.h"
#include "lds.h"
//#include "msc.h"


#include "timer.h"
#include "visual.h"

#include "video.h"
#include "keyb.h"
#include "file.h"
#include "fileio.h"

#include "8x8.h"

#include "images.h"

//#include "debug.h"


#ifndef TARGET_XT
  char sCopyrignt[] = {"FM Player ver. 2.4.2\r\n(C)Andrey Hlus, 2019\r\n$"};
#else
  char sCopyrignt[] = {"FM Player ver. 2.4.2(XT)\r\n(C)Andrey Hlus, 2019\r\n$"};
#endif



static volatile bool dopoll;         // Flag set if timer processing enabled
static volatile bool firsttime;       // First time playing flag for each song

static volatile unsigned long time_ms;     // Current playing time in ms
static volatile unsigned long time_incr;
static volatile unsigned long oldfreq;
static volatile unsigned int  del,wait;
static volatile unsigned long currate;                // текущая реальная частота прерываний
static volatile unsigned long divt;
static volatile unsigned long curtempo;

static volatile unsigned long newtempo=1000;

unsigned int tempokoeff[17] = {111, 125, 142, 166, 200, 250, 333, 500,1000,2000,3000,4000,5000,6000,7000,8000,9000};
unsigned int tempoidx = 8;


char szBasePath[MAXPATH];       // путь запуска программы
int  dskCurrent;

char fCheckAdlib;               // флаг детектирования Adlib (0 - не искать)

/*
-x9       111      8         0
-x8       125      7         4
-x7       142      6         8
-x6       166      5        12
-x5       200      4        16
-x4       250      3        20
-x3       333      2        24
-x2       500      1        28

 x1      1000               32

 x2      2000      1        36
 x3      3000      2        40
 x4      4000      3        44
 x5      5000      4        48
 x6      6000      5        52
 x7      7000      6        56
 x8      8000      7        60
 x9      9000      8        64


 10..10           37                 0 8   24  40  56
-8 ..8            29
                                     7 15  31  47  63

                                     - - - - - - - - -
                                         16  32  48  64

                                         23  39  55
*/


static CPlayer *cplay = NULL;



void do_help(void)
{
    vid_DosStr("FMPLAY [param]\r\n$");
    vid_DosStr("  -a - disable detect adlib compatible card\r\n$");
    vid_DosStr("  -c - reverse tape counter\r\n$");
    vid_DosStr("  -d - switch to the note dots effect\r\n$");
    vid_DosStr("  -o - disable the note panel effects\r\n$");
    vid_DosStr("  -e - disable the help window\r\n$");
}


void calckRate(unsigned long newfreq)
{
    curtempo = newtempo;
    oldfreq = newfreq;
    newfreq = (newfreq * curtempo) / 1000L;
    del = wait = NORMALIZE / newfreq;
    divt = 1193180000L / (newfreq*(wait+1));
    currate = 1193180000L /divt;
    time_incr = 1000000000 / (oldfreq * (wait+1)); // ms * 1000
}

static void tmr_reinit(void)
{
    calckRate(cplay->getrefresh());
    vis_SetRate(currate);
    tmr_SetNewRate(divt);
}

static void far poll_player(void)
{
    if (!dopoll)  // Timer interrupt processing enabled?
        return;
    if (!cplay)            // no player in memory means we're not playing
        return;

    time_ms += time_incr; // 1000000000 / (oldfreq * (wait+1)); // ms * 1000

    vis_Update(time_ms/1000);
    // pseudo timer environment
    if (del)
    {
        del--;          // just wait for this time fraction
        return;
    } else
        del = wait;     // last fraction, reset for next one

    firsttime = cplay->update();    // call player
    if ((oldfreq != cplay->getrefresh()) || (curtempo != newtempo)) // new timer rate requested?
    {
        tmr_reinit();
    }
}

void play_waitRelease(char key)
{
    while (key_GetState(key) && firsttime)
        vis_ReDraw();
}



void btn_pressed(char key, char nbutton)
{
    vis_PushButton(nbutton);
    play_waitRelease(key);
    vis_ReleaseButton(nbutton);

}

void play_Pause(char key)
{
    opl_setquiet(TRUE);
    tmr_Pause();
    vis_PushButton(BTN_PAUSE);
    while (key_GetState(key));
    while (TRUE)
    {
        /*
        if (key_GetState(key))
        {
            btn_pressed(key, BTN_PAUSE);
            break;
        }
        */
        if (key_GetState(SCAN_P))
        {
            btn_pressed(SCAN_P, BTN_PAUSE);
            break;
        }
        if (key_GetState(SCAN_SPACE))
        {
            btn_pressed(SCAN_SPACE, BTN_PAUSE);
            break;
        }

    }
    key_Flash();
    vis_ReleaseButton(BTN_PAUSE);
    tmr_Pause();
    opl_setquiet(FALSE);
}

/*
  задержка между автоповтором нажатия клавиши
*/
void play_KeyPause(unsigned long time)
{
    unsigned long t;

    t = (unsigned long) time_ms+(curtempo * time) / 1000L;
    while (time_ms < t)
        vis_ReDraw();
}


void play_Volume(char key, int incr)
{
    int vol;
    unsigned long t;

    while (key_GetState(key) && firsttime)
    {
        vol = opl_getvolume() + incr;
        if ( (vol >= 0) && (vol <= 63) )
        {
            opl_setvolume(vol);
            vis_SetVolume(vol);
        }
        play_KeyPause(42000L);
    }
}

void play_Tempo(char key, int incr)
{
    int t;

    while (key_GetState(key) && firsttime)
    {
        t = tempoidx + incr;
        if ((t >= 0) && (t <= 16))
        {
            tempoidx = t;
            vis_SetTempo(tempoidx*4);

        }
        play_KeyPause(96000L);
    }
    newtempo = tempokoeff[tempoidx];
}



/*
void fast_forward(unsigned long ms)
{
    unsigned long ff;

    opl_setquiet(TRUE);
    ff = 0;
    ms = ms*1000L;
    while (ff < ms)
    {
        p->update();
        ff = ff + (1000000000L / p->getrefresh());
    }
    time_ms += ff;
    opl_setquiet(FALSE);
}

void play_Forward(void)
{
    while (key_GetState(SCAN_GREAT) && firsttime)
    {
        fast_forward(3000);
        vis_ReDraw();
    }
}
*/

void do_play(FLIST *f)
{
    unsigned long totaltime;
    bool breaklist = FALSE;
    bool playing;
    FLIST *t;

    int cursorDir;

    char fname[MAXPATH+16];
    char *fn;

    char drive;


    vis_PushButton(BTN_PLAY);
    while (f && !breaklist)
    {
        cursorDir = 1;                  // по умолчанию курсор смещаем на след. запись
        t = f->next;
        // формируем полное имя файла
        file_MakeFullName(f->dir, f->dta.name, fname);

        // создаем объект нужного проигрывателя
        switch (file_Type(f->dta.name))
        {
            case FILE_HSC: cplay = new ChscPlayer();
                           break;
            case FILE_RAD: cplay = new CradLoader();
                           break;
            case FILE_AMD: cplay = new CamdLoader();
                           break;
            case FILE_SA2: cplay = new Csa2Loader();
                           break;
            case FILE_S3M: cplay = new Cs3mPlayer();
                           break;
            case FILE_DMO: cplay = new CdmoLoader();
                           break;
            case FILE_LDS: cplay = new CldsPlayer();
                           break;
            //case FILE_MSC: cplay = new CmscPlayer();
            //               break;

            default: cplay = NULL;
        }
        if (cplay)
        {
            if (cplay->load(fname))
            {
                time_ms = 0;
                // приводим частоту таймера к не менее 36 fps
                dopoll = firsttime = FALSE;
                calckRate(cplay->getrefresh());
                totaltime = cplay->songlength(0);
                key_InstallHandler();
                vis_Begin(cplay->gettitle(), totaltime);
                vis_SetRate(currate);
                tmr_Init(&poll_player, divt);
                dopoll = firsttime = TRUE;
                playing = TRUE;
                while (firsttime && playing)            // busy waiting
                {
                    if (key_GetState(SCAN_P))           // Stop/Pause
                    {
                        play_Pause(SCAN_P);
                    } else if (key_GetState(SCAN_N))    // Next song (if list)
                    {
                        btn_pressed(SCAN_N, BTN_NEXT);
                        if (f->next)
                        {
                            playing = FALSE;
                        }

                    } else if (key_GetState(SCAN_B))    // Previous song
                    {
                        btn_pressed(SCAN_B, BTN_PREV);
                        if (f->prev)
                        {
                            t = f->prev;
                            playing = FALSE;
                            cursorDir = -1;
                        }

                    } else if (key_GetState(SCAN_SPACE))  // pause song
                    {
                        play_Pause(SCAN_SPACE);
                    } else if (key_GetState(SCAN_S))  // Stop song
                    {
                        btn_pressed(SCAN_S, BTN_STOP);
                        playing = FALSE;

                    } else if (key_GetState(SCAN_ESC))  // Break list
                    {
                        btn_pressed(SCAN_ESC, BTN_STOP);
                        playing = FALSE;
                        breaklist = TRUE;
                        cursorDir = 0;

                    } else if (key_GetState(SCAN_PLUS)) // Volume +
                    {
                        play_Volume(SCAN_PLUS, 1);

                    } else if (key_GetState(SCAN_MINUS))// Volume -
                    {
                        play_Volume(SCAN_MINUS, -1);

                    } else if (key_GetState(SCAN_PGDOWN))// Tempo -
                    {
                        play_Tempo(SCAN_PGDOWN, -1);

                    } else if (key_GetState(SCAN_PGUP))// Tempo +
                    {
                        play_Tempo(SCAN_PGUP, 1);

                    } else if (key_GetState(SCAN_C))
                    {
                        vis_ToggleDirectCnt();
                        play_waitRelease(SCAN_C);
                    } else if (key_GetState(SCAN_F1))
                    {
                        vis_ToggleHelp();
                        play_waitRelease(SCAN_F1);

                    } else if (key_GetState(SCAN_R))
                    {
                        btn_pressed(SCAN_R, BTN_REPLAY); //while (key_GetState(lastscan)) vis_ReDraw();
                        dopoll = FALSE;
                        opl_reset();
                        vis_Stop();
                        cplay->rewind(0);

                        // перезапускаем проигрыватель
                        time_ms = 0;
                        vis_Begin(cplay->gettitle(), totaltime);
                        tmr_reinit();
                        dopoll = TRUE;
                    } else if (key_GetState(SCAN_END))
                    {
                        vis_SwitchNotes();
                        play_waitRelease(SCAN_END);
                    }
                    vis_ReDraw();
                }
            } else {
                // ошибка загрузки файла
                delete cplay;
                cplay = NULL;
            }
            if (cplay)
            {
                tmr_Close();
                opl_reset();
                vis_Stop();
                key_RestoreHandler();
                delete cplay;
                cplay = NULL;
            }
        } // if (cplay)

        //
        f = t;
        if (f)
        {
            if (noteType == 2)
                file_NextCursor(cursorDir);
        }

    }
    vis_ReleaseButton(BTN_PLAY);
}



char parse_paramstr(int argc, char *argv[])
{
    int par = 1;

    fCheckAdlib = 1;
    noteType = 0;
    helpVisible = 1;
    cnt_direction = 0;

    while (par < argc)
    {
        if (argv[par][0] == '-' || argv[par][0] == '/')
        {
            switch (vid_UpChr(argv[par][1]))
            {
                case 'A':
                    // отключаем проверку наличия Adlib карты
                    fCheckAdlib = 0;
                    break;
                case 'C':
                    // переключаем счетчик ленты в реверс
                    cnt_direction = 1;
                    break;
                case 'D':
                    // включаем отображение note dots
                    noteType = 1;
                    break;
                case 'O':
                    noteType = 2;
                    break;
                case 'E':
                    // выключаем отображение панели помощи
                    helpVisible = 0;
                    break;
                case '?':
                case 'H':
                    // выводим справку и выходим
                default:
                    do_help();
                    return 0;
            }
        }
        par++;
    }
    return -1;
}


void main(int argc, char *argv[])
{
    unsigned long total;
    unsigned int min, sec, msec;
    FLIST *f;

    vid_DosStr(sCopyrignt);

    if (!parse_paramstr(argc, argv))
        return;

    // проверяем наличие видеоадаптера EGA или выше
    if (!vid_IsEGA())
    {
        vid_DosStr("This programm request EGA or later!\r\n$");
        return;
    }
    // проверяем наличие Adlib-совместимой карты
    if (fCheckAdlib)
    {
        if (!opl_detect())
        {
            vid_DosStr("Adlib card not found!\r\n$");
            return;
        }
    }
    opl_reset();
    // сохраняем текущий диск и каталог
    file_GetCurDir(szBasePath);

    // распаковываем битмапы
    if (!img_Unpack())
    {
        vid_DosStr("Not enought memory!\r\n$");
        return;
    }
    // устанавливаем видеорежим
    vid_SetVideoMode();
    vid_SetFont(Font8x8, 8, 0, 256);

    // и отрисовываем мафон
    vis_SetTempo(tempoidx*4);
    vis_Initialize();
    vis_ToggleHelp();

    // инициализируем файловый менеджер
    if (file_Init() == 0)
    {
        vid_RestoreMode();
        vid_DosStr("Not enought memory!\r\n$");
        return;
    }
    // начинаем работу
    while ((f = file_Run()) != NULL)
    {
        dskCurrent = io_getcurdisk();
        do_play(f);
        io_changedisk(dskCurrent);
    }
    // завершаем работу и уходим
    vid_RestoreMode();
    file_SetCurDir(szBasePath);
    vid_DosStr("Good bye!\r\n$");
}
