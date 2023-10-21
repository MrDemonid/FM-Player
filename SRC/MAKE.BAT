SET BCDIR=D:\BC

SET INCLUDE=%BCDIR%\INCLUDE
SET LIB=%BCDIR%\LIB
SET PATH=%BCDIR%\BIN;%PATH%
cls
del .\OUT\*.obj
del .\OUT\*.map
del .\OUT\*.exe
del fmplay.exe

TASM /M3 /MX /ZN /O VIDEO.ASM, .\OUT\VIDEO.OBJ
TASM /M3 /MX /ZN /O FILEIO.ASM, .\OUT\FILEIO.OBJ
TASM /M3 /MX /ZN /O KEYB.ASM, .\OUT\KEYB.OBJ
TASM /M3 /MX /ZN /O OPL.ASM, .\OUT\OPL.OBJ
TASM /M3 /MX /ZN /O TIMER.ASM, .\OUT\TIMER.OBJ
TASM /M3 /MX /ZN /O VISUAL.ASM, .\OUT\VISUAL.OBJ


bcc +FMPLAY.CFG -I%INCLUDE% -L%LIB% -c d:fmplay.cpp
bcc +FMPLAY.CFG -I%INCLUDE% -L%LIB% -c d:file.cpp d:player.cpp d:sa2.cpp
bcc +FMPLAY.CFG -I%INCLUDE% -L%LIB% -c d:hsc.cpp d:protrack.cpp d:rad.cpp d:amd.cpp
bcc +FMPLAY.CFG -I%INCLUDE% -L%LIB% -c d:s3m.cpp d:dmo.cpp d:lds.cpp d:images.cpp

tlink /s/n/c/d/P-/L%LIB% @link.txt


copy .\OUT\fmplay.exe ..\BIN\
::copy fmplay.exe D:\DosDisk\C

del .\OUT\*.obj
del .\OUT\*.map
del .\OUT\*.exe
