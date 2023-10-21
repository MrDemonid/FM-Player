        ideal
        p8086
        model   large, c
        smart           ; генерация оптимального кода
        locals  @@      ; pазpешаем локальные идентефикатоpы
        nojumps         ; запpещаем TASM'у подставлять NOP'ы

        %noincl


public tmr_Init
public tmr_Close
public tmr_SetNewRate
public tmr_Pause


_si     equ <si>
_di     equ <di>


udataseg
  tmrUsed       db ?

  tmrPause      db ?                    ; флаг паузы


codeseg

  tmrIntCount   dw ?
  tmrTimerRate  dd ?


proc tmr_Handler far
        push    ds es si di dx cx bx ax


        mov     ax, [word cs:tmrTimerRate]
        mov     dx, [word cs:tmrTimerRate+2]
        add     [cs:tmrIntCount], ax
        adc     dx, 0
        jz      @@do_userfunc
        ; произошло переполнение, пора вызывать старый хэндлер
        pushf
        db      09Ah                    ; call  far
  tmrOldHandler:
        dd      0                       ; стаpый обpаботчик
        jmp     @@douser
    @@do_userfunc:
        mov     al, 20h                 ; сбpасываем контpолеp пpеpываний
        out     20h, al
    @@douser:
        mov     ax, @DATA
        mov     ds, ax

        cmp     [tmrPause], 0
        jne     @@exit

        cmp     [tmrUsed], 0
        jne     @@exit
        inc     [tmrUsed]               ; защита от повторного входа

        db      09Ah                    ; call  far
  tmrFunc:
        dd      0                       ; пользовательская функция

        dec     [tmrUsed]

    @@exit:
        pop     ax bx cx dx di si es ds
        iret
endp


proc tmr_Init uses ds _si _di, @@func: dword, @@timerval: dword
        mov     [tmrPause], 0
        mov     [tmrUsed], 0
        push    cs
        pop     ds
        ; tmrFunc = func
        mov     ax, [word @@func]
        mov     dx, [word @@func+2]
        mov     [word tmrFunc], ax
        mov     [word tmrFunc+2], dx
        ; tmrIntCount=0
        xor     ax, ax
        mov     [word tmrIntCount], ax
        ; tmrTimerRate=timerval
        mov     ax, [word @@timerval]
        mov     dx, [word @@timerval+2]
        mov     [word tmrTimerRate], ax
        mov     [word tmrTimerRate+2], dx
        ; tmrOldHandler = getvect(0x08);
        mov     ax, 3508h
        int     21h
        mov     [word tmrOldHandler], bx
        mov     [word tmrOldHandler+2], es
        ; setvect(0x08, &tmr_Handler);
        mov     ax, 2508h
        lea     dx, [tmr_Handler]
        int     21h
        ; outportb(0x43, 0x34);
        cli
        mov     dx, 43h
        mov     al, 34h
        out     dx, al
        ; outportb(0x40, tmrTimerRate);
        sub     dl, 3
        mov     ax, [word @@timerval]
        out     dx, al
        ; outportb(0x40, (tmrTimerRate>>8));
        xchg    al, ah
        out     dx, al
        sti
        or      ax, -1
        ret
endp


proc tmr_Close uses ds _si _di
        ; setvect(0x08, tmrOldHandler);
        mov     ax, [word cs:tmrOldHandler+2]
        mov     ds, ax
        mov     dx, [word cs:tmrOldHandler]
        mov     ax, 2508h
        int     21h
        ; outportb(0x43, 0x34);
        cli
        mov     dx, 43h
        mov     al, 34h
        out     dx, al
        ; outportb(0x40, 0x00);
        xor     al, al
        sub     dl, 3
        out     dx, al
        jmp     $+2
        out     dx, al
        sti
        ret
endp


proc tmr_SetNewRate, @@val: dword
        ; outportb(0x43, 0x34);
        cli
        mov     dx, 43h
        mov     al, 34h
        out     dx, al
        ; tmrTimerRate = val;
        mov     ax, [word @@val]
        mov     cx, [word @@val+2]
        mov     [word cs:tmrTimerRate], ax
        mov     [word cs:tmrTimerRate+2], cx
        ; outportb(0x40, tmrTimerRate);
        sub     dl, 3
        out     dx, al
        ; outportb(0x40, tmrTimerRate>>8);
        xchg    al, ah
        out     dx, al
        sti
        ret
endp


proc tmr_Pause
        mov     al, [tmrPause]
        xor     al, 1
        mov     [tmrPause], al
        ret
endp


end
