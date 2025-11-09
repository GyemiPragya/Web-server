@echo off
cd ..
bin\webserver.exe config\server.conf > server_out.txt 2>&1 &
echo Server started (background). Waiting 1s...
timeout /t 1 /nobreak > nul
echo GET /
curl -v http://127.0.0.1:8080/
echo GET /metrics
curl -v http://127.0.0.1:8080/metrics
echo Done. Ctrl+C the server or close window to stop.
