        ideal
        p8086
        model   large, c
        smart           ; генерация оптимального кода
        locals  @@      ; pазpешаем локальные идентефикатоpы
        nojumps         ; запpещаем TASM'у подставлять NOP'ы

        %noincl

_si     equ <si>
_di     equ <di>


IFNDEF TARGET_XT
  WAIT_ADLIB    = 18    ; задержка чуть меньше 23-х микросекунд
ELSE
  WAIT_ADLIB    = 1     ; для XT, поскольку OUT и вызов подпрограммы итак занимает уйму времени
ENDIF


enum CHIP_TYPE          TYPE_OPL2, TYPE_OPL3, TYPE_DUAL_OPL2

; тип таблицы перевода нот в частоту
NOTE_STANDART   = 0     ; стандартный
NOTE_SA2        = 1     ; типа SAT Player
NOTE_S3M        = 2     ; S3M (совпадает с SAT)

public adlPort          ; базовый адрес порта OPL
public currChip         ; номер текущего чипа OPL
public chipType         ; тип чипа OPL (CHIP_TYPE)

public op_table
public note_table


public chanFreq
public chanOctave
public chanNote
public chanInst
public noteSpec
public noteDots
public noteType


public opl_detect
public opl_reset
public opl_setchip
public opl_getchip
public opl_gettype
public opl_setnowrite
public opl_setnotetable

public opl_setquiet
public opl_setvolume
public opl_getvolume

public opl_getchanvol
public opl_getkeyon

public opl_setnotespc


public opl_write

dataseg
  op_table      db 00h, 01h, 02h, 08h, 09h, 0ah, 10h, 11h, 12h

  ; standart adlib note table
  std_notetbl   dw 363,385,408,432,458,485,514,544,577,611,647,686
  ; SA2 compatible adlib note table
  sa2_notetbl   dw 340,363,385,408,432,458,485,514,544,577,611,647


udataseg
  adlPort       dw ?            ; базовый адрес порта OPL
  currChip      db ?            ; номер текущего чипа OPL
  chipType      db ?            ; тип чипа OPL (CHIP_TYPE)

  curVolume     db ?            ; глобальная громкость
  oldVolume     db ?            ; и ее пред. значение
  beQuiet       db ?            ; флаг включения режима "тишины"
  noWrite       db ?            ; флаг отключения записи в порты чипа OPL

  note_table    dw 12 dup (?)   ; таблица для перевода нот в частоту

  keyRegs       db 18*2 dup (?) ; флаги апдейта каналов (для эквалайзера)
  hardVols      db 22*2 dup (?) ; громкость каналов

  ; пока пишутся только первые 9 каналов!
  chanFreq      dw 18 dup (?)   ; текущая частота для каждого канала
  chanOctave    db 18 dup (?)   ; текущая октава для каждого канала
  chanNote      db 18 dup (?)   ; текущая нота для каждого канала
  chanInst      dd 18 dup (?)   ; текущий инструмент для каждого канала

  noteSpec      db 72 dup (?)   ; 12 нот по 8 октав (исп. 9 нот по 8 октав)
  noteDots      db 648 dup (?)  ; 9 каналов, 12 нот по 8 октав (исп. 9 нот по 8 октав)
  noteType      db ?            ; флаг (0 - NoteSpectrum, 1 - NoteDots)


codeseg

IFDEF TARGET_XT
  ; запись в порт Adlib с задержкой в 23 микросекунды
  ; для инициализации карты Adlib
  ; на входе:
  ;       al      - адpес поpта
  ;       ah      - данные
    proc PutFM
            push    ax
            mov     dx, [adlPort]
            mov     cx, 30
        @@wait23us:
            in      al, dx
            loop    @@wait23us
            pop     ax
    endp
ENDIF

macro OutFM
    IFNDEF TARGET_XT
        call    writeFM
    ELSE
        call    PutFM
    ENDIF
endm

; на входе:
;       al      - адpес поpта
;       ah      - данные
proc writeFM uses ax dx
        mov     dx, [adlPort]
        out     dx, al
        ; задержка 3.3 микросекунды
        REPT 5
        in      al, dx
        ENDM
        inc     dx
        mov     al, ah
        out     dx, al
        dec     dx
        ; задержка 23 микросекунды не нужна, она компенсируется другими командами
        REPT WAIT_ADLIB
        in      al, dx
        ENDM
        ret
endp

; на выходе:
;       ax      - 0: OPL not found
;                >0: OPL present
proc hard_Detect
        mov     ax, 6004h
        OutFM
        mov     ah, 80h
        OutFM
        mov     dx, [adlPort]
        in      al, dx
        mov     bh, al
        mov     ax, 0FF02h
        OutFM
        mov     ax, 2104h
        OutFM
        mov     cx, 80
        mov     dx, [adlPort]
    @@wait:
        in      al, dx
        loop    @@wait
        in      al, dx
        mov     bl, al
        mov     ax, 6004h
        OutFM
        mov     ah, 80h
        OutFM
        xor     ax, ax
        and     bx, 0E0E0h
        or      bh, bh
        jnz     @@exit
        cmp     bl, 0C0h
        jne     @@exit
        dec     ax
    @@exit:
        ret
endp



; сброс текущего чипа OPL
proc hard_reset
        xor     ax, ax
        inc     ax
        OutFM                 ; port[0x01] = 0
        mov     al, 4
        OutFM                 ; port[0x04] = 0
        mov     al, 8
        OutFM                 ; port[0x08] = 0
        mov     al, 0BDh
        OutFM                 ; clear misc. register
        xor     bx, bx                  ; bx - index
    @@loop:
        mov     ax, bx
        add     al, 0A0h
        OutFM                 ; freq = 0
        mov     ax, bx
        add     al, 0B0h
        OutFM                 ; key off / freq = 0
        mov     al, [op_table + bx]
        add     al, 80h
        mov     ah, 0FFh
        OutFM                 ; fastest release
        inc     bx
        cmp     bx, 9
        jb      @@loop
        ret
endp


; на выходе:
;       ax      - флаг наличия OPL (chipType - тип чипа OPL)
proc opl_detect
        xor     ax, ax
        mov     [beQuiet], al                ; beQuiet = FALSE
        mov     [noWrite], al                ; noWrite = FALSE
        mov     [curVolume], 63
        call    opl_setchip, ax
        call    hard_detect
        or      ax, ax
        jz      @@exit                  ; OPL не обнаружен
        mov     cl, TYPE_OPL3           ; предполагаем наличие OPL3
        mov     dx, [adlPort]
        in      al, dx
        and     al, 6
        jz      @@done
        ; это OPL2, определяем наличие dual чипа
        mov     al, 1
        call    opl_setchip, ax
        call    hard_Detect
        mov     cl, TYPE_OPL2
        or      ax, ax
        jz      @@done
        mov     cl, TYPE_DUAL_OPL2
    @@done:
        mov     [chipType], cl
        xor     ax, ax
        call    opl_setchip, ax
        or      ax, -1
    @@exit:
        ret
endp


; сброс модуля OPL (хардварный и софтверный сразу)
proc opl_reset
        cmp     [chipType], TYPE_OPL2
        je      @@rst0
        mov     al, 1
        call    opl_setchip, ax
        call    hard_reset
    @@rst0:
        xor     ax, ax
        call    opl_setchip, ax
        call    hard_reset
        ; сбрасываем данные
        push    ds
        pop     es
        xor     ax, ax
        lea     di, [hardVols]
        mov     cx, (length hardVols) * (size hardVols)
        rep     stosb
        lea     di, [keyRegs]
        mov     cx, (length keyRegs) * (size keyRegs)
        rep     stosb

        lea     di, [chanFreq]
        mov     cx, (length chanFreq) * (size chanFreq)
        rep     stosb
        lea     di, [chanNote]
        mov     cx, (length chanNote) * (size chanNote)
        rep     stosb
        lea     di, [chanInst]
        mov     cx, (length chanInst) * (size chanInst)
        rep     stosb
        lea     di, [noteSpec]
        mov     cx, (length noteSpec) * (size noteSpec)
        rep     stosb
        lea     di, [chanOctave]
        mov     cx, (length chanOctave) * (size chanOctave)
        mov     al, 80h
        rep     stosb


        ; задаем дефолтную таблицу преобразования нот в частоту
        mov     al, NOTE_STANDART
        call    opl_SetNoteTable, ax
        ret
endp


; установка активного чипа OPL (0 или 1), для DUAL_OPL или OPL3
proc opl_setchip, @@nChip: word
        mov     ax, [@@nChip]
        mov     dx, 388h
        cmp     al, 1
        ja      @@exit
        jb      @@1
        add     dx, 2
    @@1:
        cli
        mov     [currChip], al
        mov     [adlPort], dx
        sti
    @@exit:
        ret
endp

; возвращает номер активного чипа OPL (если их более 1)
proc opl_getchip
        xor     ax, ax
        mov     al, [currChip]
        ret
endp

; запрещает запись в порты для всех операций ввода/вывода OPL
proc opl_setnowrite, @@flag: byte
        mov     al, [@@flag]
        mov     [noWrite], al
        ret
endp

proc opl_gettype
        xor     ah, ah
        mov     al, [chipType]
        ret
endp


; выбор таблицы перевода нот в частоту
;   newtable:   NOTE_STANDART - стандартная
;               NOTE_SA2      - типа SAT Player
proc opl_setnotetable uses _si _di, @@newtable: byte
        mov     al, [@@newtable]
        lea     si, [std_notetbl]
        cmp     al, NOTE_STANDART
        je      @@copy
        lea     si, [sa2_notetbl]
        cmp     al, NOTE_SA2
        je      @@copy
        cmp     al, NOTE_S3M
        jne     @@exit
    @@copy:
        push    ds
        pop     es
        lea     di, [note_table]
        mov     cx, (length note_table) * (size note_table)
        rep     movsb
    @@exit:
        ret
endp



proc opl_write, @@reg: byte, @@val: byte
        cmp     [noWrite], 0
        jne     @@exit
        xor     bh, bh
        mov     al, [@@reg]
        mov     ah, [@@val]
        mov     bl, al                  ; bx - reg
        cmp     al, 40h
        jb      @@write
        cmp     al, 55h
        ja      @@isA0
        ; 0x40..0x55
        sub     bx, 40h
        mov     [hardVols+bx], ah       ; hardVols[reg-0x40] = val;
        ; корректируем значение громкости
        mov     si, offset op_table     ; si - &op_table[chan]
        xor     bx, bx
    @@loop_vol:
        mov     dl, [si]
        mov     bl, dl                  ; bx = op_table[i]
        add     dl, 43h                 ; dl = 0x43+op_table[i]
        cmp     al, dl
        jne     @@is_modulate
        ; dl - reg
    @@vol_calck:
        mov     al, 63
        mov     ch, ah
        and     ah, al                  ; ah = val & 63
        and     ch, 0C0h                ; ch = val & 0cC0
        sub     al, ah                  ; c = 63 - (val & 63);
        mov     dh, [curVolume]
        mul     dh
        mov     cl, 63
        div     cl                      ; v = (c * curVolume) / 63;
        sub     cl, al
        or      cl, ch                  ; val = (63 - v) | (val & 0xC0);
        mov     al, dl
        mov     ah, cl
        jmp     @@write
    @@is_modulate:
        sub     dl, 3
        cmp     al, dl
        jne     @@vol_next_chan
        mov     dh, [hardVols+22+bx]
        and     dh, 1
        jnz     @@vol_calck
    @@vol_next_chan:
        inc     si
        cmp     si, offset op_table+9
        jb      @@loop_vol
        jmp     @@write

    @@isA0:
        cmp     al, 0A0h
        jb      @@write
        cmp     al, 0A8h
        ja      @@isB0
        ; 0xA0..0xA8
        sub     bx, 0A0h
        shl     bx, 1
        mov     [byte chanFreq+bx], ah  ; chanFreq[reg - 0xa0] = val
        ;jmp     @@write

    @@write:
        call    writeFM
    @@exit:
        ret

    @@isB0:
        cmp     al, 0B0h
        jb      @@write
        cmp     al, 0B8h
        ja      @@isC0
        ; 0xB0..0xB8
        sub     bx, 0B0h
        mov     cl, [keyRegs+bx]
        or      cl, cl
        jnz     @@1
        test    ah, 20h
        jz      @@1
        ; if (!keyRegs[reg - 0xb0][0] && (val & 32))
        mov     [keyRegs+18+bx], 1      ; keyRegs[reg - 0xb0][0] = val & 32;
    @@1:
        mov     cl, ah
        and     cl, 20h
        mov     [keyRegs+bx], cl        ; keyRegs[chan] = val & 32
        shl     bx, 1
        mov     cl, ah
        and     cl, 3
        mov     [byte chanFreq+1+bx], cl; chanFreq[chan] = chanFreq[chan] | ((val & 0x03) << 8)
        cmp     [beQuiet], 0
        je      @@write
        and     ah, not 20h             ; val &= ~32;
        jmp     @@write
    @@isC0:
        cmp     al, 0C0h
        jb      @@write
        cmp     al, 0C8h
        ja      @@write
        ; 0xC0..0xC8
        sub     bx, 0C0h
        mov     bl, [op_table+bx]
        mov     [hardVols+22+bx], ah    ; hardVols[op_table[reg-0xc0]][1] = val;
        jmp     @@write
endp




;+===========================================================================+
;|###########################################################################|
;|################################# VOLUMES #################################|
;|###########################################################################|
;+===========================================================================+

; sets the OPL2 quiet, while still writing to the registers
proc opl_setquiet, @@quiet: byte
        mov     ah, [@@quiet]
        mov     [beQuiet], ah
        or      ah, ah
        mov     al, [oldVolume]
        jz      @@setvol
        mov     al, [curVolume]
        mov     [oldVolume], al
        xor     ax, ax                  ; гасим звук
    @@setvol:
        call    opl_SetVolume, ax
        ret
endp


proc opl_setvolume, @@newvolume: word
        mov     ax, [@@newvolume]
        and     al, 3Fh
        mov     [curVolume], al
        ; корректируем громкость в регистрах
        mov     si, offset op_table     ; si - &op_table[chan]
    @@loop:
        ; carrier
        mov     bl, [si]
        mov     dl, bl
        xor     bh, bh                  ; bx = op_table[chan]
        add     dl, 43h                 ; dl = 0x43+op_table[i]
        mov     cl, [hardVols+bx+3]
        mov     ch, cl
        mov     al, 3Fh
        and     cl, al                  ; cl = hardVols[op_table[i]+3][0] & 63
        and     ch, 0C0h                ; ch = hardVols[op_table[i]+3][0] & 0xC0
        sub     al, cl                  ; carrier = 63 - (hardVols[op_table[i]+3][0] & 63);
        mov     dh, [curVolume]
        mul     dh
        mov     cl, 63
        div     cl                      ; vol = (carrier * curVolume) / 63
        sub     cl, al                  ; k = 63 - vol;
        mov     al, dl
        or      cl, ch                  ; cl = k | (hardVols[op_table[i]+3][0] & 0xC0)
        mov     ah, cl
        call    writeFM
        ; modulate
        mov     dl, bl
        add     dl, 40h                 ; dl = 0x40+op_table[i]
        mov     al, [hardVols+bx+22]
        and     al, 1
        jz      @@next
        mov     cl, [hardVols+bx]
        mov     ch, cl
        mov     al, 3Fh
        and     cl, al                  ; cl = hardVols[op_table[i]][0] & 63
        and     ch, 0C0h                ; ch = hardVols[op_table[i]][0] & 0xC0
        sub     al, cl                  ; modulate = 63 - (hardVols[op_table[i]][0] & 63);
        mov     dh, [curVolume]
        mul     dh
        mov     cl, 63
        div     cl                      ; vol = (modulate * curVolume) / 63
        sub     cl, al                  ; k = 63 - vol;
        mov     al, dl
        or      cl, ch                  ; cl = k | (hardVols[op_table[i]][0] & 0xC0)
        mov     ah, cl
        call    writeFM
    @@next:
        inc     si
        cmp     si, offset op_table+9
        jb      @@loop
        ret
endp



proc opl_getvolume
        xor     ah, ah
        mov     al, [curVolume]
        ret
endp


;+===========================================================================+
;|###########################################################################|
;|################################ EQUALIZER ################################|
;|###########################################################################|
;+===========================================================================+

proc opl_getchanvol, @@chan: byte
        xor     bh, bh
        mov     bl, [@@chan]
        mov     bl, [op_table+bx]       ; bx = op_table[chan]
        mov     al, [hardVols+bx]       ; al = hardVols[op_table[chan]][0] & 0x3F
        add     bx, 3
        and     ax, 3Fh
        mov     cl, [hardVols+bx]       ; cl = hardVols[op_table[chan]+3][0] & 0x3F
        and     cx, 3Fh                 ; cl = carrier volume
        add     ax, cx
        ret
endp


proc opl_getkeyon, @@chan: byte
        xor     bh, bh
        mov     bl, [@@chan]
        xor     ah, ah
        mov     al, [keyRegs+18+bx]
        mov     [keyRegs+18+bx], ah     ; keyregs[1][chan] = 0
        ret
endp


proc opl_setnotespc uses _di, @@chan: byte, @@notoct: byte
        mov     bl, [@@notoct]
        xor     bh, bh
        cmp     bl, 9*8                 ; 9 нот по 8 октав каждая
        jb      @@1
        sub     bx, 9*8
    @@1:
        mov     al, [noteType]
        cmp     al, 1
        ja      @@done
        jz      @@dots
        ; помечаем noteSpectrum
        mov     di, offset noteSpec
        add     di, bx                  ; di = &noteSpec[notoct]
        mov     al, 9*8                 ; высота окна * высоту символа
        mov     [di], al
        ret
    @@dots:
        ; и noteDots
        mov     di, offset noteDots
        add     di, bx
        mov     al, [@@chan]
        mov     cl, 9*8                 ; одна строка = 9 нот по 8 октав каждая
        mul     cl
        add     di, ax                  ; di = &noteSpec[chan][notoct]
        cmp     [byte di], 64
        jae     @@done
        mov     al, 78
        mov     [di], al
    @@done:
        ret
endp


end

