#ifndef _KEYB_H_
#define _KEYB_H_

#ifdef __cplusplus
  extern "C" {
#endif

// клавиши управлением курсором
#define KEY_UP          0x48
#define KEY_DOWN        0x50
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_INS         0x52
#define KEY_DEL         0x53
#define KEY_HOME        0x47
#define KEY_END         0x4F
#define KEY_PGDN        0x51
#define KEY_PGUP        0x49

#define KEY_ESC         0x1B
#define KEY_TAB         0x09
#define KEY_ENTER       0x0D
#define KEY_SPACE       0x20


#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F3          0x3D
#define KEY_F4          0x3E
#define KEY_F5          0x3F
#define KEY_F6          0x40
#define KEY_F7          0x41
#define KEY_F8          0x42
#define KEY_F9          0x43
#define KEY_F10         0x44

#define KEY_ALT_D       0x20
#define KEY_ALT_L       0x26
#define KEY_ALT_S       0x1F
#define KEY_ALT_X       0x2D
#define KEY_ALT_M       0x32


#define KEY_SCROLLLOCK  0x46

/*
  скэн-коды клавиш
*/
#define SCAN_1          0x02
#define SCAN_2          0x03
#define SCAN_3          0x04
#define SCAN_4          0x05
#define SCAN_5          0x06
#define SCAN_6          0x07
#define SCAN_7          0x08
#define SCAN_8          0x09
#define SCAN_9          0x0A
#define SCAN_0          0x0B

#define SCAN_A          0x1E
#define SCAN_B          0x30
#define SCAN_C          0x2E
#define SCAN_D          0x20
#define SCAN_E          0x12
#define SCAN_F          0x21
#define SCAN_G          0x22
#define SCAN_H          0x23
#define SCAN_I          0x17
#define SCAN_J          0x24
#define SCAN_K          0x25
#define SCAN_L          0x26
#define SCAN_M          0x32
#define SCAN_N          0x31
#define SCAN_O          0x18
#define SCAN_P          0x19
#define SCAN_Q          0x10
#define SCAN_R          0x13
#define SCAN_S          0x1F
#define SCAN_T          0x14
#define SCAN_U          0x16
#define SCAN_V          0x2F
#define SCAN_W          0x11
#define SCAN_X          0x2D
#define SCAN_Y          0x15
#define SCAN_Z          0x2C

#define SCAN_MINUS      0x0C
#define SCAN_PLUS       0x0D
#define SCAN_LBRACKET   0x1A  // [
#define SCAN_RBRACKET   0x1B  // ]
#define SCAN_SEMICOLON  0x27  // ;:
#define SCAN_RQUOTE     0x28  // '"
#define SCAN_LQUOTE     0x29  // `~
#define SCAN_GREAT      0x33  // >.
#define SCAN_LATER      0x34  // <,
#define SCAN_SLASH      0x35  // ?
#define SCAN_BACLSLASH  0x2B  // |
#define SCAN_F1         0x3B
#define SCAN_F2         0x3C
#define SCAN_F3         0x3D
#define SCAN_F4         0x3E
#define SCAN_F5         0x3F
#define SCAN_F6         0x40
#define SCAN_F7         0x41
#define SCAN_F8         0x42
#define SCAN_F9         0x43
#define SCAN_F10        0x44
#define SCAN_ESC        0x01
#define SCAN_BACKSPACE  0x0E
#define SCAN_TAB        0x0F
#define SCAN_ENTER      0x1C
#define SCAN_CONTROL    0x1D
#define SCAN_LSHIFT     0x2A
#define SCAN_RSHIFT     0x36
#define SCAN_PRTSC      0x37
#define SCAN_ALT        0x38
#define SCAN_SPACE      0x39
#define SCAN_CAPSLOCK   0x3A
#define SCAN_NUMLOCK    0x45
#define SCAN_SCROLLLOCK 0x46
#define SCAN_HOME       0x47
#define SCAN_UP         0x48
#define SCAN_PGUP       0x49
//#define SCAN_GRAY_MINUS      0x4A
#define SCAN_LEFT       0x4B
#define SCAN_CENTER     0x4C
#define SCAN_RIGHT      0x4D
//#define SCAN_GRAY_PLUS       0x4E
#define SCAN_END        0x4F
#define SCAN_DOWN       0x50
#define SCAN_PGDOWN     0x51
#define SCAN_INS        0x52
#define SCAN_DEL        0x53


/*
  ввод с клавиатуры средствами BIOS
  на выходе:
      ax - KEY_XXXX
*/
unsigned int key_Get(void);


/*
  установка и деинсталляция своего обработчика клавиатуры
*/
void key_InstallHandler(void);
void key_RestoreHandler(void);

/*
  возвращает 1, если клавиша нажата или была нажата с момента последнего
  вызова этой функции
  на входе:
      ScanCode - скан-код проверяемой клавиши (SCAN_XXXX)
*/
bool key_GetState(unsigned char ScanCode);

/*
  возвращает 1, если клавиша нажата в текущий момент
  на входе:
      ScanCode - скан-код проверяемой клавиши (SCAN_XXXX)
*/
bool key_GetCurrentState(unsigned char ScanCode);

/*
  очищает буфера скан-кодов клавиатуры
*/
void key_Flash(void);


#ifdef __cplusplus
}
#endif

#endif
