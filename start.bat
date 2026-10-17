@echo off
title Smart AIoT Irrigation Server
echo ========================================================
echo       SMART AIOT IRRIGATION SERVER
echo ========================================================
echo Web Dashboard : http://localhost:8000
echo ESP32 Host IP : http://10.185.32.62:8000
echo API Docs      : http://localhost:8000/docs
echo ========================================================
echo.

if exist ".\.venv\Scripts\python.exe" (
    ".\.venv\Scripts\python.exe" -m uvicorn app:app --host 0.0.0.0 --port 8000 --reload
) else (
    python -m uvicorn app:app --host 0.0.0.0 --port 8000 --reload
)

pause
