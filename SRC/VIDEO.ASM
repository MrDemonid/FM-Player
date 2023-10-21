        ideal
        p8086
        model   large, c
        smart           ; генерация оптимального кода
        locals  @@      ; pазpешаем локальные идентефикатоpы
        nojumps         ; запpещаем TASM'у подставлять NOP'ы

        %noincl



include "bios.inc"


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
  else
        mov     &par
  endif
endm


_si     equ <si>
_di     equ <di>


public vid_IsEGA
public vid_SetVideoMode
public vid_RestoreMode

public vid_DosStr
public vid_UpChr

public vid_ScrWidth
public vid_ScrHeight
public vid_GetAddress

public vid_SetFont

public vid_PutSprite
public vid_FillRect
public vid_PutText
public vid_GetText
;public vid_PutString
public vid_XorColor

public vid_ClrInfo
public vid_PutInfo
public vid_PutTime


dataseg
  szDir         db "     <dir>", 0
udataseg
  OldMode       db ?

codeseg


;╔═══════════════════════════════════════════════════════════════════════════╗
;║▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ УСТАНОВКА ВИДЕОРЕЖИМА ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║
;╚═══════════════════════════════════════════════════════════════════════════╝

proc vid_IsEGA
        mov     ah, 12h
        mov     bl, 10h
        int     10h
        sub     bl, 10h      ; if (bl == 0) - адаптер ниже EGA
        mov     al, bl
        ret
endp


proc vid_SetVideoMode uses _si _di
        xor     ax, ax
        mov     es, ax
        ; сохраняем старый видеорежим
        mov     ah, 0Fh
        int     10h
        cmp     al, 2           ; ч/б, 80x25
        je      @@chk_43lines
        cmp     al, 3
        jne     @@store_mode
    @@chk_43lines:
        cmp     [byte es:484h], 24
        je      @@store_mode
        or      al, 80h         ; устанавливаем признак режима 43/50 строк
    @@store_mode:
        mov     [OldMode], al
        ; устанавливаем видеорежим 80x43/50
        mov     ax, 3
        int     10h
        call    vid_Set50Row
        ; выключаем бит мерцания атрибутов символов
        call    vid_SetIntensityMode
        ; скрываем курсор
        mov     ah, 1
        mov     cx, 2000h       ; 0600h
        int     10h
        ; устанавливаем шиpину символов pавной 8
    if (@Cpu and 4) ne 0
        call    vid_CharWidth, 8
    else
        mov     ax, 8
        call    vid_CharWidth, ax
    endif
        ret
endp


proc vid_DosStr uses ds _si _di, @@str: ptr byte
        LDS_    dx, [@@str]
        mov     ah, 9
        int     21h
        ret
endp

;
; переводит символ в верхний регистр
;
proc vid_UpChr, @@symb: byte
        mov     al, [@@symb]
        cmp     al, 'a'
        jb      @@done
        cmp     al, 'z'
        ja      @@done
        sub     al, 'a'-'A'
    @@done:
        ret
endp


;
; устанавливает режим 16 цветов фона
;
proc vid_SetIntensityMode
        cli
        ; сбрасываем регистр 3C0h, чтобы первая запись
        ; пришлась на индексный регистр
        mov     dx, 3DAh  ; инициализация регистра 3C0h
        in      al, dx
        ; выбираем индексный регистр бита мерцания
        mov     dx, 3C0h
        mov     al, 10h   ; регистр управления режимом
        out     dx, al    ; адрес
        xor     al, al    ; по дефолту здесь 0x08, меняем на 0x00
        out     dx, al
        mov     dx, 3DAh  ; инициализация регистра 3C0h
        in      al, dx
        ; разрешаем доступ к регистрам палитры
        mov     dx, 3C0h
        mov     al, 20h
        out     dx, al
        sti
        ret
endp



; Восстанавливает предыдущий видеорежим
proc vid_RestoreMode uses _si _di
    if (@Cpu and 4) ne 0
        call    vid_CharWidth, 9
    else
        mov     ax, 9
        call    vid_CharWidth, cx
    endif
        xor     ax, ax
        mov     al, [OldMode]
        and     al, 7Fh
        int     10h
        mov     al, [OldMode]
        test    al, 80h
        jz      @@done
        call    vid_Set50Row            ; выполняем переход в режим 43/50 линий
        mov     ah, 1                   ; задаем курсор
        mov     cx, 0600h
        int     10h
    @@done:
        ret
endp


;================================= вспомогательные ===========================

; Переход в режим 50/43 строк
proc vid_Set50Row near
        ; устанавливаем активную страницу 0
        push    bp
        xor     al, al
        mov     ah, 5
        int     10h
        pop     bp
        ; выключаем бит меpцания
        mov     ax, 1003h
        xor     bx, bx
        int     10h
        ; и устанавливаем фонт 8x8
        mov     ax, 1112h
        xor     bl, bl
        int     10h
        ; задаем альтернативный обработчик print-screen
        mov     ax, 1200h
        mov     bl, 20h
        int     10h
        ; настраиваем прорисовку курсора
        mov     ax, 1A00h
        int     10h
        cmp     al, 1Ah
        je      @@exit                  ; <- это VGA, уже все настроено
        xor     ax, ax
        mov     es, ax
        or      [byte es:487h], 1       ; disable cursor size emulation
    @@exit:
        ret
endp


; Установка ширины сканлинии в 640 или 720 пикселей (только для VGA!)
; Вызывается непосредственно после установки видеорежима!
; на входе:
;   chrWidth    - шиpина символов:
;                   8: ширина сканлинии 640 пикселей
;                   9: ширина сканлинии 720 пикселей
proc vid_CharWidth near @@chrWidth: byte
        or      bx, -1
        mov     ax, 101Ah
        int     10h
        sub     bx, -1
        jz      @@exit          ; <- на EGA нет смысла
        mov     al, [@@chrWidth]
        mov     bh, 9
        cmp     al, bh          ; if (chrWidth > 9)
        ja      @@exit          ;     return
        cmp     al, 8           ; if (chrWidth < 8)
        jb      @@exit          ;     return
        xor     cx, cx
        sub     bh, al
        jnz     @@set
        mov     cx, 0804h
    @@set:
        cli
        mov     ax, 100h        ; AH = значение регистра перезапуска:
        mov     dx, 3C4h        ; бит 1 = 0{; бит 0 = 1 }
        out     dx, ax          ; синхронный перезапуск блока синхрониз.
        inc     al              ; AL = номер регистра количества
        out     dx, al          ;      пикселов в строке символа
        inc     dx
        in      al, dx          ; AL = значение регистра
        dec     dx
        shr     al, 1           ;-Убpать младший бит
        shr     bh, 1           ;-Загнать младший бит в CARRY
        rcl     al, 1           ;-CARRY в младший бит
        xchg    ah, al          ; AH = новое значение регистра
        mov     al, 1           ;-Запись нового значения в pегистp
        out     dx, ax          ; pежима синхpонизации
        mov     dl, 0CCh        ; Чтение pегистpа упpавления: поpт 3CCh
        in      al, dx          ; Сбpос и пеpеустановка битов, задающих
        and     al, not 0Ch     ; частоту синхpонизации
        or      al, cl
        mov     dl, 0C2h        ; Запись в pегистp упpавления: поpт 3C2h
        out     dx, al
        mov     dl, 0DAh        ; Получить доступ к контpоллеpу атpибутов
        in      al, dx          ; посpедством чтения из поpта 3DAh
        mov     dl, 0C0h
        mov     al, 13h         ; Получить доступ к pегистpу гоpизонтального
        out     dx, al          ; сдвига: поpт 3C0h, pегистp 13h
        mov     al, ch          ; Установить величину сдвига
        out     dx, al
        mov     dl, 0DAh        ; Получить доступ к контpоллеpу атpибутов
        in      al, dx
        mov     dl, 0C0h        ; Разpешить генеpацию изобpажения
        mov     al, 20h
        out     dx, al
        mov     dl, 0C4h        ; Пеpезапуск блока синхpонизации
        mov     ax, 300h
        out     dx, ax
        sti
    @@exit:
        ret
endp



;╔═══════════════════════════════════════════════════════════════════════════╗
;║▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ПРИМИТИВЫ ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║
;╚═══════════════════════════════════════════════════════════════════════════╝

; Вычисляет адрес видеопамяти в указанной позиции на активной видеостранице
; на входе:
;       dl      - x
;       dh      - y
; на выходе:
;       es:[di] - адрес видеопамяти
;       bx      - ширина экрана в символах
proc vid_GetAddr uses dx
        assume  es: BIOS
        xor     ax, ax
        mov     es, ax          ; es - сегмент данных BIOS
        mov     bx, [VID_columns]; ширина экрана в символах
        mov     al, dh          ; ax - y
        xor     dh, dh
        mov     di, dx          ; di - x
        mul     bx              ; ax = width*y
        add     di, ax
        mov     dx, [CRT_base]
        shl     di, 1           ; учитываем байты атрибутов
        mov     ax, 0B000h      ; предполагаем что в наличии монохромный адаптер
        add     di, [VID_pageoffs] ; добавляем смещение активной видеостраницы
        cmp     dx, 3D4h
        jne     @@done
        add     ax, 800h
    @@done:
        mov     es, ax          ; es - сегмент видеопамяти
        ret
        assume  es: nothing
endp

; Возвращает ширину экрана
; на выходе:
;   AX          - ширина в символах
proc vid_ScrWidth
        assume  es: BIOS
        xor     ax, ax
        mov     es, ax
        mov     ax, [VID_columns]
        ret
        assume  es: nothing
endp


; Возвращает высоту экрана
; на выходе:
;   AX          - высота
proc vid_ScrHeight
        assume  es: BIOS
        xor     ax, ax
        mov     es, ax
        mov     al, [VID_rows]  ; al - номер последней линии экрана
        inc     ax
        ret
        assume  es: nothing
endp


;proc vid_SetCursor uses bp, @@top: byte, @@bottom: byte
;        mov     ah, 1
;        mov     cl, [@@bottom]  ; номер нижней линии курсора
;        mov     ch, [@@top]     ; номер верхней линии курсора
;        int     10h
;        ret
;endp


proc vid_GetAddress uses si di, @x: word, @y: word
        mov     dl, [byte @x]
        mov     dh, [byte @y]
        call    vid_GetAddr
        mov     dx, es
        mov     ax, di
        ret
endp


;╔═══════════════════════════════════════════════════════════════════════════╗
;║▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ФОНТ ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║
;╚═══════════════════════════════════════════════════════════════════════════╝

; Открывает доступ к плоскости 2 знакогенератора (фонт)
proc vid_CharGenOpen
        ; разрешаем запись в слой 2 (знакогенератор) и запрещаем запись в слои 0 и 1 (коды символов и их атрибуты)
        mov     dx, 3C4h
        mov     ax, 402h
        out     dx, ax          ; set map mask 4 : only plane 2
        ; выбираем режим видеопамяти
        mov     ax, 704h
        out     dx, ax          ; set memory model (text, >64k,seg.)
        ; выбираем схему чтения в регистре графического контроллера
        mov     dx, 3CEh
        mov     ax, 204h
        out     dx, ax          ; read map select: plane 2
        ; настраиваем режим записи и считывания графического контроллера
        mov     ax, 5
        out     dx, ax          ; write mode 0
        ; подправляем регистр смешанных данных
        mov     ax, 406h
        out     dx, ax          ; memory mapping for CPU : a000h, 64k.
        ret
endp




; Закрывает доступ к плоскости 2 знакогенератора
proc vid_CharGenClose
        xor     ax, ax
        mov     es, ax
        ; ввосстанавливаем режим видеопамяти
        mov     dx,3C4h
        mov     ax,302h
        out     dx,ax
        ; разрешаем запись в слои 0 и 1, и запрещаем в слой 2
        mov     ax,304h
        out     dx,ax
        ; восстанавливаем схему чтения в регистре графического контроллера
        mov     dx,3CEh
        mov     ax,4
        out     dx,ax
        ; восстанавливаем режим записи и считывания графического контроллера
        mov     ax,1005h
        out     dx,ax
        ; восстанавливаем сегмент окна доступа к плоскостям
        mov     ax, 0A06h
        cmp     [word es:463h], 3B4h
        jz      @@1
        mov     ah, 0Eh
    @@1:
        out     dx, ax
        ret
endp



; Установка пользовательского фонта
; на входе:
;   lpFont      - указатель на фонт
proc vid_SetFont uses ds _si _di, @@pFont: ptr byte, @@chrHeight: byte, @@chrFirst: byte, @@chrNum: word
        cld
        cli
        call    vid_CharGenOpen
        mov     ax, 0A000h
        mov     es, ax                  ; es - сегмент плоскости знакогенератора
        xor     ah, ah
        mov     al, [@@chrFirst]
        mov     bx, 32
        mul     bx
        mov     di, ax
        xor     dh, dh
        mov     dl, [@@chrHeight]       ; dx - char height
        sub     bx, dx                  ; bx - 32 - char height
        LDS_    si, [@@pFont]
        mov     ax, [@@chrNum]
        or      ax, ax
        jz      @@exit
    @@charloop:
        mov     cx, dx
        shr     cx, 1
        rep     movsw
        adc     cx, 0
        rep     movsb
        add     di, bx                  ; es:di - next char
        dec     ax
        jnz     @@charloop
    @@exit:
        call    vid_CharGenClose
        sti
        ret
endp

;proc vid_GetFont uses ds _si _di, @@pFont: ptr byte, @@chrHeight: byte, @@chrFirst: byte, @@chrNum: word
;        cld
;        cli
;        call    vid_CharGenOpen
;        mov     ax, 0A000h
;        mov     ds, ax                  ; ds - сегмент плоскости знакогенератора
;        xor     ah, ah
;        mov     al, [@@chrFirst]
;        mov     bx, 32
;        mul     bx
;        mov     si, ax                  ; ds:si - first char
;        xor     dh, dh
;        mov     dl, [@@chrHeight]       ; dx - char height
;        sub     bx, dx                  ; bx - 32 - char height
;        LES_    di, [@@pFont]           ; es:di - dest buffer
;        mov     ax, [@@chrNum]
;        or      ax, ax
;        jz      @@exit
;    @@charloop:
;        mov     cx, dx
;        shr     cx, 1
;        rep     movsw
;        adc     cx, 0
;        rep     movsb
;        add     si, bx                  ; es:di - next char
;        dec     ax
;        jnz     @@charloop
;    @@exit:
;        call    vid_CharGenClose
;        sti
;        ret
;endp



;╔═══════════════════════════════════════════════════════════════════════════╗
;║▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ТЕКСТ ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║
;╚═══════════════════════════════════════════════════════════════════════════╝

; вывод строки в поле длиной CX, с заполнением остатка поля символами AH
; на входе:
;   es:di       - video addr
;   ds:si       - строка
;   cx          - длина поля вывода
;   ah          - символ-заполнитель пустого места
; на выходе:
;   es:di       - след. вид. адрес
proc vid_TextLeftJust near
        lodsb
        dec     cx
        js      @@exit
        or      al, al
        jz      @@fill
        mov     [es:di], al
        add     di, 2
        jmp     vid_TextLeftJust
    @@fill:
        mov     [es:di], ah
        add     di, 2
        dec     cx
        jns     @@fill
    @@exit:
        ret
endp

; вывод числа в поле длиной CX, с выравниванием по правому краю и заполнением
; остатка поля пробелами
; на входе:
;   es:di       - адрес видеопамяти
;   si:bx       - 32-х битное число
;   cx          - длина поля вывода
proc vid_Long near uses bp
        add     di, cx
        add     di, cx
        mov     bp, 10          ; bp - divisor
        push    di
        sub     di, 2
    @@conv:
        dec     cx
        js      @@done
        xor     dx, dx
        mov     ax, si
        div     bp
        mov     si, ax
        mov     ax, bx
        div     bp
        mov     bx, ax
        add     dl, '0'
        mov     [es:di], dl
        sub     di, 2
        or      ax, si
        jnz     @@conv
    @@fill:
        dec     cx
        js      @@done
        mov     [byte es:di], ' '
        sub     di, 2
        jmp     @@fill
    @@done:
        pop     di
        ret
endp


; на входе:
;   es:di       - адрес видеопамяти
;   al          - число
proc vid_Byte near
        mov     cl, 10
        xor     ah, ah
        div     cl
        add     ax, '00'
        mov     [es:di], al
        add     di, 2
        mov     [es:di], ah
        add     di, 2
        ret
endp



proc vid_PutSprite uses ds _si _di, @@pImg: ptr byte, @@x: byte, @@y: byte
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr             ; bx - ширина экрана в символах
        LDS_    si, [@@pImg]
        lodsw
        sub     bx, ax
        mov     dx, ax                  ; dx - ширина битмапа
        shl     bx, 1                   ; bx - screen increment
        lodsw
    @@loop:
        mov     cx, dx
        rep     movsw
        add     di, bx
        dec     ax
        jnz     @@loop
        ret
endp



proc vid_FillRect uses ds _si _di, @@x: byte, @@y: byte, @@width, @@height, @@attr
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr             ; bx - ширина экрана в символах
        mov     dx, [@@width]
        sub     bx, dx
        shl     bx, 1
        mov     ax, [@@attr]
    @@loop:
        mov     cx, dx
        rep     stosw
        add     di, bx
        dec     [@@height]
        jnz     @@loop
        ret
endp




; вывод текста в поле заданной длины:
; на входе:
;   x,y   - координаты поля вывода
;   len   - длина поля (в символах)
;   pText - строка
proc vid_PutText uses ds _si _di, @@x: byte, @@y: byte, @@len: word, @@pText: ptr byte
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr
        mov     cx, [@@len]
        LDS_    si, [@@pText]
        mov     ah, ' '
        call    vid_TextLeftJust
        ret
endp


proc vid_GetText uses ds _si _di, @@x: byte, @@y: byte, @@max: word, @@pText: ptr byte
        dec     [@@max]
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr             ; es:[di] - video address
        LDS_    si, [@@pText]           ; ds:[si] - buffer
        xor     bx, bx
        xor     cx, cx                  ; cx - num input chars
        mov     [si], bl                ; *pText[0] = '\0'
        call    @@showstr
        call    @@showcursor

    @@loop:
        xor     ax, ax
        int     16h
        or      al, al
        jz      @@extkey
        cmp     al, 27                  ; ESC ?
        je      @@esc
        jb      @@isenter
        ; простой символ
        cmp     cx, [@@max]
        jae     @@loop

        push    bx
        ; вносим символ в буфер
    @@shift:
        or      al, al
        jz      @@redraw
        mov     ah, [si+bx]
        mov     [si+bx], al
        mov     al, ah
        inc     bx
        jmp     @@shift
    @@redraw:
        mov     [si+bx], al

        pop     bx
        call    @@showcursor
        inc     bx
        inc     cx
        call    @@showstr
        call    @@showcursor
        jmp     @@loop


    @@esc:      ; возвращаем пустую строку
        call    @@showcursor
        xor     al, al
        mov     [si], al
        ret

    @@isenter:
        cmp     al, 13
        je      @@enter
        cmp     al, 8
        jne     @@loop
    @@backspc:  ; забой
        or      bx, bx
        jz      @@loop
        push    bx
      @@bkloop:
        mov     al, [si+bx]
        mov     [si+bx-1], al
        inc     bx
        or      al, al
        jnz     @@bkloop
        pop     bx
        call    @@showcursor
        call    @@showstr
        dec     bx
        call    @@showcursor
        dec     cx
        jmp     @@loop


    @@extkey:
        cmp     ah, 4bh
        je      @@curleft
        cmp     ah, 4dh
        jne     @@loop

    @@curright: ; перемещение курсора вправо
        mov     al, [si+bx]
        or      al, al
        jz      @@loop
        call    @@showcursor
        inc     bx
        call    @@showcursor
        jmp     @@loop

    @@curleft:  ; перемещение курсора влево
        or      bx, bx
        jz      @@loop
        call    @@showcursor
        dec     bx
        call    @@showcursor
        jmp     @@loop


    @@enter:    ; выходим
        call    @@showcursor            ; скрываем курсор
        ret


    @@showstr:
        push    di si cx
        mov     ah, ' '
        mov     cx, [@@max]
        call    vid_TextLeftJust
        pop     cx si di
        retn
    @@showcursor:
        shl     bx, 1
        mov     al, 0EFh
        xor     [es:di+bx+1], al
        shr     bx, 1
        retn
endp

; vid_TextLeftJust
; вывод строки в поле длиной CX, с заполнением остатка поля символами AH
; на входе:
;   es:di       - video addr
;   ds:si       - строка
;   cx          - длина поля вывода
;   ah          - символ-заполнитель пустого места
; на выходе:
;   es:di       - след. вид. адрес




;proc vid_PutString uses ds _si _di, @@x: byte, @@y: byte, @@pStr: ptr byte
;        mov     dh, [@@y]
;        mov     dl, [@@x]
;        call    vid_GetAddr
;        LDS_    si, [@@pStr]
;    @@putchar:
;        lodsb
;        or      al, al
;        jz      @@exit
;        mov     [es:di], al
;        add     di, 2
;        jmp     @@putchar
;    @@exit:
;        ret
;endp


proc vid_XorColor uses _di, @@x: byte, @@y: byte, @@len: word, @@mask: byte
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr
        mov     al, [@@mask]
        inc     di
        mov     cx, [@@len]
    @@xorbyte:
        xor     [es:di], al
        add     di, 2
        loop    @@xorbyte
        ret
endp




proc vid_ClrInfo uses ds _si _di, @@x: byte, @@y: byte
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr             ; es:di - screen addr
        ; очищаем строку
        mov     al, ' '
        mov     cx, 74;12+12+2+2+1+2+1+4+2+2+1+2+1+2
    @@fill:
        stosb
        inc     di
        loop    @@fill
        ret
endp


proc vid_PutInfo uses ds _si _di, @@x: byte, @@y: byte, @@pName: ptr byte, @@attr: byte, @@fsize: dword, @@date, @@time
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr             ; es:di - screen addr
        ; выводим имя файла
        mov     cx, 12
        LDS_    si, [@@pName]
        mov     ah, ' '
        call    vid_TextLeftJust
        ; его размер
        mov     al, ' '                 ; add     di, 2*2
        stosb
        inc     di
        stosb
        inc     di
        test    [@@attr], 10h           ; directory?
        jz      @@putsize
        mov     ax, @data
        mov     ds, ax
        lea     si, [szDir]
        mov     cx, 10
        call    vid_TextLeftJust
        jmp     @@putdate
    @@putsize:
        mov     si, [word @@fsize+2]
        mov     bx, [word @@fsize]
        mov     cx, 10
        call    vid_Long
    @@putdate:
        ; дату создания
        mov     al, ' '                 ; add     di, 2*2
        stosb
        inc     di
        stosb
        inc     di
        mov     bx, [@@date]
        mov     al, bl
        and     al, 01Fh
        call    vid_Byte
        mov     al, '/'
        stosb
        inc     di
        mov     ax, bx
    if (@Cpu and 4) ne 0
        shr     ax, 5
    else
        mov     cl, 5
        shr     ax, cl
    endif
        and     al, 0Fh
        call    vid_Byte
        mov     al, '/'
        stosb
        inc     di
        xor     si, si
    if (@Cpu and 4) ne 0
        shr     bx, 9
    else
        mov     cl, 9
        shr     bx, cl
    endif
        add     bx, 1980
        mov     cx, 4
        call    vid_Long
        ; время создания
        mov     al, ' '                 ; add     di, 2*2
        stosb
        inc     di
        stosb
        inc     di
        mov     bx, [@@time]
        mov     ax, bx
    if (@Cpu and 4) ne 0
        shr     ax, 11
    else
        mov     cl, 11
        shr     ax, cl
    endif
        call    vid_Byte
        mov     al, ':'
        stosb
        inc     di
        mov     ax, bx
    if (@Cpu and 4) ne 0
        shr     ax, 5
    else
        mov     cl, 5
        shr     ax, cl
    endif
        and     al, 3Fh
        call    vid_Byte
        mov     al, ':'
        stosb
        inc     di
        mov     al, bl
        and     al, 1Fh
        shl     al, 1
        call    vid_Byte

        ret
endp


; in: dx:ax - делимое
;        cx - делитель
; out:dx:ax - частное
;        si - остаток
proc uldiv
        xor     bx, bx
        cmp     dx, cx
        jb      @@one_div
        ; делим старшую часть
        mov     bx, ax
        mov     ax, dx
        xor     dx, dx
        div     cx
        xchg    ax, bx
    @@one_div:
        div     cx
        mov     si, dx
        mov     dx, bx
        ret
endp


proc vid_PutTime uses ds bx cx _si _di, @@x: byte, @@y: byte, @@time: dword
  local @@hour:byte, @@min:byte, @@sec:byte
        mov     dh, [@@y]
        mov     dl, [@@x]
        call    vid_GetAddr             ; es:di - screen addr
        ; dx:bx = time * 1365
        mov     ax, [word @@time]
        mov     cx, 1365
        mul     cx
        mov     bx, ax
        mov     si, dx
        mov     ax, [word @@time+2]
        mul     cx
        add     ax, si                  ; ax:bx = time * 1365
        ; dx:bx / 24852
        mov     dx, ax
        mov     ax, bx
        mov     cx, 24852
        call    uldiv
        ; переводим в часы
        mov     cx, 3600
        call    uldiv                   ; si = ticks % 3600
        mov     [@@hour], al
        mov     ax, si
        xor     dx, dx
        mov     cx, 60
        div     cx                      ; ax - min, dx - sec
        mov     [@@min], al
        mov     [@@sec], dl
        ; вывод на экран
        mov     al, [@@hour]
        call    vid_Byte
        mov     al, ':'
        stosb
        inc     di
        mov     al, [@@min]
        call    vid_Byte
        mov     al, ':'
        stosb
        inc     di
        mov     al, [@@sec]
        call    vid_Byte

        ret
endp


end

