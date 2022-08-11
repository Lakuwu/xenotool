@echo off
if not exist bin ( mkdir bin )
cls
gcc -std=c11 -fno-omit-frame-pointer -Wall -Wpedantic -static-libgcc -ggdb -o ./bin/xenotool.exe ./src/xenotool.c ./src/xeno_lex.c ./src/xeno_xtx.c ./src/xeno_arx.c ./src/xeno_jnt.c ./src/xenodebug.c
REM gcc -std=c11 -fno-omit-frame-pointer -Wall -Wpedantic -static-libgcc -ggdb -o ./bin/xenotool.exe ./src/xenotool.c ./src/xeno_lex.c ./src/xeno_xtx.c ./src/xenodebug.c -lduma