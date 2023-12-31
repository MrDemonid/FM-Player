        ideal
        p8086
        model   large, c
        smart           ; ������� ��⨬��쭮�� ����
        locals  @@      ; p��p�蠥� ������� �����䨪��p�
        nojumps         ; ���p�頥� TASM'� ����⠢���� NOP'�

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


;���������������������������������������������������������������������������ͻ
;��������������������������� ��������� ����������� ���������������������������
;���������������������������������������������������������������������������ͼ

proc vid_IsEGA
        mov     ah, 12h
        mov     bl, 10h
        int     10h
        sub     bl, 10h      ; if (bl == 0) - ������ ���� EGA
        mov     al, bl
        ret
endp


proc vid_SetVideoMode uses _si _di
        xor     ax, ax
        mov     es, ax
        ; ��࠭塞 ���� �����०��
        mov     ah, 0Fh
        int     10h
        cmp     al, 2           ; �/�, 80x25
        je      @@chk_43lines
        cmp     al, 3
        jne     @@store_mode
    @@chk_43lines:
        cmp     [byte es:484h], 24
        je      @@store_mode
        or      al, 80h         ; ��⠭�������� �ਧ��� ०��� 43/50 ��ப
    @@store_mode:
        mov     [OldMode], al
        ; ��⠭�������� �����०�� 80x43/50
        mov     ax, 3
        int     10h
        call    vid_Set50Row
        ; �몫�砥� ��� ���栭�� ��ਡ�⮢ ᨬ�����
        call    vid_SetIntensityMode
        ; ��뢠�� �����
        mov     ah, 1
        mov     cx, 2000h       ; 0600h
        int     10h
        ; ��⠭�������� �p��� ᨬ����� p����� 8
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
; ��ॢ���� ᨬ��� � ���孨� ॣ����
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
; ��⠭�������� ०�� 16 梥⮢ 䮭�
;
proc vid_SetIntensityMode
        cli
        ; ���뢠�� ॣ���� 3C0h, �⮡� ��ࢠ� ������
        ; ��諠�� �� ������� ॣ����
        mov     dx, 3DAh  ; ���樠������ ॣ���� 3C0h
        in      al, dx
        ; �롨ࠥ� ������� ॣ���� ��� ���栭��
        mov     dx, 3C0h
        mov     al, 10h   ; ॣ���� �ࠢ����� ०����
        out     dx, al    ; ����
        xor     al, al    ; �� ��䮫�� ����� 0x08, ���塞 �� 0x00
        out     dx, al
        mov     dx, 3DAh  ; ���樠������ ॣ���� 3C0h
        in      al, dx
        ; ࠧ�蠥� ����� � ॣ���ࠬ �������
        mov     dx, 3C0h
        mov     al, 20h
        out     dx, al
        sti
        ret
endp



; ����⠭�������� �।��騩 �����०��
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
        call    vid_Set50Row            ; �믮��塞 ���室 � ०�� 43/50 �����
        mov     ah, 1                   ; ������ �����
        mov     cx, 0600h
        int     10h
    @@done:
        ret
endp


;================================= �ᯮ����⥫�� ===========================

; ���室 � ०�� 50/43 ��ப
proc vid_Set50Row near
        ; ��⠭�������� ��⨢��� ��࠭��� 0
        push    bp
        xor     al, al
        mov     ah, 5
        int     10h
        pop     bp
        ; �몫�砥� ��� ��p栭��
        mov     ax, 1003h
        xor     bx, bx
        int     10h
        ; � ��⠭�������� 䮭� 8x8
        mov     ax, 1112h
        xor     bl, bl
        int     10h
        ; ������ ����ୠ⨢�� ��ࠡ��稪 print-screen
        mov     ax, 1200h
        mov     bl, 20h
        int     10h
        ; ����ࠨ���� ���ᮢ�� �����
        mov     ax, 1A00h
        int     10h
        cmp     al, 1Ah
        je      @@exit                  ; <- �� VGA, 㦥 �� ����஥��
        xor     ax, ax
        mov     es, ax
        or      [byte es:487h], 1       ; disable cursor size emulation
    @@exit:
        ret
endp


; ��⠭���� �ਭ� ᪠������ � 640 ��� 720 ���ᥫ�� (⮫쪮 ��� VGA!)
; ��뢠���� �����।�⢥��� ��᫥ ��⠭���� �����०���!
; �� �室�:
;   chrWidth    - �p��� ᨬ�����:
;                   8: �ਭ� ᪠������ 640 ���ᥫ��
;                   9: �ਭ� ᪠������ 720 ���ᥫ��
proc vid_CharWidth near @@chrWidth: byte
        or      bx, -1
        mov     ax, 101Ah
        int     10h
        sub     bx, -1
        jz      @@exit          ; <- �� EGA ��� ��᫠
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
        mov     ax, 100h        ; AH = ���祭�� ॣ���� ��१���᪠:
        mov     dx, 3C4h        ; ��� 1 = 0{; ��� 0 = 1 }
        out     dx, ax          ; ᨭ�஭�� ��१���� ����� ᨭ�஭��.
        inc     al              ; AL = ����� ॣ���� ������⢠
        out     dx, al          ;      ���ᥫ�� � ��ப� ᨬ����
        inc     dx
        in      al, dx          ; AL = ���祭�� ॣ����
        dec     dx
        shr     al, 1           ;-��p��� ����訩 ���
        shr     bh, 1           ;-������� ����訩 ��� � CARRY
        rcl     al, 1           ;-CARRY � ����訩 ���
        xchg    ah, al          ; AH = ����� ���祭�� ॣ����
        mov     al, 1           ;-������ ������ ���祭�� � p�����p
        out     dx, ax          ; p����� ᨭ�p�����樨
        mov     dl, 0CCh        ; �⥭�� p�����p� �p�������: ��p� 3CCh
        in      al, dx          ; ��p�� � ��p���⠭���� ��⮢, �������
        and     al, not 0Ch     ; ����� ᨭ�p�����樨
        or      al, cl
        mov     dl, 0C2h        ; ������ � p�����p �p�������: ��p� 3C2h
        out     dx, al
        mov     dl, 0DAh        ; ������� ����� � ����p����p� ��p���⮢
        in      al, dx          ; ���p���⢮� �⥭�� �� ��p� 3DAh
        mov     dl, 0C0h
        mov     al, 13h         ; ������� ����� � p�����p� ��p����⠫쭮��
        out     dx, al          ; ᤢ���: ��p� 3C0h, p�����p 13h
        mov     al, ch          ; ��⠭����� ����稭� ᤢ���
        out     dx, al
        mov     dl, 0DAh        ; ������� ����� � ����p����p� ��p���⮢
        in      al, dx
        mov     dl, 0C0h        ; ���p���� ����p��� ����p������
        mov     al, 20h
        out     dx, al
        mov     dl, 0C4h        ; ��p������ ����� ᨭ�p�����樨
        mov     ax, 300h
        out     dx, ax
        sti
    @@exit:
        ret
endp



;���������������������������������������������������������������������������ͻ
;�������������������������������� ��������� ����������������������������������
;���������������������������������������������������������������������������ͼ

; ������ ���� ���������� � 㪠������ ����樨 �� ��⨢��� �������࠭��
; �� �室�:
;       dl      - x
;       dh      - y
; �� ��室�:
;       es:[di] - ���� ����������
;       bx      - �ਭ� ��࠭� � ᨬ�����
proc vid_GetAddr uses dx
        assume  es: BIOS
        xor     ax, ax
        mov     es, ax          ; es - ᥣ���� ������ BIOS
        mov     bx, [VID_columns]; �ਭ� ��࠭� � ᨬ�����
        mov     al, dh          ; ax - y
        xor     dh, dh
        mov     di, dx          ; di - x
        mul     bx              ; ax = width*y
        add     di, ax
        mov     dx, [CRT_base]
        shl     di, 1           ; ���뢠�� ����� ��ਡ�⮢
        mov     ax, 0B000h      ; �।�������� �� � ����稨 �����஬�� ������
        add     di, [VID_pageoffs] ; ������塞 ᬥ饭�� ��⨢��� �������࠭���
        cmp     dx, 3D4h
        jne     @@done
        add     ax, 800h
    @@done:
        mov     es, ax          ; es - ᥣ���� ����������
        ret
        assume  es: nothing
endp

; �����頥� �ਭ� ��࠭�
; �� ��室�:
;   AX          - �ਭ� � ᨬ�����
proc vid_ScrWidth
        assume  es: BIOS
        xor     ax, ax
        mov     es, ax
        mov     ax, [VID_columns]
        ret
        assume  es: nothing
endp


; �����頥� ����� ��࠭�
; �� ��室�:
;   AX          - ����
proc vid_ScrHeight
        assume  es: BIOS
        xor     ax, ax
        mov     es, ax
        mov     al, [VID_rows]  ; al - ����� ��᫥���� ����� ��࠭�
        inc     ax
        ret
        assume  es: nothing
endp


;proc vid_SetCursor uses bp, @@top: byte, @@bottom: byte
;        mov     ah, 1
;        mov     cl, [@@bottom]  ; ����� ������ ����� �����
;        mov     ch, [@@top]     ; ����� ���孥� ����� �����
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


;���������������������������������������������������������������������������ͻ
;����������������������������������� ���� ������������������������������������
;���������������������������������������������������������������������������ͼ

; ���뢠�� ����� � ���᪮�� 2 ������������ (䮭�)
proc vid_CharGenOpen
        ; ࠧ�蠥� ������ � ᫮� 2 (������������) � ����頥� ������ � ᫮� 0 � 1 (���� ᨬ����� � �� ��ਡ���)
        mov     dx, 3C4h
        mov     ax, 402h
        out     dx, ax          ; set map mask 4 : only plane 2
        ; �롨ࠥ� ०�� ����������
        mov     ax, 704h
        out     dx, ax          ; set memory model (text, >64k,seg.)
        ; �롨ࠥ� �奬� �⥭�� � ॣ���� ����᪮�� ����஫���
        mov     dx, 3CEh
        mov     ax, 204h
        out     dx, ax          ; read map select: plane 2
        ; ����ࠨ���� ०�� ����� � ���뢠��� ����᪮�� ����஫���
        mov     ax, 5
        out     dx, ax          ; write mode 0
        ; ����ࠢ�塞 ॣ���� ᬥ蠭��� ������
        mov     ax, 406h
        out     dx, ax          ; memory mapping for CPU : a000h, 64k.
        ret
endp




; ����뢠�� ����� � ���᪮�� 2 ������������
proc vid_CharGenClose
        xor     ax, ax
        mov     es, ax
        ; �����⠭�������� ०�� ����������
        mov     dx,3C4h
        mov     ax,302h
        out     dx,ax
        ; ࠧ�蠥� ������ � ᫮� 0 � 1, � ����頥� � ᫮� 2
        mov     ax,304h
        out     dx,ax
        ; ����⠭�������� �奬� �⥭�� � ॣ���� ����᪮�� ����஫���
        mov     dx,3CEh
        mov     ax,4
        out     dx,ax
        ; ����⠭�������� ०�� ����� � ���뢠��� ����᪮�� ����஫���
        mov     ax,1005h
        out     dx,ax
        ; ����⠭�������� ᥣ���� ���� ����㯠 � ���᪮���
        mov     ax, 0A06h
        cmp     [word es:463h], 3B4h
        jz      @@1
        mov     ah, 0Eh
    @@1:
        out     dx, ax
        ret
endp



; ��⠭���� ���짮��⥫�᪮�� 䮭�
; �� �室�:
;   lpFont      - 㪠��⥫� �� 䮭�
proc vid_SetFont uses ds _si _di, @@pFont: ptr byte, @@chrHeight: byte, @@chrFirst: byte, @@chrNum: word
        cld
        cli
        call    vid_CharGenOpen
        mov     ax, 0A000h
        mov     es, ax                  ; es - ᥣ���� ���᪮�� ������������
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
;        mov     ds, ax                  ; ds - ᥣ���� ���᪮�� ������������
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



;���������������������������������������������������������������������������ͻ
;���������������������������������� ����� ������������������������������������
;���������������������������������������������������������������������������ͼ

; �뢮� ��ப� � ���� ������ CX, � ����������� ���⪠ ���� ᨬ������ AH
; �� �室�:
;   es:di       - video addr
;   ds:si       - ��ப�
;   cx          - ����� ���� �뢮��
;   ah          - ᨬ���-�������⥫� ���⮣� ����
; �� ��室�:
;   es:di       - ᫥�. ���. ����
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

; �뢮� �᫠ � ���� ������ CX, � ��ࠢ�������� �� �ࠢ��� ��� � �����������
; ���⪠ ���� �஡�����
; �� �室�:
;   es:di       - ���� ����������
;   si:bx       - 32-� ��⭮� �᫮
;   cx          - ����� ���� �뢮��
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


; �� �室�:
;   es:di       - ���� ����������
;   al          - �᫮
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
        call    vid_GetAddr             ; bx - �ਭ� ��࠭� � ᨬ�����
        LDS_    si, [@@pImg]
        lodsw
        sub     bx, ax
        mov     dx, ax                  ; dx - �ਭ� ��⬠��
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
        call    vid_GetAddr             ; bx - �ਭ� ��࠭� � ᨬ�����
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




; �뢮� ⥪�� � ���� �������� �����:
; �� �室�:
;   x,y   - ���न���� ���� �뢮��
;   len   - ����� ���� (� ᨬ�����)
;   pText - ��ப�
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
        ; ���⮩ ᨬ���
        cmp     cx, [@@max]
        jae     @@loop

        push    bx
        ; ���ᨬ ᨬ��� � ����
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


    @@esc:      ; �����頥� ������ ��ப�
        call    @@showcursor
        xor     al, al
        mov     [si], al
        ret

    @@isenter:
        cmp     al, 13
        je      @@enter
        cmp     al, 8
        jne     @@loop
    @@backspc:  ; �����
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

    @@curright: ; ��६�饭�� ����� ��ࠢ�
        mov     al, [si+bx]
        or      al, al
        jz      @@loop
        call    @@showcursor
        inc     bx
        call    @@showcursor
        jmp     @@loop

    @@curleft:  ; ��६�饭�� ����� �����
        or      bx, bx
        jz      @@loop
        call    @@showcursor
        dec     bx
        call    @@showcursor
        jmp     @@loop


    @@enter:    ; ��室��
        call    @@showcursor            ; ��뢠�� �����
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
; �뢮� ��ப� � ���� ������ CX, � ����������� ���⪠ ���� ᨬ������ AH
; �� �室�:
;   es:di       - video addr
;   ds:si       - ��ப�
;   cx          - ����� ���� �뢮��
;   ah          - ᨬ���-�������⥫� ���⮣� ����
; �� ��室�:
;   es:di       - ᫥�. ���. ����




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
        ; ��頥� ��ப�
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
        ; �뢮��� ��� 䠩��
        mov     cx, 12
        LDS_    si, [@@pName]
        mov     ah, ' '
        call    vid_TextLeftJust
        ; ��� ࠧ���
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
        ; ���� ᮧ�����
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
        ; �६� ᮧ�����
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


; in: dx:ax - �������
;        cx - ����⥫�
; out:dx:ax - ��⭮�
;        si - ���⮪
proc uldiv
        xor     bx, bx
        cmp     dx, cx
        jb      @@one_div
        ; ����� ������ ����
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
        ; ��ॢ���� � ���
        mov     cx, 3600
        call    uldiv                   ; si = ticks % 3600
        mov     [@@hour], al
        mov     ax, si
        xor     dx, dx
        mov     cx, 60
        div     cx                      ; ax - min, dx - sec
        mov     [@@min], al
        mov     [@@sec], dl
        ; �뢮� �� ��࠭
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

