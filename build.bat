@echo off
echo Compiling web server...
cl /Fe:webserver.exe src\*.c /I include /link Ws2_32.lib Mswsock.lib
echo.
echo Build complete.
