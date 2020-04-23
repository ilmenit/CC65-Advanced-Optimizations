SET PATH=%PATH%;C:\CC65\BIN
SET CC65_HOME=C:\CC65
SET CC65_INC=C:\CC65\INCLUDE
SET LD65_LIB=C:\CC65\LIB
SET LD65_CFG=C:\CC65\CFG

cl65 -t atari  -Ln game.lbl -Osir -Cl --listing game.lst --add-source -o game.xex game.c || goto :error
start game.xex
goto :end

:error
pause
goto :end

:end
