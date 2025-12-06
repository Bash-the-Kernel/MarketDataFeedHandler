@echo off
echo Starting benchmark mode...
echo.
start "Feed Handler" build\Release\feed_handler.exe --udp --benchmark
timeout /t 2 /nobreak >nul
start "Feed Simulator" build\Release\feed_simulator.exe --udp --rate 100000 --benchmark
echo.
echo Both processes started. Press Ctrl+C to stop.
pause
