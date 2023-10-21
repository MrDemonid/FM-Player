        ideal
        p8086
        model   large, c
        smart           ; генерация оптимального кода
        locals  @@      ; pазpешаем локальные идентефикатоpы
        nojumps         ; запpещаем TASM'у подставлять NOP'ы

        %noincl


public key_Get


public key_InstallHandler
public key_RestoreHandler
public key_GetState
public key_GetCurrentState
public key_Flash


udataseg
  keyState      db 128 dup (?)          ; текущее состояние клавиш
  wasPressed    db 128 dup (?)          ; флаги нажатия клавиш

  oldHandler    dw 2 dup (?)            ; старый обработчик клавиатуры


codeseg


proc key_Get
        xor     ax, ax
        int     16h
        ret
endp



;+===========================================================================+
;|###########################################################################|
;|############################ KEYBOARD HANDLER #############################|
;|###########################################################################|
;+===========================================================================+

proc key_Handler far
        push    ds ax dx bx
        mov     ax, @data
        mov     ds, ax
        mov     dx, 60h
        in      al, dx                  ; al - scan code
        xor     bh, bh
        mov     bl, al
        ; очищаем контроллер клавиатуры для XT
        inc     dx
        in      al, dx
        or      al, 80h
        out     dx, al
        and     al, 7Fh
        out     dx, al
        ; обрабатываем скэн-код
        test    bl, 80h
        jz      @@pressed
        ; клавиша отпущена
        and     bx, 7Fh
        mov     [keyState+bx], 0
        jmp     @@exit
    @@pressed:
        ; клавиша нажата
        mov     [keyState+bx], 1
        mov     [wasPressed+bx], 1
    @@exit:
        mov     ax, 20h
        mov     dx, ax
        out     dx, al
        pop     bx dx ax ds
        iret
endp


proc key_InstallHandler uses ds
        ; сохраняем предыдущий вектор
        mov     ax, 3509h
        int     21h
        mov     [oldHandler], bx
        mov     [oldHandler+2], es
        ; устанавливаем свой обработчик
        push    ds
        mov     ax, 2509h
        lea     dx, [key_Handler]
        push    cs
        pop     ds
        int     21h
        pop     es
  key_clearbuf:
        ; очищаем массивы скэн-кодов
        xor     ax, ax
        lea     di, [keyState]
        mov     cx, (length keyState) * (size keyState)
        rep     stosb
        lea     di, [wasPressed]
        mov     cx, (length wasPressed) * (size wasPressed)
        rep     stosb
        ret
endp

proc key_Flash uses ds
        push    ds
        pop     es
        jmp     key_clearbuf
endp

proc key_RestoreHandler uses ds
        mov     ax, 2509h
        lds     dx, [dword oldHandler]
        int     21h
        ret
endp


proc key_GetState, @@scancode: byte
        mov     bl, [@@scancode]
        and     bx, 7Fh
        mov     al, [keyState+bx]
        or      al, [wasPressed+bx]
        xor     ah, ah
        mov     [wasPressed+bx], 0
        ret
endp

proc key_GetCurrentState, @@scancode: byte
        mov     bl, [@@scancode]
        and     bx, 7Fh
        mov     al, [keyState+bx]
        xor     ah, ah
        ret
endp


end
