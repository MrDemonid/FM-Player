        ideal
        p8086
        model   large, c
        smart           ; ������� ��⨬��쭮�� ����
        locals  @@      ; p��p�蠥� ������� �����䨪��p�
        nojumps         ; ���p�頥� TASM'� ����⠢���� NOP'�

        %noincl


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


struc DTA
  reserv        db 21 dup(?)
  fattr         db ?
  ftime         dw ?
  fdate         dw ?
  fsize         dd ?
  fname         db 13 dup(?)
ends

SEEK_SET        equ 0
SEEK_CUR        equ 1
SEEK_END        equ 2



public io_getcurdisk
public io_changedisk
public io_disksavail

public io_getcurdir
public io_chdir

public io_findfirst
public io_findnext

public io_fopen
public io_fcreate
public io_fread
public io_write
public io_fseek
public io_filesize
public io_feof
public io_fclose
public io_gettime


;���������������������������������������������������������������������������ͻ
;��������������������������� ������������� ������� ���������������������������
;���������������������������������������������������������������������������ͼ


codeseg


proc io_getcurdisk
        mov     ah, 19h
        int     21h
        xor     ah, ah
        ret
endp


;  �����祭�� ���ன�⢠ ⥪�騬
;  �� �室�:
;    @@disk      - 0 = A:, 1 = B:, 2 = C:
;
proc io_changedisk uses ds _si _di, @@disk: byte
        mov     ah, 0Eh
        mov     dl, [@@disk]
        int     21h
        ret
endp




; �ନ��� ��ப� � ������� ����㯭�� ��᪮� ����: " A  C  D "
; �� ��室�: ������⢮ ��᪮� (0..26)
;
proc io_disksavail uses ds _si _di, @@szdisks: ptr byte
  local @@buf:byte:4
  local @@fcb:byte:40
  local @@res:byte:80
        mov     ax, ss
        mov     ds, ax
        mov     es, ax
        lea     si, [@@buf]
        lea     bx, [@@res]
        xor     cx, cx
        mov     [byte si], 'A'-1
    @@loop:
        lea     di, [@@fcb]
        inc     [byte si]
        mov     [byte si+1], ':'
        mov     [byte si+2], 0
        mov     ax, 2906h
        int     21h
        lea     si, [@@buf]
        cmp     al, 0ffh
        mov     al, [si]
        je      @@next
        ; ������塞 ���ன�⢮ � ᯨ᮪
        mov     [byte bx], ' '
        mov     [bx+1], al
        mov     [byte bx+2], ' '
        add     bx, 3
        inc     cx
    @@next:
        cmp     al, 'Z'
        jb      @@loop
        ; �����㥬 १���� � ��ப�
        push    cx
        lea     si, [@@res]
        mov     cx, bx
        sub     cx, si
        les     di, [@@szdisks]
        rep     movsb
        xor     al, al
        stosb
        pop     ax
        ret
endp




;���������������������������������������������������������������������������ͻ
;��������������������������� ������ � ������������ ���������������������������
;���������������������������������������������������������������������������ͼ


proc io_getcurdir uses ds _si _di, @@drive: byte, @@pDirRec: ptr byte
        mov     ah, 47h
        mov     dl, [@@drive]
        LDS_    si, [@@pDirRec]
        xor     bx, bx          ; bx - return code
        int     21h
        jc      @@done
        dec     bx
    @@done:
        mov     ax, bx          ; ax - return code
        ret
endp


proc io_chdir uses ds, @@pPath: ptr byte
        mov     ah, 3bh
        LDS_    dx, [@@pPath]
        int     21h
        mov     ax, 0
        jc      @@done
        or      ax, -1
    @@done:
        ret
endp


;���������������������������������������������������������������������������ͻ
;������������������������������� ����� ������ ��������������������������������
;���������������������������������������������������������������������������ͼ

proc io_findfirst uses ds _si _di, @@path: ptr byte, @@attrib: word, @@finfo: ptr DTA
        ; ����砥� ���騩 DTA
        mov     ah, 2fh
        int     21h
        push    es
        push    bx
        ; ��⠭�������� ᢮�
        mov     ah, 1ah
        LDS_    dx, [@@finfo]
        int     021h
        ; �ந������ ���� ��ࢮ�� �宦�����
        xor     bx, bx          ; bx - return code
        mov     ah, 4eh
        mov     cx, [@@attrib]
        LDS_    dx, [@@path]
        int     021h
        jnc     @@done
        dec     bx
    @@done:
        ; ����⠭�������� �०��� DTA
        mov     ah, 1ah
        pop     dx
        pop     ds
        int     21h
        mov     ax, bx          ; ax - return code
        ret
endp


proc io_findnext uses ds _si _di, @@finfo: ptr DTA
        ; ����砥� ���騩 DTA
        mov     ah, 2fh
        int     21h
        push    es
        push    bx
        ; ��⠭�������� ᢮�
        mov     ah, 1ah
        LDS_    dx, [@@finfo]
        int     021h
        ; �ந������ ���� ᫥���饣� �宦�����
        xor     bx, bx          ; bx - return code
        mov     ah, 4fh
        int     21h
        jnc     @@done
        dec     bx
    @@done:
        ; ����⠭�������� �०��� DTA
        mov     ah, 1ah
        pop     dx
        pop     ds
        int     21h
        mov     ax, bx          ; ax - return code
        ret
endp



;���������������������������������������������������������������������������ͻ
;������������������������������� ������ ������ �������������������������������
;���������������������������������������������������������������������������ͼ



proc io_fopen uses ds _si _di, @@fname: ptr byte, @@flags: byte
        ; ���뢠�� 䠩�
        mov     al, [@@flags]
        mov     ah, 3Dh
        and     al, 111b
        LDS_    dx, [@@fname]
        int     21h
        jnc     @@exit
        xor     ax, ax
    @@exit:
        ret
endp

proc io_fcreate uses ds _si _di, @@fname: ptr byte, @@flags: word
        mov     cx, [@@flags]
        mov     ah, 3Ch
        LDS_    dx, [@@fname]
        int     21h
        jnc     @@exit
        xor     ax, ax
    @@exit:
        ret
endp



proc io_fread uses ds _si _di, @@buff: ptr byte, @@rsize: word, @@fhandle: word
        mov     ah, 3fh
        mov     bx, [@@fhandle]
        mov     cx, [@@rsize]
        LDS_    dx, [@@buff]
        int     21h
        jnc     @@exit
        xor     ax, ax
    @@exit:
        ret
endp

proc io_write uses ds _si _di, @@buff: ptr byte, @@wsize: word, @@fhandle: word
        mov     ah, 40h
        mov     bx, [@@fhandle]
        mov     cx, [@@wsize]
        LDS_    dx, [@@buff]
        int     21h
        jnc     @@exit
        xor     ax, ax
    @@exit:
        ret
endp


proc io_fseek uses ds _si _di, @@fhandle: word, @@offs: dword, @@whence: byte
        mov     ah, 42h
        mov     al, [@@whence]
        mov     bx, [@@fhandle]
        mov     cx, [word @@offs+2]
        mov     dx, [word @@offs]
        int     21h
        jnc     @@exit
        or      dx, -1          ; �訡��, �����頥� 0xFFFFFFFF
        or      ax, -1
    @@exit:
        ret
endp

proc io_filesize uses ds _si _di, @@fhandle: word
        ; ���������� ⥪���� ������
        call    io_fseek, [@@fhandle], 0, 0, SEEK_CUR
        push    ax dx
        ; ���室�� � ����� 䠩��
        call    io_fseek, [@@fhandle], 0, 0, SEEK_END
        pop     bx cx
        push    dx ax
        ; ����⠭�������� ������ � 䠩��
        call    io_fseek, [@@fhandle], bx cx, SEEK_SET
        pop     ax dx
        ret
endp


proc io_feof uses ds _si _di, @@fhandle: word
        mov     ax, 4400h
        mov     bx, [@@fhandle]
        int     21h
        jc      @@eof_failed
        test    dl, 080h
        jnz     @@not_eof

        ; ����砥� ࠧ��� 䠩��
        call    io_filesize, [@@fhandle]
        push    dx ax
        ; ����砥� ⥪���� ������
        call    io_fseek, [@@fhandle], 0, 0, SEEK_CUR
        pop     cx bx
        ; �ࠢ������
        cmp     dx, bx
        jb      @@not_eof
        ja      @@is_eof
        cmp     ax, cx
        jb      @@not_eof
    @@eof_failed:
    @@is_eof:
        or      ax, -1
        ret
    @@not_eof:
        xor     ax, ax
        ret
endp


proc io_fclose uses ds _si _di, @@fhandle: word
        mov     ah, 3eh
        mov     bx, [@@fhandle]
        int     21h
        xor     ax, ax
        ret
endp


proc io_gettime uses ds _si _di, @@fhandle: word, @@date: ptr byte, @@time: ptr byte
        mov     ax, 5700h
        mov     bx, [@@fhandle]
        int     21h
        mov     ax, 0
        jc      @@done          ; -> error
        LDS_    di, [@@date]
        mov     [di], dx
        LDS_    di, [@@time]
        mov     [di], cx
        xor     ax, ax
    @@done:
        ret
endp





end

















