#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart AIoT Precision Irrigation & ESP32 Unified Dashboard</title>
    <!-- Google Fonts -->
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=Outfit:wght@400;600;700;800&display=swap" rel="stylesheet">
    <!-- FontAwesome Icons -->
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <!-- Chart.js -->
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

    <style>
        :root {
            --bg-dark: #090d16;
            --card-bg: rgba(18, 26, 43, 0.75);
            --card-border: rgba(255, 255, 255, 0.08);
            --accent-emerald: #10b981;
            --accent-emerald-glow: rgba(16, 185, 129, 0.3);
            --accent-cyan: #06b6d4;
            --accent-blue: #3b82f6;
            --accent-amber: #f59e0b;
            --accent-rose: #f43f5e;
            --text-primary: #f8fafc;
            --text-secondary: #94a3b8;
            --text-muted: #64748b;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: 'Inter', sans-serif;
        }

        body {
            background-color: var(--bg-dark);
            background-image: 
                radial-gradient(circle at 15% 15%, rgba(16, 185, 129, 0.12) 0%, transparent 40%),
                radial-gradient(circle at 85% 85%, rgba(6, 182, 212, 0.12) 0%, transparent 40%);
            color: var(--text-primary);
            min-height: 100vh;
            padding: 24px;
        }

        .dashboard-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 24px;
            padding-bottom: 20px;
            border-bottom: 1px solid var(--card-border);
            flex-wrap: wrap;
            gap: 16px;
        }

        .brand {
            display: flex;
            align-items: center;
            gap: 14px;
        }

        .brand-icon {
            width: 50px;
            height: 50px;
            background: linear-gradient(135deg, var(--accent-emerald), var(--accent-cyan));
            border-radius: 14px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 24px;
            color: #000;
            box-shadow: 0 0 20px var(--accent-emerald-glow);
        }

        .brand-text h1 {
            font-family: 'Outfit', sans-serif;
            font-size: 26px;
            font-weight: 700;
            letter-spacing: -0.5px;
            background: linear-gradient(to right, #ffffff, #cbd5e1);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .brand-text p {
            font-size: 13px;
            color: var(--text-secondary);
        }

        .header-actions {
            display: flex;
            align-items: center;
            gap: 10px;
            flex-wrap: wrap;
        }

        .btn-profile, .btn-settings {
            display: flex;
            align-items: center;
            gap: 8px;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid var(--card-border);
            color: var(--text-primary);
            padding: 8px 14px;
            border-radius: 20px;
            font-size: 13px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
        }

        .btn-profile:hover, .btn-settings:hover {
            background: rgba(255, 255, 255, 0.1);
            border-color: rgba(255, 255, 255, 0.2);
            transform: translateY(-1px);
        }

        .status-badge {
            display: flex;
            align-items: center;
            gap: 8px;
            background: rgba(16, 185, 129, 0.1);
            border: 1px solid rgba(16, 185, 129, 0.3);
            padding: 8px 14px;
            border-radius: 30px;
            font-size: 12px;
            font-weight: 600;
            color: var(--accent-emerald);
            transition: all 0.3s;
        }

        .status-badge.offline {
            background: rgba(244, 63, 94, 0.1);
            border-color: rgba(244, 63, 94, 0.3);
            color: var(--accent-rose);
        }

        .status-dot {
            width: 8px;
            height: 8px;
            background-color: var(--accent-emerald);
            border-radius: 50%;
            box-shadow: 0 0 10px var(--accent-emerald);
            animation: pulse 2s infinite;
        }

        .status-badge.offline .status-dot {
            background-color: var(--accent-rose);
            box-shadow: 0 0 10px var(--accent-rose);
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.4; transform: scale(1.2); }
        }

        .mode-pill {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid var(--card-border);
            padding: 6px 14px;
            border-radius: 20px;
            font-size: 12px;
            color: var(--accent-cyan);
            font-weight: 600;
        }

        .grid-container {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 24px;
        }

        @media (max-width: 1024px) {
            .grid-container {
                grid-template-columns: 1fr;
            }
        }

        .card {
            background: var(--card-bg);
            backdrop-filter: blur(16px);
            border: 1px solid var(--card-border);
            border-radius: 20px;
            padding: 24px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            transition: border-color 0.3s ease;
        }

        .card:hover {
            border-color: rgba(255, 255, 255, 0.15);
        }

        .card-title {
            font-family: 'Outfit', sans-serif;
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 20px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            color: #f1f5f9;
        }

        .card-title-left {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .card-title i {
            color: var(--accent-cyan);
        }

        /* Controls & Form Elements */
        .sensor-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 20px;
        }

        .sensor-control {
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid rgba(255, 255, 255, 0.05);
            padding: 14px;
            border-radius: 14px;
        }

        .sensor-control label {
            display: flex;
            justify-content: space-between;
            font-size: 13px;
            color: var(--text-secondary);
            margin-bottom: 8px;
        }

        .sensor-value {
            font-weight: 600;
            color: var(--accent-cyan);
        }

        input[type="range"] {
            width: 100%;
            height: 6px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 3px;
            outline: none;
            accent-color: var(--accent-emerald);
            cursor: pointer;
        }

        select, input[type="number"], input[type="text"] {
            width: 100%;
            background: rgba(15, 23, 42, 0.8);
            border: 1px solid rgba(255, 255, 255, 0.1);
            padding: 10px 14px;
            border-radius: 10px;
            color: #fff;
            font-size: 14px;
            outline: none;
            transition: border-color 0.2s;
        }

        select:focus, input:focus {
            border-color: var(--accent-emerald);
        }

        .toggle-group {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(110px, 1fr));
            gap: 12px;
            margin-bottom: 20px;
        }

        .toggle-btn {
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid var(--card-border);
            padding: 10px 14px;
            border-radius: 12px;
            color: var(--text-secondary);
            font-size: 13px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            text-align: center;
        }

        .toggle-btn.active-green {
            background: rgba(16, 185, 129, 0.15);
            border-color: var(--accent-emerald);
            color: var(--accent-emerald);
        }

        .toggle-btn.active-red {
            background: rgba(244, 63, 94, 0.15);
            border-color: var(--accent-rose);
            color: var(--accent-rose);
        }

        .mode-btn-group {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
        }

        .mode-btn {
            flex: 1;
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid var(--card-border);
            padding: 12px;
            border-radius: 12px;
            color: var(--text-secondary);
            font-size: 13px;
            font-weight: 600;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            transition: all 0.2s;
        }

        .mode-btn.active {
            background: linear-gradient(135deg, rgba(16, 185, 129, 0.2), rgba(6, 182, 212, 0.2));
            border-color: var(--accent-emerald);
            color: #fff;
        }

        .btn-predict {
            width: 100%;
            background: linear-gradient(135deg, var(--accent-emerald), var(--accent-cyan));
            color: #000;
            border: none;
            padding: 14px;
            border-radius: 12px;
            font-size: 15px;
            font-weight: 700;
            cursor: pointer;
            box-shadow: 0 4px 20px var(--accent-emerald-glow);
            transition: transform 0.2s, box-shadow 0.2s;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }

        .btn-predict:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 25px rgba(16, 185, 129, 0.5);
        }

        /* Right Column Output Styling */
        .output-main {
            text-align: center;
            margin-bottom: 24px;
        }

        .action-banner {
            display: inline-flex;
            align-items: center;
            gap: 12px;
            padding: 16px 36px;
            border-radius: 40px;
            font-size: 22px;
            font-weight: 800;
            font-family: 'Outfit', sans-serif;
            margin-bottom: 12px;
            letter-spacing: 0.5px;
            transition: all 0.3s ease;
        }

        .banner-on {
            background: linear-gradient(135deg, rgba(16, 185, 129, 0.2), rgba(6, 182, 212, 0.2));
            border: 2px solid var(--accent-emerald);
            color: var(--accent-emerald);
            box-shadow: 0 0 30px var(--accent-emerald-glow);
        }

        .banner-off {
            background: rgba(255, 255, 255, 0.03);
            border: 2px solid var(--card-border);
            color: var(--text-muted);
        }

        .stat-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
            gap: 16px;
            margin-bottom: 24px;
        }

        .stat-box {
            background: rgba(255, 255, 255, 0.02);
            border: 1px solid var(--card-border);
            padding: 16px;
            border-radius: 14px;
            text-align: center;
        }

        .stat-box label {
            font-size: 11px;
            color: var(--text-secondary);
            text-transform: uppercase;
            letter-spacing: 0.5px;
            display: block;
            margin-bottom: 6px;
        }

        .stat-box .val {
            font-family: 'Outfit', sans-serif;
            font-size: 22px;
            font-weight: 700;
            color: #fff;
        }

        .chart-container {
            position: relative;
            height: 200px;
            width: 100%;
        }

        /* Alerts Banner */
        .alert-pill {
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 12px 18px;
            border-radius: 12px;
            font-size: 13px;
            font-weight: 500;
            background: rgba(244, 63, 94, 0.1);
            border: 1px solid rgba(244, 63, 94, 0.3);
            color: var(--accent-rose);
        }

        .alert-pill.info {
            background: rgba(6, 182, 212, 0.1);
            border-color: rgba(6, 182, 212, 0.3);
            color: var(--accent-cyan);
        }

        .alert-pill.success {
            background: rgba(16, 185, 129, 0.1);
            border-color: rgba(16, 185, 129, 0.3);
            color: var(--accent-emerald);
        }

        .stop-alarm-btn {
            background: #dc2626;
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 12px;
            font-weight: bold;
            font-size: 14px;
            cursor: pointer;
            animation: pulse 1s infinite;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            margin-bottom: 16px;
        }

        /* Modal Overlay & Card */
        .modal-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100vw;
            height: 100vh;
            background: rgba(0, 0, 0, 0.8);
            backdrop-filter: blur(8px);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 9999;
            padding: 20px;
            animation: fadeIn 0.25s ease-out;
        }

        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        .modal-card {
            background: #0f172a;
            border: 1px solid rgba(255, 255, 255, 0.15);
            border-radius: 20px;
            width: 100%;
            max-width: 620px;
            box-shadow: 0 25px 60px rgba(0, 0, 0, 0.6);
            overflow: hidden;
            animation: slideUp 0.3s cubic-bezier(0.16, 1, 0.3, 1);
        }

        @keyframes slideUp {
            from { transform: translateY(30px) scale(0.97); }
            to { transform: translateY(0) scale(1); }
        }

        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 20px 24px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.08);
        }

        .modal-close-btn {
            background: transparent;
            border: none;
            color: var(--text-muted);
            font-size: 18px;
            cursor: pointer;
            transition: color 0.2s;
        }

        .modal-close-btn:hover {
            color: #fff;
        }

        .modal-body {
            padding: 24px;
            max-height: 70vh;
            overflow-y: auto;
        }

        .modal-label {
            font-size: 12px;
            font-weight: 500;
            color: var(--text-secondary);
            display: block;
            margin-bottom: 6px;
        }

        .modal-footer {
            display: flex;
            justify-content: flex-end;
            gap: 12px;
            padding: 16px 24px;
            background: rgba(0, 0, 0, 0.2);
            border-top: 1px solid rgba(255, 255, 255, 0.08);
        }

        .btn-cancel {
            padding: 10px 18px;
            background: transparent;
            border: 1px solid rgba(255, 255, 255, 0.1);
            color: var(--text-secondary);
            border-radius: 10px;
            font-size: 13px;
            cursor: pointer;
            transition: all 0.2s;
        }

        .btn-cancel:hover {
            background: rgba(255, 255, 255, 0.05);
            color: #fff;
        }

        .btn-save-profile {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 10px 22px;
            background: linear-gradient(135deg, var(--accent-emerald), var(--accent-cyan));
            color: #000;
            border: none;
            border-radius: 10px;
            font-size: 13px;
            font-weight: 700;
            cursor: pointer;
            box-shadow: 0 4px 15px var(--accent-emerald-glow);
            transition: transform 0.2s, opacity 0.2s;
        }

        .btn-save-profile:hover {
            transform: translateY(-1px);
            opacity: 0.95;
        }

        /* Tank Empty Glow */
        .tank-empty-glow {
            border-color: var(--accent-rose) !important;
            animation: alertBorderPulse 1.5s infinite alternate;
        }

        @keyframes alertBorderPulse {
            0% { box-shadow: 0 0 5px rgba(244, 63, 94, 0.4); }
            100% { box-shadow: 0 0 20px rgba(244, 63, 94, 0.8); }
        }
    </style>
</head>
<body>

    <!-- Top Header -->
    <div class="dashboard-header">
        <div class="brand">
            <div class="brand-icon">
                <i class="fa-solid fa-droplet"></i>
            </div>
            <div class="brand-text">
                <h1>Smart AIoT Precision Irrigation Dashboard</h1>
                <p id="dashboard-subtitle">Synchronized ESP32 Hardware, AI Model Decision Engine & Farmer Profile</p>
            </div>
        </div>

        <div class="header-actions">
            <button class="btn-profile" onclick="openProfileModal()">
                <i class="fa-solid fa-wheat-awn"></i> Farm Profile
            </button>
            <button class="btn-settings" onclick="openSettingsModal()">
                <i class="fa-solid fa-gear"></i> Hardware Settings
            </button>
            <div class="mode-pill" id="active-mode-pill">Mode: AI_AUTO</div>
            <div class="status-badge" id="esp32-status-badge">
                <div class="status-dot"></div>
                <span id="esp32-status-text">ESP32 Syncing...</span>
            </div>
            <div class="status-badge" id="ai-status-badge">
                <div class="status-dot"></div>
                <span id="ai-status-text">AI Server: Syncing...</span>
            </div>
        </div>
    </div>

    <!-- Rain Audio Alarm Button (shows when alarm sounds) -->
    <button class="stop-alarm-btn" id="stop-alarm-btn" style="display: none;" onclick="stopRainAlarm()">
        <i class="fa-solid fa-volume-xmark"></i> MUTE RAIN ALARM SOUND
    </button>

    <!-- Live Diagnostics & Safety Alerts Container -->
    <div id="alerts-banner" style="margin-bottom: 24px; display: none; flex-direction: column; gap: 10px;"></div>

    <!-- Main Grid -->
    <div class="grid-container">

        <!-- Left Column: Operational Controls & Hardware Telemetry -->
        <div class="card">
            <div class="card-title">
                <div class="card-title-left">
                    <i class="fa-solid fa-sliders"></i>
                    <span>System Operating Mode</span>
                </div>
            </div>

            <!-- Mode Selector -->
            <div class="mode-btn-group">
                <button class="mode-btn active" id="btn-mode-ai" onclick="setSystemMode('AI_AUTO')">
                    <i class="fa-solid fa-robot"></i> AI Model Auto
                </button>
                <button class="mode-btn" id="btn-mode-thresh" onclick="setSystemMode('THRESHOLD_AUTO')">
                    <i class="fa-solid fa-gauge-high"></i> Threshold Auto
                </button>
                <button class="mode-btn" id="btn-mode-manual" onclick="setSystemMode('MANUAL')">
                    <i class="fa-solid fa-hand"></i> Manual Switch
                </button>
            </div>

            <!-- Threshold Auto Bar (Visible when in Threshold Auto mode) -->
            <div id="threshold-control-section" style="display: none; background: rgba(255,255,255,0.03); border: 1px solid var(--card-border); padding: 14px; border-radius: 14px; margin-bottom: 20px;">
                <label style="display: flex; justify-content: space-between; font-size: 13px; color: var(--text-secondary); margin-bottom: 8px;">
                    <span><i class="fa-solid fa-arrow-down-up-across-line"></i> Soil Moisture Dryness Threshold (ADC)</span>
                    <span class="sensor-value" id="val-thresh-display">2500</span>
                </label>
                <div style="display: flex; gap: 10px; align-items: center;">
                    <input type="range" id="input-thresh-slider" min="500" max="3800" step="50" value="2500" oninput="onThresholdChange(this.value)">
                    <button onclick="saveThresholdSetting()" style="background: var(--accent-emerald); color: #000; border: none; padding: 6px 14px; border-radius: 8px; font-weight: 700; cursor: pointer; white-space: nowrap;">Save</button>
                </div>
            </div>

            <div class="card-title" style="font-size: 15px; margin-top: 10px; margin-bottom: 14px;">
                <div class="card-title-left">
                    <i class="fa-solid fa-microchip"></i>
                    <span>ESP32 Hardware Sensor Telemetry</span>
                </div>
                <span style="font-size: 11px; color: var(--text-muted);" id="last-sync-time">Polling live data...</span>
            </div>

            <div class="sensor-grid">
                <div class="sensor-control">
                    <label><span><i class="fa-solid fa-temperature-half"></i> Temperature</span> <span class="sensor-value" id="val-temp">-- °C</span></label>
                    <input type="range" id="input-temp" min="10" max="50" step="0.5" value="28.5" oninput="manualSliderChange()" onmousedown="isUserDraggingSlider=true" onmouseup="isUserDraggingSlider=false" ontouchstart="isUserDraggingSlider=true" ontouchend="isUserDraggingSlider=false">
                </div>
                <div class="sensor-control">
                    <label><span><i class="fa-solid fa-droplet"></i> Humidity</span> <span class="sensor-value" id="val-hum">-- %</span></label>
                    <input type="range" id="input-hum" min="10" max="100" step="1" value="40" oninput="manualSliderChange()" onmousedown="isUserDraggingSlider=true" onmouseup="isUserDraggingSlider=false" ontouchstart="isUserDraggingSlider=true" ontouchend="isUserDraggingSlider=false">
                </div>
                <div class="sensor-control">
                    <label><span><i class="fa-solid fa-seedling"></i> Soil Value (ADC)</span> <span class="sensor-value" id="val-soil">--</span></label>
                    <input type="range" id="input-soil" min="0" max="4095" step="10" value="450" oninput="manualSliderChange()" onmousedown="isUserDraggingSlider=true" onmouseup="isUserDraggingSlider=false" ontouchstart="isUserDraggingSlider=true" ontouchend="isUserDraggingSlider=false">
                </div>
                <div class="sensor-control">
                    <label><span><i class="fa-solid fa-sun"></i> Light Value (LDR)</span> <span class="sensor-value" id="val-light">--</span></label>
                    <input type="range" id="input-light" min="0" max="4095" step="50" value="1200" oninput="manualSliderChange()" onmousedown="isUserDraggingSlider=true" onmouseup="isUserDraggingSlider=false" ontouchstart="isUserDraggingSlider=true" ontouchend="isUserDraggingSlider=false">
                </div>
                <div class="sensor-control" id="sensor-control-water">
                    <label><span><i class="fa-solid fa-water"></i> Water Level (Tank)</span> <span class="sensor-value" id="val-water">--</span></label>
                    <input type="range" id="input-water" min="0" max="4095" step="50" value="1800" oninput="manualSliderChange()" onmousedown="isUserDraggingSlider=true" onmouseup="isUserDraggingSlider=false" ontouchstart="isUserDraggingSlider=true" ontouchend="isUserDraggingSlider=false">
                </div>
            </div>

            <!-- Toggles for Digital Sensors -->
            <div class="card-title" style="font-size: 15px; margin-top: 10px; margin-bottom: 12px;">
                <div class="card-title-left">
                    <i class="fa-solid fa-toggle-on"></i> Sensor Flags & Physical Relay
                </div>
            </div>
            <div class="toggle-group">
                <button class="toggle-btn active-red" id="btn-rain" onclick="toggleRain()">Rain: NO</button>
                <button class="toggle-btn active-red" id="btn-soil" onclick="toggleSoil()">Soil: DRY</button>
                <button class="toggle-btn active-green" id="btn-tank" onclick="toggleTank()">Tank: AVAILABLE</button>
                <button class="toggle-btn active-red" id="btn-pump-manual" onclick="toggleManualPump()">Pump: OFF</button>
            </div>

            <!-- Farm Configuration -->
            <div class="card-title" style="font-size: 15px; margin-top: 10px; margin-bottom: 12px;">
                <div class="card-title-left">
                    <i class="fa-solid fa-wheat-awn"></i> Crop & Soil Context Sync
                </div>
            </div>
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 20px;">
                <div>
                    <label style="font-size: 12px; color: var(--text-secondary); display: block; margin-bottom: 4px;">Crop Type</label>
                    <select id="select-crop" onchange="syncFarmSettings()">
                        <option value="Wheat" selected>Wheat</option>
                        <option value="Cotton">Cotton</option>
                        <option value="Maize">Maize</option>
                        <option value="Rice">Rice</option>
                        <option value="Sugarcane">Sugarcane</option>
                        <option value="Vegetables">Vegetables</option>
                    </select>
                </div>
                <div>
                    <label style="font-size: 12px; color: var(--text-secondary); display: block; margin-bottom: 4px;">Season</label>
                    <select id="select-season" onchange="syncFarmSettings()">
                        <option value="Winter" selected>Winter</option>
                        <option value="Summer">Summer</option>
                        <option value="Rainy">Rainy</option>
                    </select>
                </div>
                <div>
                    <label style="font-size: 12px; color: var(--text-secondary); display: block; margin-bottom: 4px;">Growth Stage</label>
                    <select id="select-growth" onchange="syncFarmSettings()">
                        <option value="Vegetative" selected>Vegetative</option>
                        <option value="Flowering">Flowering</option>
                        <option value="Maturity">Maturity</option>
                        <option value="Seedling">Seedling</option>
                    </select>
                </div>
                <div>
                    <label style="font-size: 12px; color: var(--text-secondary); display: block; margin-bottom: 4px;">Soil Type</label>
                    <select id="select-soiltype" onchange="syncFarmSettings()">
                        <option value="Loamy" selected>Loamy</option>
                        <option value="Sandy">Sandy</option>
                        <option value="Clay">Clay</option>
                    </select>
                </div>
            </div>

            <button class="btn-predict" onclick="triggerManualInference()">
                <i class="fa-solid fa-bolt"></i> Run Manual AI Inference
            </button>
        </div>

        <!-- Right Column: AI Model Output & Live Visualizations -->
        <div style="display: flex; flex-direction: column; gap: 24px;">
            
            <!-- Output Decision Card -->
            <div class="card">
                <div class="card-title">
                    <div class="card-title-left">
                        <i class="fa-solid fa-brain"></i>
                        <span>AI Model Decision & Pump Output</span>
                    </div>
                </div>

                <div class="output-main">
                    <!-- Status telemetry pills: Desired, Actual Hardware, Mode -->
                    <div style="display: flex; justify-content: center; align-items: center; gap: 10px; margin-bottom: 16px; flex-wrap: wrap;">
                        <div id="pill-desired-state" style="padding: 5px 14px; border-radius: 20px; font-size: 12px; font-weight: 600; background: rgba(255,255,255,0.04); border: 1px solid rgba(255,255,255,0.1); color: var(--text-secondary); display: flex; align-items: center; gap: 6px;">
                            <i class="fa-solid fa-bullseye" style="font-size: 11px;"></i>
                            <span>Desired: <strong id="val-desired-state" style="color: #94a3b8;">PUMP_OFF</strong></span>
                        </div>
                        <div id="pill-target-state" style="padding: 5px 14px; border-radius: 20px; font-size: 12px; font-weight: 600; background: rgba(255,255,255,0.04); border: 1px solid rgba(255,255,255,0.1); color: var(--text-secondary); display: flex; align-items: center; gap: 6px;">
                            <i class="fa-solid fa-tower-broadcast" style="font-size: 11px;"></i>
                            <span>Target Command: <strong id="val-target-state" style="color: #94a3b8;">PUMP_OFF</strong></span>
                        </div>
                        <div id="pill-actual-state" style="padding: 5px 14px; border-radius: 20px; font-size: 12px; font-weight: 600; background: rgba(255,255,255,0.04); border: 1px solid rgba(255,255,255,0.1); color: var(--text-secondary); display: flex; align-items: center; gap: 6px;">
                            <i class="fa-solid fa-microchip" style="font-size: 11px;"></i>
                            <span>Hardware Relay: <strong id="val-actual-state" style="color: #94a3b8;">INACTIVE (LOW)</strong></span>
                        </div>
                        <div id="pill-mode-state" style="padding: 5px 14px; border-radius: 20px; font-size: 12px; font-weight: 600; background: rgba(6, 182, 212, 0.1); border: 1px solid rgba(6, 182, 212, 0.3); color: var(--accent-cyan); display: flex; align-items: center; gap: 6px;">
                            <i class="fa-solid fa-sliders" style="font-size: 11px;"></i>
                            <span>Mode: <strong id="val-mode-state">AI_AUTO</strong></span>
                        </div>
                    </div>

                    <div class="action-banner banner-off" id="action-banner">
                        <i class="fa-solid fa-power-off" id="action-icon"></i>
                        <span id="action-text">PUMP_OFF</span>
                    </div>
                    <p style="font-size: 14px; color: var(--text-secondary); margin-top: 4px; line-height: 1.5;" id="status-reason">
                        Reason: Syncing with AI Model...
                    </p>
                </div>

                <div class="stat-grid" style="grid-template-columns: repeat(2, 1fr); gap: 12px;">
                    <div class="stat-box">
                        <label>AI Irrigation Volume</label>
                        <div class="val" id="res-water">0.0 L</div>
                    </div>
                    <div class="stat-box">
                        <label>Farm Total Requirement</label>
                        <div class="val" id="res-farm-water" style="color: var(--accent-emerald);">0.0 L</div>
                    </div>
                    <div class="stat-box">
                        <label>Pump Flow Rate</label>
                        <div class="val" id="res-flow">2.5 L/m</div>
                    </div>
                    <div class="stat-box">
                        <label>Pump Runtime</label>
                        <div class="val" id="res-runtime" style="color: var(--accent-cyan);">0 s</div>
                    </div>
                </div>
            </div>

            <!-- Live Weather & Rain-Aware Forecast Card -->
            <div class="card">
                <div class="card-title">
                    <div class="card-title-left">
                        <i class="fa-solid fa-cloud-sun-rain"></i>
                        <span>Live Weather & Rain Forecast (Open-Meteo)</span>
                    </div>
                    <span class="mode-pill" style="font-size: 11px;" id="weather-location-pill">Pune, Maharashtra</span>
                </div>
                <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; margin-bottom: 14px;">
                    <div class="stat-box" style="padding: 12px;">
                        <label><i class="fa-solid fa-temperature-three-quarters"></i> Local Temp</label>
                        <div class="val" id="weather-temp" style="font-size: 20px;">-- °C</div>
                    </div>
                    <div class="stat-box" style="padding: 12px;">
                        <label><i class="fa-solid fa-cloud-showers-heavy"></i> Rain Chance (6h)</label>
                        <div class="val" id="weather-rain-prob" style="font-size: 20px; color: var(--accent-cyan);">-- %</div>
                    </div>
                    <div class="stat-box" style="padding: 12px;">
                        <label><i class="fa-solid fa-cloud-rain"></i> Expected Rain</label>
                        <div class="val" id="weather-rain-mm" style="font-size: 20px; color: var(--accent-emerald);">-- mm</div>
                    </div>
                </div>
                <div style="background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 12px; padding: 12px 16px; display: flex; align-items: center; gap: 12px;">
                    <i class="fa-solid fa-cloud-bolt" style="color: var(--accent-cyan); font-size: 20px;"></i>
                    <div style="font-size: 13px;">
                        <div style="font-weight: 600; color: #f1f5f9;" id="weather-condition-text">Syncing Weather...</div>
                        <div style="font-size: 12px; color: var(--text-secondary);" id="weather-advisory-text">Fetching live meteorological rainfall forecast...</div>
                    </div>
                </div>
            </div>

            <!-- Live Chart Card -->
            <div class="card">
                <div class="card-title">
                    <div class="card-title-left">
                        <i class="fa-solid fa-chart-line"></i>
                        <span>Real-time Water Volume Prediction Curve</span>
                    </div>
                </div>
                <div class="chart-container">
                    <canvas id="telemetryChart"></canvas>
                </div>
            </div>

        </div>
    </div>

    <!-- Farmer Setup Modal -->
    <div id="profile-modal" class="modal-overlay" style="display: none;">
        <div class="modal-card">
            <div class="modal-header">
                <div style="display: flex; align-items: center; gap: 12px;">
                    <div class="brand-icon" style="width: 40px; height: 40px; font-size: 18px; border-radius: 10px;">
                        <i class="fa-solid fa-tractor"></i>
                    </div>
                    <div>
                        <h2 style="font-family: 'Outfit', sans-serif; font-size: 20px; font-weight: 700; color: #fff;">Farmer & Farm Configuration</h2>
                        <p style="font-size: 12px; color: var(--text-secondary);">Personalized precision irrigation profile & crop specifications</p>
                    </div>
                </div>
                <button class="modal-close-btn" onclick="closeProfileModal()"><i class="fa-solid fa-xmark"></i></button>
            </div>
            <div class="modal-body">
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 14px;">
                    <div>
                        <label class="modal-label">Farmer Full Name</label>
                        <input type="text" id="modal-farmer-name" placeholder="e.g. Sujal Chavan" />
                    </div>
                    <div>
                        <label class="modal-label">Farm / Plot Name</label>
                        <input type="text" id="modal-farm-name" placeholder="e.g. Plot Alpha - River Field" />
                    </div>
                    <div style="grid-column: span 2;">
                        <label class="modal-label">Geographic Location</label>
                        <input type="text" id="modal-location" placeholder="e.g. Pune, Maharashtra" />
                    </div>
                    <div>
                        <label class="modal-label">Crop Name</label>
                        <select id="modal-crop">
                            <option value="Wheat">Wheat</option>
                            <option value="Cotton">Cotton</option>
                            <option value="Maize">Maize</option>
                            <option value="Rice">Rice</option>
                            <option value="Sugarcane">Sugarcane</option>
                            <option value="Vegetables">Vegetables</option>
                        </select>
                    </div>
                    <div>
                        <label class="modal-label">Growth Stage</label>
                        <select id="modal-growth">
                            <option value="Seedling">Seedling</option>
                            <option value="Vegetative">Vegetative</option>
                            <option value="Flowering">Flowering</option>
                            <option value="Maturity">Maturity</option>
                        </select>
                    </div>
                    <div>
                        <label class="modal-label">Soil Type</label>
                        <select id="modal-soiltype">
                            <option value="Loamy">Loamy</option>
                            <option value="Sandy">Sandy</option>
                            <option value="Clay">Clay</option>
                        </select>
                    </div>
                    <div>
                        <label class="modal-label">Farm Area & Unit</label>
                        <div style="display: flex; gap: 8px;">
                            <input type="number" id="modal-area" step="0.1" min="0.1" value="1.5" style="flex: 1;" />
                            <select id="modal-unit" style="width: 110px;">
                                <option value="Acres">Acres</option>
                                <option value="Hectares">Hectares</option>
                                <option value="Guntha">Guntha</option>
                            </select>
                        </div>
                    </div>
                    <div>
                        <label class="modal-label">Irrigation System</label>
                        <select id="modal-irrig-type">
                            <option value="Drip Irrigation">Drip Irrigation</option>
                            <option value="Sprinkler System">Sprinkler System</option>
                            <option value="Furrow / Surface">Furrow / Surface</option>
                        </select>
                    </div>
                    <div>
                        <label class="modal-label">Pump Flow Rate (LPM)</label>
                        <input type="number" id="modal-flow-rate" step="0.1" min="0.5" value="2.5" />
                    </div>
                </div>
            </div>
            <div class="modal-footer">
                <button class="btn-cancel" onclick="closeProfileModal()">Cancel</button>
                <button class="btn-save-profile" onclick="saveFarmerProfile()">
                    <i class="fa-solid fa-check"></i> Save & Apply Farm Profile
                </button>
            </div>
        </div>
    </div>

    <!-- Hardware & Connectivity Settings Modal -->
    <div id="settings-modal" class="modal-overlay" style="display: none;">
        <div class="modal-card">
            <div class="modal-header">
                <div style="display: flex; align-items: center; gap: 12px;">
                    <div class="brand-icon" style="width: 40px; height: 40px; font-size: 18px; border-radius: 10px; background: linear-gradient(135deg, var(--accent-cyan), var(--accent-blue));">
                        <i class="fa-solid fa-sliders"></i>
                    </div>
                    <div>
                        <h2 style="font-family: 'Outfit', sans-serif; font-size: 20px; font-weight: 700; color: #fff;">Hardware & Connectivity</h2>
                        <p style="font-size: 12px; color: var(--text-secondary);">ESP32 Wi-Fi IP, AI Server link, Relay Polarity & Thresholds</p>
                    </div>
                </div>
                <button class="modal-close-btn" onclick="closeSettingsModal()"><i class="fa-solid fa-xmark"></i></button>
            </div>
            <div class="modal-body">
                <div style="display: flex; flex-direction: column; gap: 16px;">
                    <div>
                        <label class="modal-label">AI Model Backend Host IP & Port</label>
                        <div style="display: flex; gap: 8px;">
                            <input type="text" id="setting-ai-ip" placeholder="192.168.1.3" style="flex: 2;" />
                            <input type="number" id="setting-ai-port" placeholder="8000" style="flex: 1;" />
                        </div>
                        <span style="font-size: 11px; color: var(--text-muted); margin-top: 4px; display: block;">Host PC running the Python FastAPI ML inference server.</span>
                    </div>

                    <div>
                        <label class="modal-label">ESP32-S3 Hardware IP Address</label>
                        <input type="text" id="setting-esp32-ip" placeholder="192.168.1.9" />
                        <span style="font-size: 11px; color: var(--text-muted); margin-top: 4px; display: block;">Local Wi-Fi IP assigned to the ESP32-S3 microcontroller.</span>
                    </div>

                    <div>
                        <label class="modal-label">Relay Module Polarity Logic</label>
                        <select id="setting-relay-polarity">
                            <option value="0">Active-HIGH (3.3V HIGH to Arduino / Relay for ON)</option>
                            <option value="1">Active-LOW (0V LOW triggers Relay ON)</option>
                        </select>
                        <span style="font-size: 11px; color: var(--text-muted); margin-top: 4px; display: block;">Configures ESP32 GPIO 11 signal polarity to Arduino Uno or direct Relay.</span>
                    </div>

                    <div>
                        <label class="modal-label">Soil Dryness Threshold (ADC Value)</label>
                        <input type="number" id="setting-soil-threshold" min="500" max="3800" step="50" value="2500" />
                        <span style="font-size: 11px; color: var(--text-muted); margin-top: 4px; display: block;">When Soil ADC > Threshold (2500), soil is dry and system triggers irrigation in Threshold Auto mode.</span>
                    </div>

                    <div style="display: flex; align-items: center; justify-content: space-between; padding: 12px; background: rgba(255,255,255,0.03); border-radius: 12px; border: 1px solid var(--card-border);">
                        <div>
                            <div style="font-size: 13px; font-weight: 600; color: #fff;">Web Audio Rain Alarm</div>
                            <div style="font-size: 11px; color: var(--text-secondary);">Plays audible browser beep alert when rain sensor triggers</div>
                        </div>
                        <button id="btn-audio-toggle" onclick="toggleAudioAlerts()" style="background: rgba(255,255,255,0.08); border: 1px solid var(--card-border); color: #fff; padding: 6px 14px; border-radius: 8px; font-size: 12px; cursor: pointer;">Disabled</button>
                    </div>
                </div>
            </div>
            <div class="modal-footer">
                <button class="btn-cancel" onclick="closeSettingsModal()">Cancel</button>
                <button class="btn-save-profile" onclick="saveHardwareSettings()">
                    <i class="fa-solid fa-floppy-disk"></i> Apply Settings
                </button>
            </div>
        </div>
    </div>

    <script>
        // Global State & Endpoints
        let isRain = "NO";
        let isSoil = "DRY";
        let isTank = "AVAILABLE";
        let manualPumpState = false;
        let currentMode = "AI_AUTO";
        let isUserDraggingSlider = false;
        let currentThreshold = 2500;

        // Dynamic Dual-Origin API Base detection
        // If served from FastAPI (port 8000), aiBase is relative ("")
        // If served from ESP32 (port 80), aiBase points to http://${aiIp}:8000
        let aiServerIp = localStorage.getItem('ai_server_ip') || (window.location.hostname !== 'localhost' && window.location.hostname !== '127.0.0.1' ? '192.168.1.3' : 'localhost');
        let aiServerPort = localStorage.getItem('ai_server_port') || '8000';
        let esp32Ip = localStorage.getItem('esp32_ip') || (window.location.port !== '8000' && window.location.hostname ? window.location.hostname : '192.168.1.9');

        function getAiBase() {
            if (window.location.port === "8000") return "";
            return `http://${aiServerIp}:${aiServerPort}`;
        }

        function getEsp32Base() {
            if (window.location.port !== "8000" && window.location.hostname) return "";
            return `http://${esp32Ip}`;
        }

        // Web Audio Rain Alert Engine
        let audioCtx = null;
        let alarmInterval = null;
        let audioAlertsEnabled = localStorage.getItem('audio_alerts_enabled') === 'true';
        let alarmSilenced = false;

        function toggleAudioAlerts() {
            audioAlertsEnabled = !audioAlertsEnabled;
            localStorage.setItem('audio_alerts_enabled', audioAlertsEnabled ? 'true' : 'false');
            updateAudioButton();
            if (audioAlertsEnabled && !audioCtx) {
                audioCtx = new (window.AudioContext || window.webkitAudioContext)();
                playSingleBeep();
            }
        }

        function updateAudioButton() {
            const btn = document.getElementById('btn-audio-toggle');
            if (btn) {
                btn.innerText = audioAlertsEnabled ? "🔔 Enabled" : "Disabled";
                btn.style.background = audioAlertsEnabled ? "rgba(16, 185, 129, 0.2)" : "rgba(255,255,255,0.08)";
                btn.style.color = audioAlertsEnabled ? "var(--accent-emerald)" : "#fff";
                btn.style.borderColor = audioAlertsEnabled ? "var(--accent-emerald)" : "var(--card-border)";
            }
        }

        function playSingleBeep() {
            try {
                if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
                if (audioCtx.state === 'suspended') audioCtx.resume();
                const osc = audioCtx.createOscillator();
                const gain = audioCtx.createGain();
                osc.type = 'sine';
                osc.frequency.value = 880;
                gain.gain.value = 0.2;
                osc.connect(gain);
                gain.connect(audioCtx.destination);
                osc.start();
                setTimeout(() => {
                    try { osc.stop(); } catch(e){}
                }, 200);
            } catch (e) {
                console.warn("AudioContext play error:", e);
            }
        }

        function triggerRainAlarm() {
            if (!audioAlertsEnabled || alarmSilenced) return;
            const stopBtn = document.getElementById('stop-alarm-btn');
            if (stopBtn) stopBtn.style.display = 'flex';
            if (!alarmInterval) {
                playSingleBeep();
                alarmInterval = setInterval(playSingleBeep, 800);
            }
        }

        function stopRainAlarm() {
            alarmSilenced = true;
            if (alarmInterval) {
                clearInterval(alarmInterval);
                alarmInterval = null;
            }
            const stopBtn = document.getElementById('stop-alarm-btn');
            if (stopBtn) stopBtn.style.display = 'none';
        }

        // Toggles
        function toggleRain() {
            isRain = (isRain === "NO") ? "YES" : "NO";
            updateToggleButtons();
            syncFarmSettings();
        }

        function toggleSoil() {
            isSoil = (isSoil === "DRY") ? "WET" : "DRY";
            updateToggleButtons();
            syncFarmSettings();
        }

        function toggleTank() {
            isTank = (isTank === "AVAILABLE") ? "EMPTY" : "AVAILABLE";
            updateToggleButtons();
            syncFarmSettings();
        }

        async function toggleManualPump() {
            manualPumpState = !manualPumpState;
            currentMode = "MANUAL";
            updateModeUI("MANUAL");
            updateToggleButtons();
            
            // Dual sync: update AI server AND direct ESP32 for immediate hardware reaction
            try {
                fetch(`${getAiBase()}/api/control`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ mode: "MANUAL", manual_pump_state: manualPumpState })
                }).catch(()=>{});
            } catch (e) {}

            try {
                fetch(`${getEsp32Base()}/relay?state=${manualPumpState ? 1 : 0}`).catch(()=>{});
            } catch (e) {}

            setTimeout(pollTelemetry, 300);
        }

        function updateToggleButtons() {
            const bRain = document.getElementById("btn-rain");
            if (bRain) {
                bRain.innerText = "Rain: " + isRain;
                bRain.className = "toggle-btn " + (isRain === "YES" ? "active-green" : "active-red");
            }

            const bSoil = document.getElementById("btn-soil");
            if (bSoil) {
                bSoil.innerText = "Soil: " + isSoil;
                bSoil.className = "toggle-btn " + (isSoil === "WET" ? "active-green" : "active-red");
            }

            const bTank = document.getElementById("btn-tank");
            if (bTank) {
                bTank.innerText = "Tank: " + isTank;
                bTank.className = "toggle-btn " + (isTank === "AVAILABLE" ? "active-green" : "active-red");
            }

            const bPump = document.getElementById("btn-pump-manual");
            if (bPump) {
                bPump.innerText = "Pump: " + (manualPumpState ? "ON" : "OFF");
                bPump.className = "toggle-btn " + (manualPumpState ? "active-green" : "active-red");
            }
        }

        function updateModeUI(mode) {
            currentMode = mode;
            const bAi = document.getElementById("btn-mode-ai");
            const bThresh = document.getElementById("btn-mode-thresh");
            const bMan = document.getElementById("btn-mode-manual");
            const pill = document.getElementById("active-mode-pill");
            const threshSec = document.getElementById("threshold-control-section");

            if (bAi) bAi.className = "mode-btn " + (mode === "AI_AUTO" ? "active" : "");
            if (bThresh) bThresh.className = "mode-btn " + (mode === "THRESHOLD_AUTO" ? "active" : "");
            if (bMan) bMan.className = "mode-btn " + (mode === "MANUAL" ? "active" : "");
            if (pill) pill.innerText = "Mode: " + mode;
            if (threshSec) threshSec.style.display = (mode === "THRESHOLD_AUTO") ? "block" : "none";
        }

        async function setSystemMode(mode) {
            updateModeUI(mode);

            // Notify AI Server
            try {
                fetch(`${getAiBase()}/api/control`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ mode: mode })
                }).catch(()=>{});
            } catch (e) {}

            // Notify ESP32 directly
            try {
                fetch(`${getEsp32Base()}/setmode?mode=${mode}`).catch(()=>{});
            } catch (e) {}

            setTimeout(pollTelemetry, 300);
        }

        function onThresholdChange(val) {
            currentThreshold = parseInt(val);
            document.getElementById('val-thresh-display').innerText = val;
        }

        async function saveThresholdSetting() {
            try {
                fetch(`${getAiBase()}/api/control`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ soil_threshold: currentThreshold })
                }).catch(()=>{});

                fetch(`${getEsp32Base()}/threshold?value=${currentThreshold}`).catch(()=>{});
            } catch(e){}
        }

        async function syncFarmSettings() {
            const crop = document.getElementById('select-crop').value;
            const season = document.getElementById('select-season').value;
            const growth = document.getElementById('select-growth').value;
            const soiltype = document.getElementById('select-soiltype').value;

            try {
                fetch(`${getAiBase()}/api/control`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        crop: crop,
                        season: season,
                        growth_stage: growth,
                        soil_type: soiltype
                    })
                }).catch(()=>{});

                fetch(`${getEsp32Base()}/setcrop?crop=${encodeURIComponent(crop)}&season=${encodeURIComponent(season)}`).catch(()=>{});
            } catch(e){}

            triggerManualInference();
        }

        // Initialize Chart
        const ctx = document.getElementById('telemetryChart').getContext('2d');
        const telemetryChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: ['T-4', 'T-3', 'T-2', 'T-1', 'Current'],
                datasets: [{
                    label: 'Predicted Water Required (L)',
                    data: [0, 0, 0, 0, 0],
                    borderColor: '#10b981',
                    backgroundColor: 'rgba(16, 185, 129, 0.1)',
                    borderWidth: 3,
                    fill: true,
                    tension: 0.4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: { legend: { display: false } },
                scales: {
                    x: { grid: { color: 'rgba(255, 255, 255, 0.05)' } },
                    y: { grid: { color: 'rgba(255, 255, 255, 0.05)' }, beginAtZero: true }
                }
            }
        });

        function manualSliderChange() {
            document.getElementById('val-temp').innerText = document.getElementById('input-temp').value + ' °C';
            document.getElementById('val-hum').innerText = document.getElementById('input-hum').value + ' %';
            document.getElementById('val-soil').innerText = document.getElementById('input-soil').value;
            document.getElementById('val-light').innerText = document.getElementById('input-light').value;
            document.getElementById('val-water').innerText = document.getElementById('input-water').value;
        }

        async function triggerManualInference() {
            const temp = parseFloat(document.getElementById('input-temp').value);
            const hum = parseFloat(document.getElementById('input-hum').value);
            const soil = parseInt(document.getElementById('input-soil').value);
            const light = parseInt(document.getElementById('input-light').value);
            const water = parseInt(document.getElementById('input-water').value);

            const payload = {
                temperature: temp,
                humidity: hum,
                soil_value: soil,
                light_value: light,
                water_level: water,
                rain: isRain,
                soil_status: isSoil,
                tank_status: isTank,
                crop: document.getElementById('select-crop').value,
                season: document.getElementById('select-season').value,
                growth_stage: document.getElementById('select-growth').value,
                soil_type: document.getElementById('select-soiltype').value,
                pump_flow_lpm: 2.5
            };

            try {
                const res = await fetch(`${getAiBase()}/predict`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                const data = await res.json();
                updateDisplayWithPrediction(data, data, true);
            } catch (err) {
                console.warn("Manual inference fallback:", err);
            }
        }

        function updateDisplayWithPrediction(pred, ctrl, isOnline) {
            ctrl = ctrl || {};
            if (isOnline === undefined) isOnline = true;
            const actionBanner = document.getElementById('action-banner');
            const actionText = document.getElementById('action-text');
            const actionIcon = document.getElementById('action-icon');

            const desired = isOnline ? (ctrl.desired_pump_state !== undefined ? ctrl.desired_pump_state : (pred.desired_pump_state !== undefined ? pred.desired_pump_state : (pred.action === 'PUMP_ON'))) : false;
            const target = isOnline ? (ctrl.target_pump_state !== undefined ? ctrl.target_pump_state : (pred.target_pump_state !== undefined ? pred.target_pump_state : (pred.action === 'PUMP_ON'))) : false;
            const actual = isOnline ? (ctrl.actual_pump_state !== undefined ? ctrl.actual_pump_state : (pred.actual_pump_state !== undefined ? pred.actual_pump_state : false)) : false;
            const activeMode = ctrl.mode || pred.mode || currentMode || "AI_AUTO";
            const reason = isOnline ? (pred.status_reason || ctrl.pump_reason || "Precision moisture criteria met") : "ESP32 Hardware Offline: Irrigation Disengaged (Failsafe Active)";

            // Update Desired State Pill
            const pillDesired = document.getElementById('pill-desired-state');
            const valDesired = document.getElementById('val-desired-state');
            if (pillDesired && valDesired) {
                if (!isOnline) {
                    valDesired.innerText = 'OFFLINE';
                    pillDesired.style.borderColor = 'rgba(255, 255, 255, 0.1)';
                    pillDesired.style.background = 'rgba(255, 255, 255, 0.04)';
                    valDesired.style.color = '#94a3b8';
                } else {
                    valDesired.innerText = desired ? 'PUMP_ON' : 'PUMP_OFF';
                    if (desired) {
                        pillDesired.style.borderColor = 'rgba(16, 185, 129, 0.4)';
                        pillDesired.style.background = 'rgba(16, 185, 129, 0.12)';
                        valDesired.style.color = '#10b981';
                    } else {
                        pillDesired.style.borderColor = 'rgba(255, 255, 255, 0.1)';
                        pillDesired.style.background = 'rgba(255, 255, 255, 0.04)';
                        valDesired.style.color = '#94a3b8';
                    }
                }
            }

            // Update Target Command Pill
            const pillTarget = document.getElementById('pill-target-state');
            const valTarget = document.getElementById('val-target-state');
            if (pillTarget && valTarget) {
                if (!isOnline) {
                    valTarget.innerText = 'OFFLINE';
                    pillTarget.style.borderColor = 'rgba(255, 255, 255, 0.1)';
                    pillTarget.style.background = 'rgba(255, 255, 255, 0.04)';
                    valTarget.style.color = '#94a3b8';
                } else {
                    valTarget.innerText = target ? 'PUMP_ON' : 'PUMP_OFF';
                    if (target) {
                        pillTarget.style.borderColor = 'rgba(56, 189, 248, 0.4)';
                        pillTarget.style.background = 'rgba(56, 189, 248, 0.12)';
                        valTarget.style.color = '#38bdf8';
                    } else {
                        pillTarget.style.borderColor = 'rgba(255, 255, 255, 0.1)';
                        pillTarget.style.background = 'rgba(255, 255, 255, 0.04)';
                        valTarget.style.color = '#94a3b8';
                    }
                }
            }

            // Update Actual Hardware Relay Pill
            const pillActual = document.getElementById('pill-actual-state');
            const valActual = document.getElementById('val-actual-state');
            if (pillActual && valActual) {
                if (!isOnline) {
                    valActual.innerText = 'OFFLINE';
                    pillActual.style.borderColor = 'rgba(244, 63, 94, 0.3)';
                    pillActual.style.background = 'rgba(244, 63, 94, 0.1)';
                    pillActual.style.boxShadow = 'none';
                    valActual.style.color = '#f87171';
                } else {
                    valActual.innerText = actual ? 'ACTIVE (HIGH)' : 'INACTIVE (LOW)';
                    if (actual) {
                        pillActual.style.borderColor = '#10b981';
                        pillActual.style.background = 'rgba(16, 185, 129, 0.25)';
                        pillActual.style.boxShadow = '0 0 12px rgba(16, 185, 129, 0.35)';
                        valActual.style.color = '#34d399';
                    } else {
                        pillActual.style.borderColor = 'rgba(255, 255, 255, 0.1)';
                        pillActual.style.background = 'rgba(255, 255, 255, 0.04)';
                        pillActual.style.boxShadow = 'none';
                        valActual.style.color = '#94a3b8';
                    }
                }
            }

            // Update Operating Mode Pill
            const valMode = document.getElementById('val-mode-state');
            if (valMode) valMode.innerText = activeMode;

            // Update Action Banner & Icons
            if (!isOnline) {
                actionBanner.className = 'action-banner banner-off';
                actionBanner.style.borderColor = 'rgba(244, 63, 94, 0.4)';
                actionText.innerText = 'PUMP_OFF (DEVICE OFFLINE)';
                if (actionIcon) actionIcon.className = 'fa-solid fa-triangle-exclamation';
            } else if (actual) {
                actionBanner.className = 'action-banner banner-on';
                actionBanner.style.borderColor = '';
                actionText.innerText = 'PUMP ACTIVE (RUNNING)';
                if (actionIcon) actionIcon.className = 'fa-solid fa-droplet fa-beat';
            } else if (target) {
                actionBanner.className = 'action-banner banner-on';
                actionBanner.style.borderColor = '';
                actionText.innerText = 'PUMP_ON (STARTING)';
                if (actionIcon) actionIcon.className = 'fa-solid fa-power-off';
            } else if (desired && !target) {
                actionBanner.className = 'action-banner banner-off';
                actionBanner.style.borderColor = 'var(--accent-rose)';
                actionText.innerText = 'PUMP_LOCKED (SAFETY)';
                if (actionIcon) actionIcon.className = 'fa-solid fa-shield-halved';
            } else {
                actionBanner.className = 'action-banner banner-off';
                actionBanner.style.borderColor = '';
                actionText.innerText = 'PUMP_OFF';
                if (actionIcon) actionIcon.className = 'fa-solid fa-power-off';
            }

            document.getElementById('status-reason').innerText = 'Reason: ' + reason;
            document.getElementById('res-water').innerText = (pred.predicted_water_required_l !== undefined ? pred.predicted_water_required_l : 0.0) + ' L';
            const farmArea = pred.farm_area || 1.5;
            const areaUnit = pred.farm_area_unit || 'Acres';
            const farmWater = pred.farm_total_water_l !== undefined ? pred.farm_total_water_l : ((pred.predicted_water_required_l || 0.0) * farmArea).toFixed(2);
            document.getElementById('res-farm-water').innerText = farmWater + ' L (' + farmArea + ' ' + areaUnit + ')';
            document.getElementById('res-flow').innerText = (pred.pump_flow_lpm || 2.5) + ' L/m';
            document.getElementById('res-runtime').innerText = (pred.pump_runtime_sec || 0) + ' s (' + (pred.pump_runtime_min || 0) + ' m)';

            if (pred.predicted_water_required_l !== undefined) {
                const chartData = telemetryChart.data.datasets[0].data;
                chartData.shift();
                chartData.push(pred.predicted_water_required_l);
                telemetryChart.update();
            }
        }

        // Main Unified Telemetry Poller (Dual Origin)
        async function pollTelemetry() {
            let aiConnected = false;
            let espConnected = false;

            // 1. Try polling AI Model Server
            try {
                const aiRes = await fetch(`${getAiBase()}/api/telemetry`, { signal: AbortSignal.timeout(1800) });
                if (aiRes.ok) {
                    const state = await aiRes.json();
                    aiConnected = true;

                    // Header Badges
                    const espBadge = document.getElementById('esp32-status-badge');
                    const espText = document.getElementById('esp32-status-text');
                    const aiBadge = document.getElementById('ai-status-badge');
                    const aiText = document.getElementById('ai-status-text');

                    if (aiBadge) {
                        aiBadge.className = 'status-badge';
                        if (aiText) aiText.innerText = `AI Server Online (${aiServerIp}:${aiServerPort})`;
                    }

                    if (state.device_online) {
                        espConnected = true;
                        if (espBadge) espBadge.className = 'status-badge';
                        if (espText) espText.innerText = `ESP32 Online (${esp32Ip})`;
                    } else {
                        if (espBadge) espBadge.className = 'status-badge offline';
                        if (espText) espText.innerText = 'ESP32 Offline (Waiting for Sync)';
                    }

                    if (state.last_seen_timestamp) {
                        document.getElementById('last-sync-time').innerText = 'Last sync: ' + new Date(state.last_seen_timestamp).toLocaleTimeString();
                    }

                    // Profile subtitle
                    if (state.profile) {
                        const p = state.profile;
                        if (p.location) {
                            const locPill = document.getElementById('weather-location-pill');
                            if (locPill) locPill.innerText = p.location;
                        }
                        if (p.farmer_name && p.farm_name) {
                            const sub = document.getElementById('dashboard-subtitle');
                            if (sub) sub.innerText = `${p.farmer_name} • ${p.farm_name} (${p.farm_area || 1.5} ${p.area_unit || 'Acres'})`;
                        }
                    }

                    // Weather forecast
                    if (state.weather) {
                        const w = state.weather;
                        const wTemp = document.getElementById('weather-temp');
                        const wProb = document.getElementById('weather-rain-prob');
                        const wMm = document.getElementById('weather-rain-mm');
                        const wCond = document.getElementById('weather-condition-text');
                        const wAdv = document.getElementById('weather-advisory-text');

                        if (wTemp) wTemp.innerText = (w.current_temp !== undefined ? w.current_temp : '--') + ' °C';
                        if (wProb) wProb.innerText = (w.rain_probability_next_6h !== undefined ? w.rain_probability_next_6h : 0) + ' %';
                        if (wMm) wMm.innerText = (w.expected_rain_mm_next_6h !== undefined ? w.expected_rain_mm_next_6h : 0) + ' mm';
                        if (wCond) wCond.innerText = w.weather_condition || 'Clear / Dry';
                        if (wAdv) wAdv.innerText = w.advisory || 'Scheduled AI predictive irrigation active.';
                    }

                    // Mode
                    if (state.control && state.control.mode) {
                        updateModeUI(state.control.mode);
                    }

                    // Alerts Banner
                    const alertsBanner = document.getElementById('alerts-banner');
                    if (state.alerts && state.alerts.length > 0) {
                        alertsBanner.style.display = 'flex';
                        alertsBanner.innerHTML = state.alerts.map(a => {
                            let cls = 'alert-pill';
                            if (a.includes('Active') || a.includes('Delivering')) cls += ' info';
                            else if (a.includes('Healthy') || a.includes('Adequate')) cls += ' success';
                            return `<div class="${cls}"><i class="fa-solid fa-triangle-exclamation"></i> <span>${a}</span></div>`;
                        }).join('');
                    } else {
                        alertsBanner.style.display = 'none';
                        alertsBanner.innerHTML = '';
                    }

                    // Telemetry Sliders
                    if (state.telemetry) {
                        const tel = state.telemetry;
                        if (!isUserDraggingSlider) {
                            document.getElementById('input-temp').value = tel.temperature;
                            document.getElementById('input-hum').value = tel.humidity;
                            document.getElementById('input-soil').value = tel.soil_value;
                            document.getElementById('input-light').value = tel.light_value;
                            document.getElementById('input-water').value = tel.water_level;
                        }
                        manualSliderChange();

                        const waterCtrl = document.getElementById('sensor-control-water');
                        if (waterCtrl) {
                            if (tel.water_level < 500 || tel.tank_status === 'EMPTY') {
                                waterCtrl.classList.add('tank-empty-glow');
                            } else {
                                waterCtrl.classList.remove('tank-empty-glow');
                            }
                        }

                        isRain = tel.rain || "NO";
                        isSoil = tel.soil_status || "DRY";
                        isTank = tel.tank_status || "AVAILABLE";
                        updateToggleButtons();

                        if (isRain === "YES") triggerRainAlarm(); else { alarmSilenced = false; stopRainAlarm(); }

                        if (tel.crop) document.getElementById('select-crop').value = tel.crop;
                        if (tel.season) document.getElementById('select-season').value = tel.season;
                        if (tel.growth_stage) document.getElementById('select-growth').value = tel.growth_stage;
                        if (tel.soil_type) document.getElementById('select-soiltype').value = tel.soil_type;
                    }

                    // Control / Relay Toggle Sync
                    if (state.control) {
                        manualPumpState = state.control.manual_pump_state || false;
                        const bPump = document.getElementById("btn-pump-manual");
                        if (currentMode === "MANUAL") {
                            bPump.innerText = "Manual: " + (manualPumpState ? "ON" : "OFF");
                            bPump.className = "toggle-btn " + (manualPumpState ? "active-green" : "active-red");
                        } else {
                            const actualRelay = state.control.actual_pump_state || false;
                            const targetRelay = state.control.target_pump_state || false;
                            bPump.innerText = "Relay: " + (actualRelay ? "ACTIVE ON" : (targetRelay ? "COMMAND ON" : "OFF"));
                            bPump.className = "toggle-btn " + (actualRelay || targetRelay ? "active-green" : "active-red");
                        }
                    }

                    // Update AI Prediction
                    if (state.prediction) {
                        updateDisplayWithPrediction(state.prediction, state.control, state.device_online);
                    }
                }
            } catch (err) {
                // AI server unreachable
                aiConnected = false;
                const aiBadge = document.getElementById('ai-status-badge');
                const aiText = document.getElementById('ai-status-text');
                if (aiBadge) {
                    aiBadge.className = 'status-badge offline';
                    if (aiText) aiText.innerText = `AI Server Offline (${aiServerIp}:${aiServerPort})`;
                }
            }

            // 2. Direct Hardware Fallback / Verification via ESP32 /data
            try {
                const espRes = await fetch(`${getEsp32Base()}/data`, { signal: AbortSignal.timeout(1500) });
                if (espRes.ok) {
                    const d = await espRes.json();
                    espConnected = true;

                    const espBadge = document.getElementById('esp32-status-badge');
                    const espText = document.getElementById('esp32-status-text');
                    if (espBadge) {
                        espBadge.className = 'status-badge';
                        if (espText) espText.innerText = `ESP32 Online (${window.location.hostname || esp32Ip})`;
                    }

                    // If AI server is disconnected, update UI directly from ESP32 telemetry!
                    if (!aiConnected) {
                        if (!isUserDraggingSlider) {
                            document.getElementById('input-temp').value = d.temperature;
                            document.getElementById('input-hum').value = d.humidity;
                            document.getElementById('input-soil').value = d.soil;
                            document.getElementById('input-light').value = d.light;
                            document.getElementById('input-water').value = d.water;
                        }
                        manualSliderChange();

                        isRain = d.rain ? "YES" : "NO";
                        isSoil = (d.soil > (d.threshold || 2500)) ? "DRY" : "WET";
                        isTank = (d.water < 500) ? "EMPTY" : "AVAILABLE";
                        updateToggleButtons();

                        if (isRain === "YES") triggerRainAlarm(); else { alarmSilenced = false; stopRainAlarm(); }

                        updateModeUI(d.mode || "AI_AUTO");

                        const alertsBanner = document.getElementById('alerts-banner');
                        alertsBanner.style.display = 'flex';
                        let alertHtml = `<div class="alert-pill info"><i class="fa-solid fa-microchip"></i> <span>Direct ESP32-S3 Hardware Link Active • Standalone Failsafe Mode</span></div>`;
                        if (d.water < 500) {
                            alertHtml += `<div class="alert-pill"><i class="fa-solid fa-triangle-exclamation"></i> <span>Tank Empty Lockout (< 500) - Hardware Pump Interlocked OFF</span></div>`;
                        }
                        if (d.rain) {
                            alertHtml += `<div class="alert-pill"><i class="fa-solid fa-cloud-showers-heavy"></i> <span>Rain Sensor Override Active - Irrigation Suppressed</span></div>`;
                        }
                        alertsBanner.innerHTML = alertHtml;

                        const mockPred = {
                            action: d.ai_action || (d.relay ? "PUMP_ON" : "PUMP_OFF"),
                            status_reason: d.ai_reason || "Operating via ESP32 firmware safety rules",
                            predicted_water_required_l: d.ai_water || 0.0,
                            pump_runtime_sec: d.ai_runtime || 0.0,
                            pump_flow_lpm: 2.5
                        };
                        const mockCtrl = {
                            mode: d.mode,
                            actual_pump_state: d.relay,
                            target_pump_state: d.relay,
                            desired_pump_state: d.relay
                        };
                        updateDisplayWithPrediction(mockPred, mockCtrl, true);
                    }

                    // Auto-sync AI IP if reported by ESP32
                    if (d.ai_ip && d.ai_ip !== aiServerIp && d.ai_ip !== "192.168.1.100") {
                        aiServerIp = d.ai_ip;
                    }
                }
            } catch (err) {
                if (!aiConnected) {
                    const espBadge = document.getElementById('esp32-status-badge');
                    const espText = document.getElementById('esp32-status-text');
                    if (espBadge) {
                        espBadge.className = 'status-badge offline';
                        if (espText) espText.innerText = 'ESP32 Connection Offline';
                    }
                }
            }
        }

        // Farmer Profile Modal
        async function openProfileModal() {
            try {
                const res = await fetch(`${getAiBase()}/api/profile`);
                const p = await res.json();
                if (p) {
                    if (p.farmer_name) document.getElementById('modal-farmer-name').value = p.farmer_name;
                    if (p.farm_name) document.getElementById('modal-farm-name').value = p.farm_name;
                    if (p.location) document.getElementById('modal-location').value = p.location;
                    if (p.crop) document.getElementById('modal-crop').value = p.crop;
                    if (p.growth_stage) document.getElementById('modal-growth').value = p.growth_stage;
                    if (p.soil_type) document.getElementById('modal-soiltype').value = p.soil_type;
                    if (p.farm_area) document.getElementById('modal-area').value = p.farm_area;
                    if (p.area_unit) document.getElementById('modal-unit').value = p.area_unit;
                    if (p.irrigation_type) document.getElementById('modal-irrig-type').value = p.irrigation_type;
                    if (p.pump_flow_lpm) document.getElementById('modal-flow-rate').value = p.pump_flow_lpm;
                }
            } catch (e) {
                console.warn("Failed to load remote profile:", e);
            }
            document.getElementById('profile-modal').style.display = 'flex';
        }

        function closeProfileModal() {
            document.getElementById('profile-modal').style.display = 'none';
        }

        async function saveFarmerProfile() {
            const profileData = {
                farmer_name: document.getElementById('modal-farmer-name').value || "Sujal Chavan",
                farm_name: document.getElementById('modal-farm-name').value || "Smart Agriculture Plot #1",
                location: document.getElementById('modal-location').value || "Pune, Maharashtra",
                crop: document.getElementById('modal-crop').value,
                growth_stage: document.getElementById('modal-growth').value,
                soil_type: document.getElementById('modal-soiltype').value,
                farm_area: parseFloat(document.getElementById('modal-area').value) || 1.5,
                area_unit: document.getElementById('modal-unit').value || "Acres",
                irrigation_type: document.getElementById('modal-irrig-type').value,
                daily_water_target_l: 50.0,
                pump_flow_lpm: parseFloat(document.getElementById('modal-flow-rate').value) || 2.5
            };

            try {
                await fetch(`${getAiBase()}/api/profile`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(profileData)
                });
                localStorage.setItem('farmer_profile_saved', 'true');
                closeProfileModal();
                pollTelemetry();
            } catch (err) {
                alert("Profile saved locally (AI Server not reached): " + err.message);
                closeProfileModal();
            }
        }

        // Hardware & Network Settings Modal
        function openSettingsModal() {
            document.getElementById('setting-ai-ip').value = aiServerIp;
            document.getElementById('setting-ai-port').value = aiServerPort;
            document.getElementById('setting-esp32-ip').value = esp32Ip;
            document.getElementById('setting-soil-threshold').value = currentThreshold;
            updateAudioButton();
            document.getElementById('settings-modal').style.display = 'flex';
        }

        function closeSettingsModal() {
            document.getElementById('settings-modal').style.display = 'none';
        }

        async function saveHardwareSettings() {
            aiServerIp = document.getElementById('setting-ai-ip').value.trim() || '192.168.1.3';
            aiServerPort = document.getElementById('setting-ai-port').value.trim() || '8000';
            esp32Ip = document.getElementById('setting-esp32-ip').value.trim() || '192.168.1.9';
            currentThreshold = parseInt(document.getElementById('setting-soil-threshold').value) || 2500;
            const polVal = document.getElementById('setting-relay-polarity').value;

            localStorage.setItem('ai_server_ip', aiServerIp);
            localStorage.setItem('ai_server_port', aiServerPort);
            localStorage.setItem('esp32_ip', esp32Ip);

            // Send new IP and threshold to ESP32
            try {
                await fetch(`${getEsp32Base()}/setip?ip=${encodeURIComponent(aiServerIp)}`).catch(()=>{});
                await fetch(`${getEsp32Base()}/threshold?value=${currentThreshold}`).catch(()=>{});
                await fetch(`${getEsp32Base()}/polarity?active_low=${polVal}`).catch(()=>{});
            } catch(e) {}

            closeSettingsModal();
            pollTelemetry();
        }

        function checkFirstLaunch() {
            if (!localStorage.getItem('farmer_profile_saved')) {
                openProfileModal();
            }
        }

        // Periodic Polling
        setInterval(pollTelemetry, 1500);
        window.onload = () => {
            updateAudioButton();
            checkFirstLaunch();
            pollTelemetry();
        };
    </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
