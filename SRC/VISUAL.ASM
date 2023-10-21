; ------------------------------------------
; оптимизированные функции модуля VISUAL.CPP
; ------------------------------------------

        ideal
        p8086
        model   large, c
        smart           ; генерация оптимального кода
        locals  @@      ; pазpешаем локальные идентефикатоpы
        jumps           ; TASM сам оптимизирует условные переходы

        %noincl

;
; частоты   (в формате: частота x 1000)
;
NORMALIZE       = 60000 ; Timer frequency to normalize to

EQUAL_REFRESH   = 30000 ; частота обновления полос эквалайзера (30 fps)
COUNTER_REFRESH = 16000 ; частота обновления счетчика ленты (16 fps)
NOTESPEC_REFRESH= 60000 ; частота обновления Note spectrum
TAPE_REFRESH    = 10000 ; частота обновления кассетника


LEN_EQUAL       = 20    ; длина эквалайзера в символах (!!!! заменить на size или length)
LEN_VOLUMES     = 32    ; длина громкости (в позициях)

LEN_NOTE_SPC    = 72    ; 12 нот по 8 октав (исп. 9 нот по 8 октав)
LEN_NOTE_DOTS   = 648   ; 9 каналов, 12 нот по 8 октав (исп. 9 нот по 8 октав)

TAPE_LENGTH     = 40    ; ширина ленты в катушке
TAPE_FCHAR      = 200   ; символы пленки

BTN_NUMKEYS     = 6


; координаты для элементов панелей (относительно начала координат панелей)
VOLBAR_X        = 28    ; начало панели бегунков громкости каналов
VOLBAR_Y        = 2
NOTEBAR_X       = 39    ; начало панели нот и октав каналов
EQUALBAR_X      = 44    ; начало панели эквалайзера
INSTRBAR_X      = 66    ; начало панели инструментов каналов

MAGTITLE_X      = 2     ; координаты названия музыки кассетника
MAGTITLE_Y      = 2
MAGTITLE_WIDTH  = 21    ; макс. длина названия музыки
MAGBOB1_X       = 7     ; координата левой бобины кассетника
MAGTAPE_X       = 10    ; координата пленки кассетника
MAGBOB2_X       = 18    ; координата правой бобины кассетника
MAGBOB_Y        = 4

TEMPO_X         = 2     ; координата регулятора скорости
TEMPO_Y         = 10
VOLUME_X        = 15    ; координата регулятора громкости

BUTTONS_X       = 33    ; начало группы кнопок
BUTTONS_Y       = 12

CURTIME_X       = 15    ; координата текущего времени
CURTIME_Y       = 13
JOBTIME_X       = 70    ; координата времени работы проигрывателя

SPECTRBAR_X     = 4     ; начало панелей NoteSpectrum и NoteDots
SPECTRBAR_Y     = 2





struc TXTBMP
    Width       dw ?
    Height      dw ?
ends



macro LDS_ par:REST
  if @DataSize ne 0
        lds     &par
  else
        mov     &par
  endif
endm

macro LES_ par:REST
  if @DataSize ne 0
        les     &par
        _es     equ <es>
        _seges  equ <es:>
  else
        mov     &par
        _es     equ <>
        _seges  equ <>
  endif
endm


_si     equ <si>
_di     equ <di>


extrn noteType  : byte      ; флаг (0 - NoteSpectrum, 1 - NoteDots)
extrn noteSpec  : byte:LEN_NOTE_SPC  ; [LEN_NOTE_SPC]
extrn noteDots  : byte:LEN_NOTE_DOTS ; [LEN_NOTE_DOTS]
extrn chanOctave: byte:18   ; [18] текущая октава для каждого канала
extrn chanNote  : byte:18   ; [18] текущая нота для каждого канала
extrn chanInst  : dword:18     ; [18] имя инструмента для каждого канала



extrn imgHelp       : ptr TXTBMP
extrn imgPlayer     : ptr TXTBMP
extrn imgFile       : ptr TXTBMP
extrn imgNoteSpc    : ptr TXTBMP
extrn imgNoteDot    : ptr TXTBMP
extrn imgBtnRel     : ptr TXTBMP
extrn imgBtnPush    : ptr TXTBMP


extrn strlen        : proc

extrn opl_getkeyon  : proc
extrn opl_getchanvol: proc
extrn opl_getvolume : proc

extrn vid_ScrWidth  : proc
extrn vid_ScrHeight : proc
extrn vid_GetAddress: proc
extrn vid_PutText   : proc
extrn vid_PutSprite : proc
extrn vid_FillRect  : proc
extrn vid_PutTime   : proc
extrn vid_SetFont   : proc

extrn CPP F_LUDIV@  : proc      ; деление long/long

public vis_Initialize
public vis_SetRate

public vis_Begin
public vis_Stop

public vis_Update

public vis_ShowTime
public vis_ReDraw

public vis_ReleaseButton
public vis_PushButton

public vis_ToggleDirectCnt
public vis_SetTempo
public vis_SetVolume
public vis_ToggleHelp
public vis_SwitchNotes


public getTickCount


; флаг видимости панели справки
public helpVisible

; координата панелей по Y (X равен 0)
public panHelpY
public panPlayerY
public panFileY

; счетчик длины ленты
public cnt_direction






dataseg
  numBitmap     db 078h,08Ch,094h,0B4h,0A4h,0C4h,078h,000h   ; char 0
                db 018h,070h,010h,010h,010h,010h,07Ch,000h   ; char 1
                db 078h,084h,004h,038h,040h,084h,0FCh,000h   ; char 2
                db 078h,084h,004h,038h,004h,084h,078h,000h   ; char 3
                db 01Ch,024h,044h,084h,0FEh,004h,00Eh,000h   ; char 4
                db 0FCh,080h,0F8h,004h,004h,084h,078h,000h   ; char 5
                db 038h,040h,080h,0B8h,0C4h,084h,078h,000h   ; char 6
                db 0FCh,084h,004h,008h,010h,010h,010h,000h   ; char 7
                db 078h,084h,084h,078h,084h,084h,078h,000h   ; char 8
                db 078h,084h,084h,07Ch,004h,008h,070h,000h   ; char 9
                db 078h,08Ch,094h,0B4h,0A4h,0C4h,078h,000h   ; char 0

  ; символы бобин кассетника
  bob_Image     db 184, 183, 182, 181

  ; цвета фона полос эквалайзера (длиной LEN_EQUAL)
  eqlCols       db 50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,52,52,52,52,52
  ; имена нот для эквалайзера
  noteName      db 'C#','D-','D#','E-','F-','F#','G-','G#','A-','A#','B-','C-','XX'
  noteEmpty     db '  '
  ; символы "регуляторов" громкости каналов эквалайзера
  imgVolume     db 240, 242, 244, 246


  szNULL        db 0

udataseg
  totalms       dd ?        ; длина текущей музыки в мс

  ; панель помощи
  helpVisible   db ?        ; флаг показа панели помощи

  ; эквалайзер и бегунки громкости каналов
  eql_update    db ?        ; флаг обновившихся данных эквалайзера и инструментов
  eql_wait      dw ?        ; счетчик времени до апдейта дорожек эквалайзера
  eql_step      dw ?        ; шаг счетчика времени
  chnEqualizer  db 9 dup(?) ; эквалайзер для 9 каналов
  chnVolumes    db 9 dup(?) ; громкость для каждого канала
  chnOldVolumes db 9 dup(?) ; текущая громкость для каждого канала

  offsEqual     dw ?        ; адрес видеопамяти панели эквалайзера
  offsNote      dw ?        ; адрес видеопамяти note bar
  offsVol       dw ?        ; адрес видеопамяти volume bar
  offsInstr     dw ?        ; адрес видеопамяти панели названий инструментов

  ; кассетник
  tap_update    db ?
  tap_pos       db ?
  tap_wait      dw ?        ; счетчик времени до апдейта счетчика ленты
  tap_step      dw ?        ; шаг счетчика времени
  tap_koef      dw ?        ; соотношения времени музыки к ширине ленты в "бобинах"
  tap_tape      db ?        ; остаток ленты на левой катушке

  tapOffs1      dw ?        ; адрес левой катушки
  tapOffs2      dw ?        ;
  tapOffsTape   dw ?        ; адрес ленты левой катушки

  tapTitleX     dw ?        ; координаты строки с названием проигрываемой музыки
  tapTitleY     dw ?
  tapTitleWidth dw ?        ; макс. длина строки с названием музыки

  ; счетчик длины ленты
  cnt_update    db ?        ; флаг обновившихся данных счетчика ленты
  cnt_wait      dw ?        ; счетчик времени до апдейта счетчика ленты
  cnt_step      dw ?        ; шаг счетчика времени
  cnt_direction db ?        ; направление счета: 0-увеличение, 1-уменьшение
  cnt_bitmap    db 3*8 dup(?); текущие битмапы для каждой цифры счетчика

  ; панель note specrum и note dots
  spc_update    db ?        ; флаг обновившихся данных note spectrum
  spc_wait      dw ?        ; счетчик времени до апдейта note spectrum
  spc_step      dw ?        ; шаг счетчика времени

  offsNoteSpec  dw ?        ; адрес видеопамяти note spectrum

  ; регуляторы громкости и темпа
  volOffs       dw ?        ; адрес видеопамяти регулятора громкости
  tempoOffs     dw ?        ; адрес видеопамяти регулятора скорости
  volOld        db ?
  tempoOld      db ?
  tempoNew      db ?

  ; время
  startTicks    dd ?        ; время запуска программы
  curTimeX      dw ?        ; координаты текущего времени
  fbTimeX       dw ?        ; и прошедшего времени
  timeY         dw ?        ; координата панелей времени по Y
  tempo_update  db ?        ; флаг обновившихся данных регулятора скорости
  volume_update db ?        ; флаг обновившихся данных регулятора громкости

  ; кнопки
  offsButtons   dw ?        ; адрес видеопамяти блока кнопок


  ; координата панелей по Y (X равен 0)
  panHelpY      dw ?
  panPlayerY    dw ?
  panFileY      dw ?







codeseg


proc getTickCount
        xor     ax, ax
        mov     es, ax
        mov     bx, 46Ch
        mov     ax, [es:bx]
        mov     dx, [es:bx+2]
        ret
endp


proc vis_Initialize uses _si _di
        ; расчитываем координаты панелей, в зависимости от высоты экрана
        call    vid_ScrHeight
        LES_    bx, [imgHelp]
        mov     si, [(TXTBMP _seges bx).Height]     ; si = imgHelp->Height
        LES_    bx, [imgPlayer]
        mov     di, [(TXTBMP _seges bx).Height]     ; cx = imgPlayer->Height
        LES_    bx, [imgFile]
        mov     bx, [(TXTBMP _seges bx).Height]     ; bx = imgFile->Height
        add     bx, si
        add     bx, di                          ; bx = total height player
        sub     ax, bx
        shr     ax, 1
        mov     [panHelpY], ax  ; panHelpY = (vid_ScrHeight() - height) >> 1;
        add     si, ax
        mov     [panPlayerY], si; panPlayerY = panHelpY + imgHelp->Height;
        add     di, si
        mov     [panFileY], di  ; panFileY = panPlayerY + imgPlayer->Height;
        ; di = panFileY
        ; si = panPlayerY
        ; задаем координаты для названия музыки
        mov     ax, MAGTITLE_X
        mov     [tapTitleX], ax
        mov     ax, si
        add     ax, MAGTITLE_Y
        mov     [tapTitleY], ax
        mov     ax, MAGTITLE_WIDTH
        mov     [tapTitleWidth], ax
        ;
        add     si, VOLBAR_Y    ; si = panPlayerY+2
        mov     ax, VOLBAR_X
        call    vid_GetAddress, ax, si
        mov     [offsVol], ax
        add     ax, (NOTEBAR_X-VOLBAR_X)*2
        mov     [offsNote], ax
        add     ax, (EQUALBAR_X-NOTEBAR_X)*2+1 ; +1 для смещения на атрибуты
        mov     [offsEqual], ax
        add     ax, (INSTRBAR_X-EQUALBAR_X)*2-1; -1 удаляем смещение на атрибуты
        mov     [offsInstr], ax
        ; координаты для кассетника
        add     si, MAGBOB_Y-VOLBAR_Y   ; si = panPlayerY+4
        mov     ax, MAGBOB1_X
        call    vid_GetAddress, ax, si
        mov     [tapOffs1], ax
        add     ax, (MAGTAPE_X-MAGBOB1_X)*2
        mov     [tapOffsTape], ax
        add     ax, (MAGBOB2_X-MAGTAPE_X)*2
        mov     [tapOffs2], ax
        ; координаты для регуляторов темпа и громкости
        add     si, TEMPO_Y-MAGBOB_Y    ; si = panPlayerY+10
        mov     ax, TEMPO_X
        call    vid_GetAddress, ax, si
        mov     [tempoOffs], ax
        add     ax, (VOLUME_X-TEMPO_X)*2
        mov     [volOffs], ax
        mov     al, 32
        mov     [tempoOld], al
        mov     [tempoNew], al
        ; координаты для группы кнопок
        add     si, BUTTONS_Y-TEMPO_Y   ; si = panPlayerY+12
        mov     ax, BUTTONS_X
        call    vid_GetAddress, ax, si
        mov     [offsButtons], ax
        ; координаты для noteSpectrum и noteDots
        add     di, SPECTRBAR_Y
        mov     ax, SPECTRBAR_X
        call    vid_GetAddress, ax, di
        mov     [offsNoteSpec], ax
        ; координаты для времени
        add     si, CURTIME_Y-BUTTONS_Y
        mov     [timeY], si             ; timeY = panPlayerY + 13
        mov     ax, CURTIME_X
        mov     [curTimeX], ax
        mov     ax, JOBTIME_X
        mov     [fbTimeX], ax
        call    getTickCount
        mov     [word startTicks+2], dx
        mov     [word startTicks], ax
        ;
        call    opl_getvolume
        mov     [volOld], al
        mov     al, TAPE_LENGTH
        mov     [tap_tape], al
        ; "обнуляем" счетчик ленты
        lea     di, [cnt_bitmap]
        lea     si, [numBitmap]
        mov     cx, 8
    @@fillcnt:
        lodsb
        mov     [di], al
        mov     [di+8], al
        mov     [di+16], al
        inc     di
        loop    @@fillcnt
        ; выводим панель проигрывателя и кнопки
        call    vid_PutSprite, [imgPlayer], cx, [panPlayerY]
        xor     si, si
    @@relbtn:
        call    vis_ReleaseButton, si
        inc     si
        cmp     si, BTN_NUMKEYS
        jb      @@relbtn
        ; завершаем инициализацию
        call    vis_Stop
        ret
endp



proc vis_InitData uses _di, @@isOldVol: byte
        push    ds
        pop     es
        xor     bx, bx
        xor     ax, ax
        mov     dl, 80h
        mov     cx, 9
    @@fillchn:
        mov     [chnEqualizer+bx], al
        mov     [chnVolumes+bx], al
        cmp     [@@isOldVol], 0
        jz      @@1
        mov     [chnOldVolumes+bx], al
      @@1:
        mov     [chanOctave+bx], dl
        mov     [chanOctave+9+bx], dl
        inc     bx
        loop    @@fillchn

        mov     cx, (length chanInst) * (size chanInst)
        lea     di, [chanInst]
        rep     stosb
        ; инициализируем noteSpec и noteDots
        mov     cx, (length noteSpec) * (size noteSpec)
        lea     di, [noteSpec]
        rep     stosb
        mov     cx, (length noteDots) * (size noteDots)
        lea     di, [noteDots]
        rep     stosb
        ; выставляем флаги для обновления визуальных компонентов
        inc     al
        mov     [tap_update], al
        mov     [cnt_update], al
        mov     [eql_update], al
        mov     [tempo_update], al
        mov     [volume_update], al
        cmp     [noteType], 1
        jbe     @@2
        dec     al
      @@2:
        mov     [spc_update], al
        ret
endp

;
; настройка визуализатора на заданную частоту прерываний
;
proc vis_SetRate uses _si _di, @@rate: dword
        xor     ax, ax
        mov     [eql_wait], ax
        mov     [spc_wait], ax
        mov     [cnt_wait], ax
        mov     [tap_wait], ax

        mov     si, [word @@rate]
        mov     di, [word @@rate+2]
        ; частота обновления для эквалайзера
        xor     ax, ax
        mov     dx, EQUAL_REFRESH
        call    F_LUDIV@ STDCALL, dx ax, di si
        mov     [eql_step], ax
        ; для счетчика ленты
        xor     ax, ax
        mov     dx, COUNTER_REFRESH
        call    F_LUDIV@ STDCALL, dx ax, di si
        mov     [cnt_step], ax
        ; для кассетника
        xor     ax, ax
        mov     dx, TAPE_REFRESH
        call    F_LUDIV@ STDCALL, dx ax, di si
        mov     [tap_step], ax
        ; частота обновления для note spectrum
        xor     ax, ax
        cmp     [noteType], 1
        ja      @@1
        mov     dx, NOTESPEC_REFRESH
        call    F_LUDIV@ STDCALL, dx ax, di si
    @@1:
        mov     [spc_step], ax
        ret
endp

;proc vis_SetRate uses _si _di, @@rate: dword
;        xor     ax, ax
;        mov     [eql_wait], ax
;        mov     [spc_wait], ax
;        mov     [cnt_wait], ax
;        mov     [tap_wait], ax
;        ; частота обновления для эквалайзера
;        xor     ax, ax
;        mov     dx, EQUAL_REFRESH
;        call    F_LUDIV@ STDCALL, dx ax, [@@rate]
;        mov     [eql_step], ax
;        ; частота обновления для note spectrum
;        xor     ax, ax
;        mov     dx, NOTESPEC_REFRESH
;        call    F_LUDIV@ STDCALL, dx ax, [@@rate]
;        mov     [spc_step], ax
;        ; для счетчика ленты
;        xor     ax, ax
;        mov     dx, COUNTER_REFRESH
;        call    F_LUDIV@ STDCALL, dx ax, [@@rate]
;        mov     [cnt_step], ax
;        ; для кассетника
;        xor     ax, ax
;        mov     dx, TAPE_REFRESH
;        call    F_LUDIV@ STDCALL, dx ax, [@@rate]
;        mov     [tap_step], ax
;        ret
;endp


;#############################################################################
;#############################################################################
;####################### Обновление данных визуализации ######################
;#############################################################################
;#############################################################################

;
; обновление всех данных
;
proc vis_Update uses _si _di, @@time: dword
        call    vis_UpdateCounter, [@@time]
        call    vis_UpdateEqual
        call    vis_UpdateTape, [@@time]
        call    vis_UpdateNoteSpectrum
        ret
endp

;
; обновление полос эквалайзера
;
proc vis_UpdateEqual near
    local @@i
        mov     ax, [eql_wait]
        add     ax, [eql_step]
        mov     [eql_wait], ax
        jnc     @@exit
        ; обновляем данные эквалайзера

        xor     ax, ax
        mov     [@@i], ax
        lea     si, [chnEqualizer]
        lea     di, [chnVolumes]
    @@loop:
        push    si di
        call    opl_getkeyon, [@@i]
        or      al, al
        jz      @@no_key
        ; задаем новое значение каналу
        call    opl_getchanvol, [@@i]
        mov     bx, 128
        pop     di si
        sub     bx, ax          ; BX = 128 - opl_getchanvol(i)
        ; t = b / (128 / LEN_EQUAL);
        mov     ax, 0AAABh
        mul     bx
        shr     dx, 1
        shr     dx, 1           ; dx = BX / (128 / LEN_EQUAL)
        cmp     dl, LEN_EQUAL
        jbe     @@1
        mov     dl, LEN_EQUAL
      @@1:
        mov     [si], dl        ; chnEqualizer[i] = t
        ; t = b / (128 / LEN_VOLUMES);
        shr     bx, 1
        shr     bx, 1          ; BX = BX / (128 / LEN_VOLUMES)
        cmp     bl, LEN_VOLUMES
        jb      @@2
        mov     bl, LEN_VOLUMES-1
      @@2:
        mov     [di], bl        ; chnVolumes[i] = t
        jmp     @@next

    @@no_key:
        ; нет нового значения на канале, просто уменьшаем старое
        pop     di si
        mov     al, [si]
        dec     al              ; chnEqualizer[i]--
        js      @@next
        mov     [si], al

    @@next:
        mov     ax, [@@i]
        inc     ax
        cmp     ax, 9
        jae     @@done
        mov     [@@i], ax
        inc     di
        inc     si
        jmp     @@loop

    @@done:
        inc     [eql_update]
    @@exit:
        ret
endp


;
; обновление note spectrum
;
proc vis_UpdateNoteSpectrum near
        mov     ax, [spc_wait]
        add     ax, [spc_step]
        mov     [spc_wait], ax
        jnc     @@exit
        ; уточняем что нужно обновить
        mov     al, [noteType]
        or      al, al
        jnz     @@dots
        ; note spectrum
        mov     cx, LEN_NOTE_SPC/2
        lea     si, [noteSpec]
        jmp     @@update
    @@dots:
        mov     cx, LEN_NOTE_DOTS/2
        lea     si, [noteDots]

    @@update:
        mov     ax, [si]
        or      al, al
        jz      @@nextlo
        sub     al, 2
        mov     [si], al
    @@nextlo:
        or      ah, ah
        jz      @@nexthi
        sub     ah, 2
        mov     [si+1], ah
    @@nexthi:
        add     si, 2
        loop    @@update

    @@done:
        inc     [spc_update]
    @@exit:
        ret
endp



;
; обновление кассетника
;
proc vis_UpdateTape near, @@time: dword
        mov     ax, [tap_wait]
        add     ax, [tap_step]
        mov     [tap_wait], ax
        mov     cx, [tap_koef]
        mov     ax, [word @@time]
        mov     dx, [word @@time+2]
        jc      @@doupdate
        ; последний апдейт должен пройти, поэтому проверяем time
        cmp     dx, [word totalms+2]
        jne     @@islater
        cmp     ax, [word totalms]
      @@islater:
        jb      @@exit          ; --> (time < totalms)

    @@doupdate:
        ; обновляем "ленту" мафона
        div     cx              ; t = time / tap_koef;
        mov     bl, TAPE_LENGTH
        cmp     al, bl
        jbe     @@1
        mov     al, bl
    @@1:
        sub     bl, al
        mov     [tap_tape], bl  ; tap_tape = 40 - t;
        ; обновляем "бобины"
        mov     al, [tap_pos]
        inc     al              ; tap_pos++
        cmp     al, 2
        jbe     @@2
        xor     al, al
    @@2:
        mov     [tap_pos], al
    @@done:
        inc     [tap_update]
    @@exit:
        ret
endp



; на входе:
;       di      - &cnt_bitmap[i][0]
;       ax      - цифра (num)
;       bx      - смещение битмапа по вертикали (pos)
proc cnt_SetBitmap near uses cx ;si ; (dx, ax, bx)
        mov     si, ax
        mov     cx, 3
        shl     si, cl
        add     si, bx
        add     si, offset numBitmap    ; si = &numBitmap[num*8+pos]
        inc     cx
        rep     movsw
        ret
endp




;
;  обновление счетчика ленты
;  на входе:
;    time - время проигрывания в мс.
;
proc vis_UpdateCounter near, @@time: dword
        mov     ax, [cnt_wait]
        add     ax, [cnt_step]
        mov     [cnt_wait], ax
        mov     ax, [word @@time]
        mov     dx, [word @@time+2]
        mov     bx, [word totalms+2]
        mov     cx, [word totalms]
        jc      @@doupdate
        ; последний апдейт должен пройти, поэтому проверяем time
        cmp     dx, bx
        jne     @@islater
        cmp     ax, cx
      @@islater:
        jb      @@exit          ; --> (time < totalms)

    @@doupdate:
        cmp     [cnt_direction], 0
        jz      @@calckcnt
        ; счетчик в реверсном режиме, поэтому берем остаток времени с конца
        cmp     dx, bx
        jne     @@
        cmp     ax, cx
      @@:
        jb      @@invtime
        ; time = 0
        xor     ax, ax
        xor     dx, dx
        jmp     @@calckcnt
      @@invtime:
        ; time = totalms - time
        sub     cx, ax
        sbb     bx, dx
        mov     ax, cx
        mov     dx, bx

    @@calckcnt:
        ; dx:ax - time
        ; старшая часть time всегда меньше 1000, поэтому
        ; деление можно упростить
        mov     bx, 1000
        div     bx              ; ax - время в секундах
        mov     cx, dx          ; cx - остаток
        ; получаем младший разряд счетчика
        mov     bx, 10
        xor     dx, dx
        div     bx
        mov     si, dx          ; c3 = sec % 10
        ; получаем средний разряд
        xor     dx, dx
        div     bx
        mov     di, dx          ; c2 = (sec / 10) % 10
        ; получаем старший разряд
        xchg    ax, cx          ; cx = c1 = sec / 100
                                ; ax = mod
        ; получаем дробную часть младшего разряда
        xor     dx, dx
        mov     bx, 125
        div     bx
        mov     bx, ax          ; mod = mod / 125

        mov     ax, SEG cnt_bitmap
        mov     dx, di
        mov     es, ax
        mov     di, offset cnt_bitmap+2*8
        ; cx - c1
        ; dx - c2
        ; si - c3
        ; bx - mod
        ; es:di = &cnt_bitmap[2][0]
        mov     ax, si
        cmp     si, 9
        jne     @@cntnol
        ; c3 == 9
        call    cnt_SetBitmap     ; , 2, c3, mod
        sub     di, 8+8           ; es:di = &cnt_bitmap[1][0]
        mov     ax, dx
        call    cnt_SetBitmap     ; , 1, c2, mod
        sub     di, 8+8           ; es:di = &cnt_bitmap[0][0]
        mov     ax, cx
        cmp     dx, 9
        jnz     @@cnt1
        call    cnt_SetBitmap     ; , 0, c1, mod
        jmp     @@done
      @@cnt1:
        xor     bx, bx
        call    cnt_SetBitmap     ; , 0, c1, 0
        jmp     @@done


    @@cntnol:
        call    cnt_SetBitmap     ; , 2, c3, mod
        sub     di, 8+8           ; es:di = &cnt_bitmap[1][0]
        mov     ax, dx
        xor     bx, bx
        call    cnt_SetBitmap     ; , 1, c2, 0
        sub     di, 8+8           ; es:di = &cnt_bitmap[0][0]
        mov     ax, cx
        call    cnt_SetBitmap     ; , 0, c1, 0

    @@done:
        inc     [cnt_update]
    @@exit:
        ret
endp
;       di      - &cnt_bitmap[i][0]
;       ax      - цифра (num)
;       bx      - смещение битмапа по вертикали (pos)




;#############################################################################
;#############################################################################
;####################### Отрисовка данных визуализации #######################
;#############################################################################
;#############################################################################


;
; перерисовка кассетника
;
proc vis_ReDrawTape near
        ; обновляем бобины
        mov     ax, 0B800h
        mov     es, ax
        mov     di, [tapOffs1]
        xor     bh, bh
        mov     bl, [tap_pos]
        mov     al, [bob_Image+bx]
        stosb
        mov     di, [tapOffs2]
        inc     bx
        cmp     bx, 2
        jbe     @@put2
        xor     bx, bx
    @@put2:
        mov     al, [bob_Image+bx]
        stosb

        ; прорисовываем пленку
        mov     bx, TAPE_LENGTH
        mov     di, [tapOffsTape]
        xor     ch, ch
        mov     cl, [tap_tape]
        mov     dx, cx
        shr     cx, 1
        sub     bx, dx                  ; bx - ширина пленки правой катушки
        shr     cx, 1
        and     dl, 07h                 ; dl = ax % 8
        mov     al, TAPE_FCHAR+8
        shr     cx, 1
        jz      @@loop2
    @@loop1:
        stosb
        inc     di
        loop    @@loop1
    @@loop2:
        mov     al, TAPE_FCHAR
        add     al, dl
        stosb
        inc     di
        add     al, 8
        mov     cx, bx
        jcxz    @@done
        stosb
        inc     di
        dec     cx
        jz      @@done
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        jz      @@done
        mov     al, TAPE_FCHAR+8
    @@loop3:
        stosb
        inc     di
        loop    @@loop3
    @@done:
        ret
endp


;
; полная перерисовка эквалайзера
;
proc vis_ReDrawEqual near
  local @@wincr
        call    vid_ScrWidth
        shl     ax, 1
        mov     [@@wincr], ax
        mov     ax, 0B800h
        mov     es, ax
        ; перерисовываем эквалайзер
        mov     di, [offsEqual]
        xor     bx, bx
    @@loop:
        push    di
        mov     si, offset eqlCols
        xor     ch, ch
        mov     cl, [chnEqualizer+bx]
        mov     dx, cx
        jcxz    @@hide
    @@show:
        lodsb
        or      al, 08h
        stosb
        inc     di
        loop    @@show
    @@hide:
        mov     cx, LEN_EQUAL
        sub     cx, dx
        jz      @@next
    @@hideloop:
        movsb
        inc     di
        loop    @@hideloop
    @@next:
        pop     di
        add     di, [@@wincr]
        inc     bx
        cmp     bx, 9
        jb      @@loop

        ; перерисовываем ноты
        xor     bx, bx
        mov     di, [offsNote]
    @@note_loop:
        push    di
        xor     ah, ah
        mov     cl, [chanOctave+bx]
        and     cl, 87h                   ; выделяем октаву и старший бит
        mov     al, [chanNote+bx]
        add     cl, '0'
        shl     ax, 1
        mov     si, offset noteName       ; ds:si - note name
        add     si, ax                    ; [si] = noteName[chanNote[i]]
        cmp     cl, 7Fh
        jb      @@note_show
        mov     cl, ' '
        mov     si, offset noteEmpty
    @@note_show:
        movsb
        inc     di
        movsb
        inc     di
        mov     al, cl
        stosb
        pop     di
        inc     bx
        add     di, [@@wincr]
        cmp     bx, 9
        jb      @@note_loop

        ; обновляем бегунки громкости каналов
        xor     si, si
        mov     cx, [offsVol]
    @@vol_loop:
        mov     di, cx
        ; удаляем предыдущий
        xor     bh, bh
        mov     bl, [chnOldVolumes + si]
        mov     dl, [imgVolume]
        inc     dl
        mov     [es:di+bx], dl
        mov     [es:di+bx+2], dl
        ; расчитываем координаты нового
        xor     ah, ah
        mov     al, [chnVolumes + si]
        mov     bx, ax
        shr     ax, 1
        shr     ax, 1       ; al = chnVolumes[i] / 4
        and     bx, 3       ; bx = chnVolumes[i] % 4
        shl     ax, 1       ; + аттрибуты символов
        mov     [chnOldVolumes + si], al
        add     di, ax
        mov     dl, [imgVolume + bx]
        mov     [es:di], dl
        inc     dl
        mov     [es:di+2], dl
        inc     si
        add     cx, [@@wincr]
        cmp     si, 9
        jb      @@vol_loop

        ret
endp


;
; полная перерисовка инструментов
;
proc vis_ReDrawInstruments near
  local @@wincr, @@idx
        call    vid_ScrWidth
        sub     ax, 12
        shl     ax, 1
        mov     [@@wincr], ax
        mov     ax, 0B800h
        mov     es, ax

        mov     bx, offset chanInst
        mov     di, [offsInstr]         ; di - video address
        mov     [@@idx], 9
    @@loop:
        push    ds
        mov     cx, 12                  ; cx - длина поля имени инструмента
        mov     ah, ' '                 ;
        lds     si, [dword bx]          ; ds:si = chanInst[i]
        mov     dx, ds
        or      dx, si
        jz      @@empty                 ; --> выводим пустую строку
        ; выводим название инструмента
    @@putinst:
        lodsb
        dec     cx
        js      @@next
        or      al, al
        jz      @@fill
        mov     [es:di], al
        add     di, 2
        jmp     @@putinst
    @@empty:
        dec     cx
    @@fill:
        mov     [es:di], ah
        add     di, 2
        dec     cx
        jns     @@fill
    @@next:
        pop     ds
        add     di, [@@wincr]
        add     bx, 4
        dec     [@@idx]
        jnz     @@loop

        ret
endp

;
; полная перерисовка note spectrum
;
proc vis_ReDrawNoteSpectrum near uses bp
        call    vid_ScrWidth
        mov     dx, ax
        shl     dx, 1           ; dx = screen width
        mov     ax, 0B800h
        mov     es, ax

        ; перерисовываем эквалайзер
        mov     di, [offsNoteSpec]
        lea     si, [noteSpec]
        mov     bp, di
        mov     ch, 6*12        ; ch = ширина "окна"
    @@loop:
        add     bp, 2
        lodsb
        mov     cl, 3
        mov     bl, al
        mov     bh, 72
        sub     bh, al
        shr     bh, cl          ; bh = (72 - noteSpec[i]) / 8
        or      bh, bh
        jz      @@term
        ; заполняем "пустоту"
        mov     al, ' '
    @@clear:
        mov     [es:di], al
        add     di, dx
        dec     bh
        jnz     @@clear
    @@term:
        mov     bh, bl
        shr     bl, cl          ; bl = noteSpec[i] / 8
        and     bh, 7           ; nh = noteSpec[i] - (bl*8)
        or      bh, bh
        jz      @@isfill
        add     bh, 0F8h-1
        mov     [es:di], bh
        add     di, dx
    @@isfill:
        or      bl, bl
        jz      @@next
        mov     al, 0FFh
    @@fill:
        mov     [es:di], al
        add     di, dx
        dec     bl
        jnz     @@fill
    @@next:
        mov     di, bp
        dec     ch
        jnz     @@loop

        ret
endp


;
; полная перерисовка note dots
;
proc vis_ReDrawNoteDots near
  local @@scrIncr
        call    vid_ScrWidth
        sub     ax, 72
        shl     ax, 1
        mov     [@@scrIncr], ax ; scrIncr = (vid_ScrWidth() - 72) << 1

        mov     ax, 0B800h
        mov     es, ax

        ; перерисовываем эквалайзер
        mov     di, [offsNoteSpec]
        lea     si, [noteDots]
        mov     ch, 9           ; 9 каналов
        mov     cl, 4

    @@hloop:
        mov     bl, 72
    @@wloop:
        lodsb
        shr     al, cl
        add     al, 11
        stosb
        inc     di
        dec     bl
        jnz     @@wloop
        add     di, [@@scrIncr]
        dec     ch
        jnz     @@hloop

        ret
endp



;
; перерисовка регулятора громкости
;
proc vis_ReDrawVolume near
        call    opl_getvolume
        mov     dl, al                  ; dl - global volume

        mov     ax, 0B800h
        mov     es, ax
        ; удаляем со старой позиции
        mov     di, [volOffs]
        mov     si, di                  ; save di
        xor     bh, bh
        mov     bl, [volOld]
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shl     bx, 1                   ; bx = volume / 16
        add     di, bx
        mov     al, [imgVolume]
        inc     al
        stosb
        inc     di
        stosb
        ; рисуем на новой позиции
        mov     di, si
        mov     [volOld], dl
        xor     dh, dh
        mov     bx, dx
        shr     dx, 1
        shr     dx, 1
        shr     dx, 1
        shl     dx, 1
        add     di, dx
        and     bx, 7
        shr     bx, 1
        mov     al, [imgVolume + bx]
        stosb
        inc     di
        inc     al
        stosb
        ret
endp


;
; перерисовка регулятора скорости воспроизведения
;
proc vis_ReDrawTempo near
        mov     ax, 0B800h
        mov     es, ax
        ; удаляем старый
        mov     di, [tempoOffs]
        mov     bl, [tempoOld]
        xor     bh, bh
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shl     bx, 1                   ; bx = volume / 16
        add     di, bx
        mov     al, [imgVolume]
        inc     al
        stosb
        inc     di
        stosb
        ; рисуем на новой позиции
        mov     di, [tempoOffs]
        xor     bh, bh
        mov     bl, [tempoNew]
        mov     dx, bx
        shr     dx, 1
        shr     dx, 1
        shr     dx, 1
        mov     [tempoOld], bl
        shl     dx, 1
        add     di, dx
        and     bx, 7
        shr     bx, 1
        mov     al, [imgVolume + bx]
        stosb
        inc     di
        inc     al
        stosb
        ret
endp


proc vis_ShowTime
        call    getTickCount            ; dx:ax = ticks
        mov     bx, dx
        mov     cx, ax
        sub     cx, [word startTicks]
        sbb     bx, [word startTicks+2] ; bx:cx = ticks - startTicks
        call    vid_PutTime, [curTimeX], [timeY], dx ax
        call    vid_PutTime, [fbTimeX], [timeY], bx cx
        ret
endp



proc vis_ReDraw uses _si _di
        cmp     [eql_update], 0
        jz      @@1
        ; обновляем эквалайзер
        mov     [eql_update], 0
        call    vis_ReDrawEqual
        call    vis_ReDrawInstruments
    @@1:
        cmp     [spc_update], 0
        jz      @@2
        mov     [spc_update], 0
        cmp     [noteType], 0
        jz      @@spcdraw
        call    vis_ReDrawNoteDots
        jmp     @@2
      @@spcdraw:
        call    vis_ReDrawNoteSpectrum
    @@2:
        cmp     [cnt_update], 0
        jz      @@3
        mov     [cnt_update], 0
        mov     ax, 8
        mov     dx, 1
        mov     cx, 3
        call    vid_SetFont, ds offset cnt_bitmap, ax, dx, cx ; vid_SetFont(&cnt_bitmap[0][0], 8, 0x1, 3);
    @@3:
        cmp     [tap_update], 0
        jz      @@4
        mov     [tap_update], 0
        call    vis_ReDrawTape
    @@4:
        cmp     [tempo_update], 0
        jz      @@5
        mov     [tempo_update], 0
        call    vis_ReDrawTempo
    @@5:
        cmp     [volume_update], 0
        jz      @@done
        mov     [volume_update], 0
        call    vis_ReDrawVolume
    @@done:
        call    vis_ShowTime

        ret
endp




;#############################################################################
;#############################################################################
;############################ Управление кнопками ############################
;#############################################################################
;#############################################################################
;
; отрисовка кнопок
;
proc vis_DrawButton near uses ds si _di, @@img: ptr TXTBMP, @@key: byte
  local @@btWidth
  local @@scrWidth
  local @@scrIncr
        call    vid_ScrWidth
        mov     [@@scrWidth], ax
        mov     di, [offsButtons]
        LDS_    si, [@@img]
        lodsw                   ; ax = img->Width
        mov     bx, ax          ; bx = img->Width
        mov     cl, BTN_NUMKEYS
        div     cl
        xor     ah, ah
        mov     [@@btWidth], ax ; btWidth = img->Width / BTN_NUMKEYS
        mov     cx, [@@scrWidth]
        sub     cx, ax
        add     cx, cx
        mov     [@@scrIncr], cx ; scrIncr = (scrWidth-btWidth) * 2
        sub     bx, ax
        add     bx, bx          ; bx = (img->Width - btnWidth) * 2;
        mov     cl, [@@key]
        mul     cl
        mov     cx, ax
        add     cx, cx          ; cx = key*btnWidth*2
        lodsw
        mov     dx, ax
        mov     ax, 0B800h
        mov     es, ax
        add     di, cx          ; es:di - offsButtons+(key*btnWidth*2)
        add     si, cx          ; ds:si - img->Data[key*btnWidth*2]
    @@loop:
        mov     cx, [@@btWidth]
        rep     movsw
        add     di, [@@scrIncr]
        add     si, bx          ; si += btnIncr
        dec     dx
        jnz     @@loop
        ret
endp


proc vis_ReleaseButton, @@key: byte
        mov     al, [@@key]
        call    vis_DrawButton, [imgBtnRel], ax
        ret
endp

proc vis_PushButton, @@key: byte
        mov     al, [@@key]
        call    vis_DrawButton, [imgBtnPush], ax
        ret
endp



;#############################################################################
;#############################################################################
;########################## Управление визуализацией #########################
;#############################################################################
;#############################################################################

;
; начало визуализации мелодии
; на входе:
;   title     - название мелодии
;   totaltime - длина мелодии в мс
;
proc vis_Begin uses _si _di, @@title: ptr byte, @@totaltime: dword
        ; totalms = totaltime
        mov     ax, [word @@totaltime]
        mov     dx, [word @@totaltime+2]
        mov     [word totalms], ax
        mov     [word totalms+2], dx
        mov     bx, TAPE_LENGTH
        ; кассетник
        cmp     dx, bx
        jb      @@lastdiv
        mov     cx, ax
        mov     ax, dx
        xor     dx, dx
        div     bx
        mov     ax, cx
    @@lastdiv:
        div     bx
        mov     cx, 1
        mov     [tap_koef], ax  ; tap_koef = totaltime / TAPE_LENGTH;
        mov     [tap_pos], ch   ; tap_pos = 0
        mov     [tap_tape], bl  ; tap_tape = TAPE_LENGTH
        ; other datas
        call    vis_InitData, cx

        ; отрисовка title проигрываемого музона
        call    strlen, [@@title]
        mov     bx, [tapTitleWidth]
        cmp     ax, bx
        jbe     @@1
        mov     ax, bx
    @@1:
        sub     bx, ax
        shr     bx, 1
        add     bx, [tapTitleX]
        call    vid_PutText, bx, [tapTitleY], ax, [@@title]

        ; отрисовка панели визуализации
        xor     ax, ax
        cmp     [noteType], 1
        ja      @@exit
        LES_    bx, [imgNoteSpc]
        jb      @@2
        LES_    bx, [imgNoteDot]
    @@2:
        call    vid_PutSprite, _es bx, ax, [panFileY]
    @@exit:
        ret
endp


;
; завершение визуализации мелодии
; (вызывать после сброса OPL)
;
proc vis_Stop
        xor     ax, ax
        call    vis_InitData, ax
        call    vis_ReDraw
        ;скрываем титлы
        lea     bx, [szNULL]
  if @DataSize ne 0
        call    vid_PutText, [tapTitleX], [tapTitleY], [tapTitleWidth], ds bx
  else
        call    vid_PutText, [tapTitleX], [tapTitleY], [tapTitleWidth], bx
  endif
        ret
endp


;
; переключает направление счетчика ленты (на увеличение/уменьшение отсчета)
;
proc vis_ToggleDirectCnt
        mov     al, [cnt_direction]
        xor     al, 1
        mov     [cnt_direction], al
        mov     [cnt_update], 1
        ret
endp

;
; задает новое положение регулятора скорости воспроизведения
;
proc vis_SetTempo, @@tempo: byte
        mov     al, [@@tempo]
        cmp     al, 63
        jbe     @@1
        mov     al, 63
    @@1:
        cmp     al, [tempoNew]
        je      @@done
        mov     [tempoNew], al
        mov     [tempo_update], 1
    @@done:
        ret
endp

;
; задает новое положение регулятора громкости воспроизведения
;
proc vis_SetVolume, @@volume: byte
        mov     al, [@@volume]
        cmp     al, [volOld]
        je      @@done
        mov     [volume_update], 1
    @@done:
        ret
endp

;
; отображает/скрывает справку
;
proc vis_ToggleHelp uses _si _di
        LES_    bx, [imgHelp]
        xor     dx, dx
        mov     cx, [panHelpY]
        mov     al, [helpVisible]
        xor     al, 1
        mov     [helpVisible], al
        jnz     @@hide
        ; показываем панель помощи
        call    vid_PutSprite, _es bx, dx, cx
        jmp     @@done

    @@hide:
        ; скрываем панель помощи
        mov     ax, 0F20h
        call    vid_FillRect, dx, cx, [(TXTBMP _seges bx).Width], [(TXTBMP _seges bx).Height], ax
    @@done:
        ret
endp



;
; переключает NoteSpectrum и NoteDots
;
proc vis_SwitchNotes
        LES_    bx, [imgNoteSpc]
        xor     dx, dx
        mov     al, [noteType]
        cmp     al, 1
        ja      @@exit
        xor     al, 1
        mov     [noteType], al
        jz      @@put
        ; note dots
        LES_    bx, [imgNoteDot]
    @@put:
        call    vid_PutSprite, _es bx, dx, [panFileY]
        mov     [spc_update], 1
    @@exit:
        ret
endp



end



