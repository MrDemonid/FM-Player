record HARDWARE LPT_Cnt     :2, \; ������⢮ ��ࠫ����� ���⮢
                Reserved_13 :1, \;
                IsGame      :1, \; ����稥 ���� �����⨪�
                COM_Cnt     :3, \; ������⢮ ��᫥����⥫��� ���⮢
                Reserved_8  :1, \;
                FDD_Cnt     :2, \; ������⢮ ��᪮�����-1
                InitVidMode :2, \; ��砫�� �����०��: 00=EGA/VGA, 01=40x25CGA, 10=80x25CGA, 11=80x25mono
                Reserved_3  :1, \;
                IsPS2       :1, \; ����稥 ���ன�⢠ PS/2 (��誨)
                IsFPU       :1, \; ����稥 ᮯ�����
                IsFDD       :1   ; ����稥 ��᪮�����

record KBDFLAGS Ins_press   :1, \; 䫠� ������ Insert
                Caps_press  :1, \; 䫠� ������ Caps Lock
                Num_press   :1, \; 䫠� ������ Num Lock
                Scroll_press:1, \; 䫠� ������ Scroll Lock
                Pause       :1, \; Pause state active
                SysReq_press:1, \
                LAlt_press  :1, \; 䫠� ������ Left Alt
                LCtrl_press :1, \; 䫠� ������ Left Ctrl
                Insert      :1, \
                CapsLock    :1, \
                NumLock     :1, \
                ScrollLock  :1, \
                Alt_press   :1, \; either Alt pressed
                Ctrl_press  :1, \; either Ctrl pressed
                LShift_press:1, \; 䫠� ������ Left Shift
                RShift_press:1   ; 䫠� ������ Right Shift

record KBDSTATUS                \
                LED_Caps    :1, \; Caps Lock LED               \
                LED_NumLock :1, \; Num Lock LED
                LED_Scroll  :1, \; Scroll Lock LED
                Key_5_press :3, \; ������ '5' �� ���. ���������
                IsAT        :1, \; 101-key enhanced keyboard present
                RAlt_press  :1, \; Right Alt pressed
                RCtrl_press :1, \; Right Ctrl pressed
                Key_0_press :2   ; ������ '0/Ins' �� ���. ���������


record VIDCONTROL               \
                clear_bit   :1, \; high bit of video mode
                                \; =1 screen is not cleared
                RAM_64K     :2, \; RAM on adapter-1 in 64K
                Reserved_4  :1, \
                in_active   :1, \; EGA/VGA video system inactive
                IsWait      :1, \; wait for display enable
                IsMono      :1, \; display is mono
                NotEmulate  :1   ; =0 cursor shape treated as CGA ones

record VIDSWITCHES              \; EGA/VGA video switches
                VIDSW_feature   :3,     \; power-on state of feature
                                        \; connector bits 3-1
                VIDSW_feature0  :1,     \; =0 enhanced color display (ECD)
                VIDSW_display   :4      ; adapters+attached display
                ;       primary                 secondary
                ; =00h  MDA                     EGA+40x25 color display
                ; =01h  MDA                     EGA+80x25 color display
                ; =02h  MDA                     EGA+ECD (CGA emulation)
                ; =03h  MDA                     EGA+ECD
                ; =04h  CGA+40x25 color display EGA+mono display
                ; =05h  CGA+80x25 color display EGA+mono display
                ; =06h  EGA+40x25 color display MDA
                ; =07h  EGA+80x25 color display MDA
                ; =08h  EGA+ECD (CGA emulation) MDA
                ; =09h  EGA+ECD                 MDA
                ; =0Ah  EGA+mono display        CGA+40x25 color display
                ; =0Ah  EGA+mono display        CGA+80x25 color display

record VGAOPTIONS                       \; MCGA/VGA mode set option control
                VGA_scan2       :1,     \; =0 350-line mode when VGA_scan=0
                                        \; =1 200-line mode when VGA_scan=0
                VGA_DCC_enabled :1,     \; DCC display switching enabled
                VGAOPTIONS_1    :1,     \
                VGA_scan        :1,     \; =1 400-line mode at next mode set
                                        \; =0 350/200-line mode
                VGA_nopalette   :1,     \; default palette loading disabled
                VGA_grayscale   :1,     \; gray scale summing enabled
                VGA_active      :1      ; =1 VGA active (MCGA always 0)



segment BIOS at 0000h
  IVT           dd 256 dup(?)   ; ������ ����஢ ���뢠���
  COM_base      dw ?,?,?,?      ; base IO address of serial ports
  LPT_base      dw ?,?,?        ; base IO address of parallel ports
  XBDA_seg      dw ?            ; ᥣ���� eXtended BIOS data area
  HW_lisi       HARDWARE ?      ; ����稥 � ������⢮ ���ன�� � ��⥬�
                db ?
  MEM_size      dw ?            ; ࠧ��� ������� ����� � �� (�� 640��)
                dw ?
  KBD_flags     KBDFLAGS ?      ; 䫠�� ���ﭨ� ��. ������ ����������
                db ?            ; Alt-nnn keypad workspace
  KBD_head      dw ?            ; pointer to next character in buffer
  KBD_tail      dw ?            ; pointer to free space in buffer
  KBD_buffer    dw 16 dup(?)    ; ���� ����������
                db 11 dup(?)
  VID_mode      db ?            ; ⥪�騩 ����� ०��
  VID_columns   dw ?            ; �ਭ� ��࠭� � ᨬ�����
  VID_pagesize  dw ?            ; ࠧ��� �������࠭��� � �����
  VID_pageoffs  dw ?            ; ᬥ饭�� ��⨢��� �������࠭���
  CUR_pos       dw 8 dup(?)     ; ���न���� ����� ��� ���쬨 �������࠭��
  CUR_shape     dw ?            ; ����� ��ࢮ�(high) � ��᫥����(low) ��ப ��㭪� �����
  VID_pageno    db ?            ; ����� ��⨢��� �������࠭���
  CRT_base      dw ?            ; ������ ���� ����஫��� ���
  CRT_mode      db ?            ; ���祭�� ॣ���� 3x8 (Mode Control CGA,MDA)
  CRT_palette   db ?            ; ���祭�� ॣ���� 3x9 (Color Select CGA)
                db 5 dup(?)     ;
  BIOS_timer    dd ?            ; timer ticks since midnight
  BIOS_overflow db ?            ; 䫠� ��९������� ⠩��� (BIOS_timer)
  KBD_break     db ?            ; bit 7 - 䫠� ������ Ctrl+Break
  POST_reset    dw ?            ; specifies the action at POST when reboot
                db ?
  HDD_cnt       db ?            ; ������⢮ ���⪨� ��᪮�
                db ?,?
  LPT_timeout   db ?,?,?        ; ���稪� ⠩���⮢ LPT-���ன��
  INT_4B_flags  db ?            ; INT 4B flags
  COM_timeout   db ?,?,?,?      ; ���稪� ⠩���⮢ COM-���⮢
  KBD_buffptr   dw ?,?          ; keyboard buffer start/end+1 offsets in 40:0


  VID_rows      db ?            ; ����� ��᫥���� ��ப� ��࠭� (��稭�� � 0)
  VID_charsize  dw ?            ; ࠧ��� ��㭪� ᨬ���� � �����
  VID_control   VIDCONTROL ?    ; EGA/VGA control
  VID_switches  VIDSWITCHES ?   ; EGA/VGA switches
  VGA_options   VGAOPTIONS ?    ; MCGA/VGA mode set option control
  VGA_DCC_index db ?            ; index into display combination code table
                db 11 dup(?)
  KBD_status    KBDSTATUS ?     ; keyboard status flags
  TMR_waitflag  dd ?            ; pointer to user wait flag
  TMR_waitmks   dd ?            ; wait counter in mks
  TMR_active    db ?            ; =0 acknowledged, =1 busy, =80h elapsed
                db 7 dup(?)

  VID_ptrtable@ dd ?            ; pointer to video save pointer table
        org 04f0h
  IAC           db 16 dup(?)    ; Inter-Application Communication Area
ends
