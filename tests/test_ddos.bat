@echo off
for /L %%i in (1,1,200) do (
  start /B cmd /C "curl -s http://127.0.0.1:8080/ > nul"
)
echo Launched 200 requests
