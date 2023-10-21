#ifndef _VISUAL_H_
#define _VISUAL_H_

#ifdef __cplusplus
  extern "C" {
#endif

/*
  частоты   (в формате: частота x 1000)
*/
#define NORMALIZE        60000L    // Timer frequency to normalize to

#define EQUAL_REFRESH    30000L    // частота обновления полос эквалайзера (30 fps)
#define COUNTER_REFRESH  16000L    // частота обновления счетчика ленты (16 fps)
#define NOTESPEC_REFRESH 60000L    // частота обновления Note spectrum
#define TAPE_REFRESH     10000L    // частота обновления кассетника



/*
  панель помощи
*/
extern char helpVisible;           // флаг показа панели помощи
/*
  счетчик длины ленты
*/
extern char cnt_direction;      // направление счета: 0-увеличение, 1-уменьшение
/*
  координата панелей по Y
*/
extern int panHelpY;
extern int panPlayerY;
extern int panFileY;





/*
  инициализация модуля
  битмапы уже должны быть распакованы
*/
void vis_Initialize(void);

/*
  начало визуализации мелодии
  на входе:
    title     - название мелодии
    totaltime - длина мелодии в мс
*/
void vis_Begin(char *title, unsigned long totaltime);
/*
  завершение визуализации мелодии
*/
void vis_Stop(void);
/*
  настройка визуализатора на заданную частоту прерываний и длину музыки
*/
void vis_SetRate(unsigned long rate);


/*
  обновление данных визуальной части проигрывателя
  на входе:
    time - время от начала проигрывания в мс
*/
void vis_Update(unsigned long time);

// полная перерисовка
void vis_ReDraw(void);

// отрисовка времени
void vis_ShowTime(void);

// отрисовка кнопок
void vis_PushButton(unsigned char key);
void vis_ReleaseButton(unsigned char key);

/*
  переключает направление счетчика ленты (на увеличение/уменьшение отсчета)
*/
void vis_ToggleDirectCnt(void);

/*
  установка регуляторов темпа и громкости
*/
void vis_SetTempo(unsigned char tempo);
void vis_SetVolume(unsigned char volume);

/*
  отображает/скрывает справку
*/
void vis_ToggleHelp(void);

/*
  переключает NoteSpectrum и NoteDots
*/
void vis_SwitchNotes(void);




unsigned long getTickCount(void);



#ifdef __cplusplus
  }
#endif

#endif
