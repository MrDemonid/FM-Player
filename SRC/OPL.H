#ifndef H_ADPLUG_OPL
#define H_ADPLUG_OPL


#define TRUE           -1
#define FALSE           0
#define INP             inportb
#define OUTP            outportb

// типы таблиц базовых частот нот
#define NOTE_STANDART   0
#define NOTE_SA2        1
#define NOTE_S3M        2

// размеры массивов noteSpec[] и noteDots[]
#define LEN_NOTE_SPC    72      // 12 нот по 8 октав (исп. 9 нот по 8 октав)
#define LEN_NOTE_DOTS   648     // 9 каналов, 12 нот по 8 октав (исп. 9 нот по 8 октав)

#if LEN_NOTE_SPC != 72
    #error Please reassembler module VISASM for new LEN_NOTE_SPC!
#endif

#if LEN_NOTE_DOTS != 648
    #error Please reassembler module VISASM for new LEN_NOTE_DOTS!
#endif


typedef unsigned char bool;

typedef enum {TYPE_OPL2, TYPE_OPL3, TYPE_DUAL_OPL2} CHIP_TYPE;


//unsigned char inpb(unsigned int port);
//void outpb(unsigned int port, unsigned char val);


#ifdef __cplusplus
  extern "C" {
#endif

bool opl_detect(void);                  // returns true if adlib compatible board is found, else false
void opl_reset(void);                   // hardware reset
void opl_setnotetable(char notetype);   // set note table (const NOTE_XXXX)
void opl_setvolume(int volume);         // set adlib master volume (0 - 63) 0 = loudest, 63 = softest
void opl_setquiet(bool quiet);          // sets the OPL2 quiet, while still writing to the registers
void opl_setport(unsigned int port);    // set new OPL2 hardware baseport
void opl_setnowrite(bool nw);           // set hardware write status
int  opl_getvolume(void);               // get adlib master volume
CHIP_TYPE opl_gettype(void);            // return this OPL chip's type

void opl_write(unsigned char reg, unsigned char val);

void opl_setchip(int n);                // select OPL chip
int  opl_getchip(void);                 // returns current OPL chip

/*
  эквалайзер
*/
bool opl_getkeyon(unsigned char chan);
unsigned int opl_getchanvol(unsigned char chan); // return carrier+modulator volumes


/*
  note spectrum и note dots
  на входе:
    chan - номер канала
    notoct - нота * 8 + октава
*/
void opl_setnotespc(char chan, char notoct);

#ifdef __cplusplus
}
#endif

extern unsigned int  adlPort;           // базовый адрес порта OPL
extern unsigned char currChip;          // номер текущего чипа OPL
extern CHIP_TYPE     chipType;          // тип чипа OPL (CHIP_TYPE)

//extern int curVolume;
//extern int oldVolume;
//extern bool beQuiet;
//extern bool noWrite;

extern unsigned int  note_table[12];    // standard adlib note table
extern unsigned char op_table[9];       // the 9 operators as expected by the OPL2


extern unsigned int  chanFreq[18];      // текущая частота для каждого канала
extern unsigned char chanOctave[18];    // текущая октава для каждого канала
extern unsigned char chanNote[18];      // текущая нота для каждого канала
extern char far     *chanInst[18];      // текущий инструмент для каждого канала

extern unsigned char noteSpec[LEN_NOTE_SPC];
extern unsigned char noteDots[LEN_NOTE_DOTS];
extern unsigned char noteType;          // флаг (0 - NoteSpectrum, 1 - NoteDots)

//extern void hardwrite(char reg, char val);

#endif

