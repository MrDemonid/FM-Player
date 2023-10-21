#ifndef H_TIMER
#define H_TIMER

#ifdef __cplusplus
  extern "C" {
#endif


int  tmr_Init(void (far *func)(), long timerval);
void tmr_Close(void);
void tmr_SetNewRate(long val);

void tmr_Pause(void);

#ifdef __cplusplus
}
#endif

#endif
