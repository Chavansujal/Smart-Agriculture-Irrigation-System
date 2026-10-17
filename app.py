import os
import json
import time
import requests
from datetime import datetime
from typing import Optional, List
from fastapi import FastAPI, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from pydantic import BaseModel, Field
from predict import SmartIrrigationPredictor

app = FastAPI(
    title="Smart AIoT Irrigation API",
    description="Backend API syncing ESP32 Hardware, AI Inference Engine, Farmer Profile, and Web Dashboard",
    version="2.1.0"
)

# Enable CORS for local network and web dashboards
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Initialize predictor singleton
try:
    predictor = SmartIrrigationPredictor(models_dir='models')
except Exception as e:
    predictor = None
    print(f"Warning: Predictor failed to load models: {e}")

# ---------- Pydantic Data Models ----------
class FarmerProfile(BaseModel):
    farmer_name: str = "Sujal Chavan"
    farm_name: str = "Smart Agriculture Plot #1"
    location: str = "Pune, Maharashtra"
    latitude: float = 18.5204
    longitude: float = 73.8567
    crop: str = "Wheat"
    growth_stage: str = "Vegetative"
    soil_type: str = "Loamy"
    farm_area: float = 1.5
    area_unit: str = "Acres"
    irrigation_type: str = "Drip Irrigation"
    daily_water_target_l: float = 50.0
    pump_flow_lpm: float = 2.5

class TelemetryInput(BaseModel):
    temperature: Optional[float] = 25.0
    humidity: Optional[float] = 50.0
    soil_value: Optional[int] = 1000
    light_value: Optional[int] = 1000
    water_level: Optional[int] = 2000
    rain: Optional[str] = "NO"
    soil_status: Optional[str] = "DRY"
    tank_status: Optional[str] = "AVAILABLE"
    crop: Optional[str] = "Wheat"
    season: Optional[str] = "Winter"
    growth_stage: Optional[str] = "Vegetative"
    soil_type: Optional[str] = "Loamy"
    pump_flow_lpm: Optional[float] = 2.5
    actual_pump_state: Optional[bool] = False
    current_mode: Optional[str] = None

class ControlInput(BaseModel):
    mode: Optional[str] = None
    manual_pump_state: Optional[bool] = None
    soil_threshold: Optional[int] = None
    crop: Optional[str] = None
    season: Optional[str] = None
    growth_stage: Optional[str] = None
    soil_type: Optional[str] = None

class PredictionResponse(BaseModel):
    status: str
    timestamp: str
    predicted_water_required_l: float
    farm_total_water_l: Optional[float] = 0.0
    farm_area: Optional[float] = 1.5
    farm_area_unit: Optional[str] = "Acres"
    irrigation_required: bool
    action: str
    pump_flow_lpm: float
    pump_runtime_min: float
    pump_runtime_sec: float
    status_reason: str

# ---------- Farmer Profile Persistence ----------
PROFILE_FILE = "farmer_profile.json"

def load_farmer_profile() -> dict:
    if os.path.exists(PROFILE_FILE):
        try:
            with open(PROFILE_FILE, "r") as f:
                return json.load(f)
        except Exception as e:
            print(f"Notice: loading farmer profile: {e}")
    default_p = {
        "farmer_name": "Sujal Chavan",
        "farm_name": "Smart Agriculture Plot #1",
        "location": "Pune, Maharashtra",
        "latitude": 18.5204,
        "longitude": 73.8567,
        "crop": "Wheat",
        "growth_stage": "Vegetative",
        "soil_type": "Loamy",
        "farm_area": 1.5,
        "area_unit": "Acres",
        "irrigation_type": "Drip Irrigation",
        "daily_water_target_l": 50.0,
        "pump_flow_lpm": 2.5
    }
    try:
        with open(PROFILE_FILE, "w") as f:
            json.dump(default_p, f, indent=2)
    except Exception:
        pass
    return default_p

def save_farmer_profile(data: dict):
    with open(PROFILE_FILE, "w") as f:
        json.dump(data, f, indent=2)

initial_profile = load_farmer_profile()

# ---------- Live State Store for ESP32 Hardware & Web Dashboard Synchronization ----------
live_state = {
    "device_online": False,
    "last_seen_timestamp": None,
    "profile": initial_profile,
    "weather": {
        "current_temp": 28.0,
        "current_precipitation": 0.0,
        "rain_probability_next_6h": 0,
        "expected_rain_mm_next_6h": 0.0,
        "weather_condition": "Clear / Mild",
        "advisory": "No rain expected. Scheduled AI irrigation active."
    },
    "telemetry": {
        "temperature": 26.5,
        "humidity": 45.0,
        "soil_value": 3100,
        "light_value": 1200,
        "water_level": 1800,
        "rain": "NO",
        "soil_status": "DRY",
        "tank_status": "AVAILABLE",
        "crop": initial_profile.get("crop", "Wheat"),
        "season": "Winter",
        "growth_stage": initial_profile.get("growth_stage", "Vegetative"),
        "soil_type": initial_profile.get("soil_type", "Loamy"),
        "pump_flow_lpm": initial_profile.get("pump_flow_lpm", 2.5)
    },
    "prediction": {
        "status": "success",
        "timestamp": datetime.now().isoformat(),
        "predicted_water_required_l": 0.0,
        "farm_total_water_l": 0.0,
        "farm_area": initial_profile.get("farm_area", 1.5),
        "farm_area_unit": initial_profile.get("area_unit", "Acres"),
        "irrigation_required": False,
        "action": "PUMP_OFF",
        "pump_flow_lpm": initial_profile.get("pump_flow_lpm", 2.5),
        "pump_runtime_min": 0.0,
        "pump_runtime_sec": 0.0,
        "status_reason": "System Initializing"
    },
    "control": {
        "mode": "AI_AUTO",  # Modes: 'AI_AUTO', 'THRESHOLD_AUTO', 'MANUAL'
        "manual_pump_state": False,
        "soil_threshold": 2500,
        "target_pump_state": False,
        "desired_pump_state": False,
        "actual_pump_state": False,
        "pump_reason": "System Initializing"
    },
    "actual_pump_state": False,
    "alerts": []
}

# ---------- Real-Time Weather Forecast Integration (Open-Meteo) ----------
weather_cache = {
    "last_fetched": 0.0,
    "data": live_state["weather"]
}

def get_weather_forecast(lat: float = 18.5204, lon: float = 73.8567) -> dict:
    now = time.time()
    if now - weather_cache["last_fetched"] < 900:  # 15 minutes cache
        return weather_cache["data"]

    try:
        url = f"https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}&current=temperature_2m,precipitation,rain&hourly=precipitation_probability,precipitation&forecast_days=1"
        res = requests.get(url, timeout=3)
        if res.status_code == 200:
            d = res.json()
            curr = d.get("current", {})
            hourly = d.get("hourly", {})
            probs = hourly.get("precipitation_probability", [0])[:6]
            precips = hourly.get("precipitation", [0])[:6]
            max_prob = max(probs) if probs else 0
            total_rain = sum(precips) if precips else 0.0
            curr_p = float(curr.get("precipitation", 0.0) or 0.0)

            if curr_p > 1.0 or max_prob > 75:
                cond = "Rain Predicted"
                adv = f"High precipitation likelihood ({max_prob}% chance, {total_rain:.1f}mm expected). Irrigation deferred."
            elif max_prob > 40:
                cond = "Overcast / Drizzle"
                adv = f"Moderate rain chance ({max_prob}%). AI irrigation volume reduced by 30%."
            else:
                cond = "Clear / Dry"
                adv = "Clear conditions forecast. Full AI predictive irrigation active."

            w_data = {
                "current_temp": curr.get("temperature_2m", 28.0),
                "current_precipitation": curr_p,
                "rain_probability_next_6h": max_prob,
                "expected_rain_mm_next_6h": round(total_rain, 1),
                "weather_condition": cond,
                "advisory": adv
            }
            weather_cache["last_fetched"] = now
            weather_cache["data"] = w_data
            live_state["weather"] = w_data
            return w_data
    except Exception as e:
        print(f"Weather fetch notice: {e}")

    return weather_cache["data"]

# ---------- Switching Hysteresis & Minimum Switching Delays ----------
pump_switch_tracker = {
    "last_state": False,
    "last_switch_time": 0.0,
    "min_dwell_seconds": 15.0  # Minimum 15s dwell time to prevent relay chattering
}

def sanitize_telemetry(t: dict, threshold: int = 2500) -> dict:
    """Clamps sensor readings to valid physical boundaries and normalizes categories."""
    temp = float(t.get('temperature', 25.0) if t.get('temperature') is not None else 25.0)
    hum = float(t.get('humidity', 50.0) if t.get('humidity') is not None else 50.0)
    soil = int(t.get('soil_value', 1000) if t.get('soil_value') is not None else 1000)
    light = int(t.get('light_value', 1000) if t.get('light_value') is not None else 1000)
    water = int(t.get('water_level', 2000) if t.get('water_level') is not None else 2000)

    # Clamping out-of-range sensor noise
    temp = max(-10.0, min(70.0, temp))
    hum = max(0.0, min(100.0, hum))
    soil = max(0, min(4095, soil))
    light = max(0, min(4095, light))
    water = max(0, min(4095, water))

    rain_raw = str(t.get('rain', 'NO')).upper().strip()
    rain = "YES" if (rain_raw in ["YES", "TRUE", "1", "LOW", "HEAVY"]) else "NO"

    # Derive accurate physical state flags: Above threshold is DRY (Pump ON), Below threshold is WET (Pump OFF)
    tank_status = "EMPTY" if water < 500 else "AVAILABLE"
    soil_status = "DRY" if soil > threshold else "WET"

    crop = str(t.get('crop', live_state["profile"].get("crop", "Wheat"))).strip().capitalize()
    season_raw = str(t.get('season', 'Winter')).strip().capitalize()
    season = "Monsoon" if season_raw in ["Rainy", "Monsoon"] else season_raw
    growth_stage = str(t.get('growth_stage', live_state["profile"].get("growth_stage", "Vegetative"))).strip().capitalize()
    soil_type = str(t.get('soil_type', live_state["profile"].get("soil_type", "Loamy"))).strip().capitalize()
    pump_flow = float(t.get('pump_flow_lpm', live_state["profile"].get("pump_flow_lpm", 2.5)) or 2.5)

    return {
        'temperature': round(temp, 1),
        'humidity': round(hum, 1),
        'soil_value': soil,
        'light_value': light,
        'water_level': water,
        'rain': rain,
        'soil_status': soil_status,
        'tank_status': tank_status,
        'crop': crop,
        'season': season,
        'growth_stage': growth_stage,
        'soil_type': soil_type,
        'pump_flow_lpm': max(0.1, pump_flow),
        'actual_pump_state': bool(t.get('actual_pump_state', False))
    }


def evaluate_pump_target(mode: str, prediction: dict, telemetry: dict, manual_state: bool, threshold: int):
    """
    Evaluates the target physical pump state based on active operational mode, safety rules, and anti-chatter timing.
    Safety Rules Enforced:
    1. Critical Dry-Run Protection: Tank empty or water level < 500 shuts off pump IMMEDIATELY (0s delay) across ALL modes.
    2. Rain Protection: Active rain shuts off pump IMMEDIATELY in automated modes.
    3. Soil Wet Protection: Wet soil shuts off pump in automated modes.
    4. Anti-chatter Dwell Time: 15s delay required before normal state transitions to protect pump relay contacts.
    5. Threshold Hysteresis: ±80 ADC counts around threshold to eliminate fluttering.

    Returns: (target_state: bool, reason: str, desired_state: bool)
    """
    water_val = telemetry.get("water_level", 2000)
    tank = str(telemetry.get("tank_status", "AVAILABLE")).upper()
    rain = str(telemetry.get("rain", "NO")).upper()
    soil_val = telemetry.get("soil_value", 0)
    soil_status = str(telemetry.get("soil_status", "DRY")).upper()

    # Rule 1: CRITICAL DRY-RUN PROTECTION (Never run pump without water - Instant zero-delay trip across ALL modes)
    if tank == "EMPTY" or water_val < 500:
        pump_switch_tracker["last_state"] = False
        pump_switch_tracker["last_switch_time"] = time.time()
        reason = f"Safety Override: Tank Empty ({water_val} ADC < 500). Dry-Run Lock Active."
        return False, reason, False

    # Rule 2: IMMEDIATE RAIN SUSPENSION in automated modes
    if rain == "YES" and mode in ["AI_AUTO", "THRESHOLD_AUTO"]:
        pump_switch_tracker["last_state"] = False
        pump_switch_tracker["last_switch_time"] = time.time()
        reason = "Safety Override: Rain Active (Precipitation Detected). Irrigation Suspended."
        return False, reason, False

    # Rule 3: SOIL SATURATION in automated modes (Sensor: Above threshold = DRY -> Pump ON, Below threshold = WET -> Pump OFF)
    if soil_status == "WET" and mode in ["AI_AUTO", "THRESHOLD_AUTO"]:
        desired = False
        reason = f"Moisture Adequate: Soil Wet ({soil_val} ADC <= {threshold}). Pump Disengaged."
    elif mode == "AI_AUTO":
        desired = (prediction.get("action") == "PUMP_ON")
        reason = prediction.get("status_reason", "AI Automated Evaluation")
    elif mode == "THRESHOLD_AUTO":
        hysteresis = 80
        if pump_switch_tracker["last_state"]:
            # If pump is running, keep running until soil becomes wet (drops below threshold - hysteresis)
            desired = soil_val >= (threshold - hysteresis)
        else:
            # If pump is off, start running when soil dries out (rises above threshold + hysteresis)
            desired = soil_val >= (threshold + hysteresis)
        if desired:
            reason = f"Threshold Auto: Soil Dry ({soil_val} ADC >= {threshold} + {hysteresis}). Pump Active."
        else:
            reason = f"Threshold Auto: Soil Wet ({soil_val} ADC < {threshold} - {hysteresis}). Pump Off."
    elif mode == "MANUAL":
        desired = bool(manual_state)
        reason = "Manual Override: Pump Energized by Farmer" if manual_state else "Manual Override: Pump De-energized by Farmer"
    else:
        desired = False
        reason = "System Idle"

    # In MANUAL mode, de-energizing the pump is an immediate operator action (zero dwell delay)
    now = time.time()
    if mode == "MANUAL" and not desired:
        pump_switch_tracker["last_state"] = False
        pump_switch_tracker["last_switch_time"] = now
        return False, reason, False

    # Enforce minimum switching dwell time to protect relay contacts
    if desired != pump_switch_tracker["last_state"]:
        elapsed = now - pump_switch_tracker["last_switch_time"]
        if elapsed < pump_switch_tracker["min_dwell_seconds"]:
            remaining = int(pump_switch_tracker["min_dwell_seconds"] - elapsed)
            locked_state = pump_switch_tracker["last_state"]
            dwell_reason = f"{reason} [Anti-Chatter Dwell: state held for {remaining}s]"
            return locked_state, dwell_reason, desired
        else:
            pump_switch_tracker["last_state"] = desired
            pump_switch_tracker["last_switch_time"] = now
            return desired, reason, desired

    return desired, reason, desired


def generate_system_alerts(telemetry: dict, target_pump: bool, is_online: bool, mode: str) -> list:
    """Generates real-time diagnostic and safety alerts for dashboard and hardware display."""
    alerts = []
    if not is_online:
        alerts.append("⚠️ ESP32 Hardware Disconnected: Waiting for live telemetry stream")

    water_lvl = telemetry.get("water_level", 2000)
    tank_st = telemetry.get("tank_status", "AVAILABLE")

    # Critical Empty Tank Alert
    if water_lvl < 500 or tank_st == "EMPTY":
        alerts.append("🚨 Water Tank Empty: Pump dry-run lock engaged (Irrigation Inhibited)")
    elif water_lvl < 1000:
        alerts.append(f"⚠️ Water Tank Low ({water_lvl}/4095): Please plan tank refill")

    if str(telemetry.get("rain", "NO")).upper() == "YES":
        alerts.append("🌧️ Rain Sensor Active: Natural precipitation detected (Irrigation Suspended)")

    weather = weather_cache.get("data", {})
    if weather.get("rain_probability_next_6h", 0) > 60:
        alerts.append(f"🌦️ Rain Forecast: {weather['advisory']}")

    if telemetry.get("soil_value", 0) > 3000:
        alerts.append("🌾 Soil Critically Dry: Moisture below critical agricultural threshold")

    if target_pump:
        alerts.append(f"💧 Pump Active ({mode} mode): Delivering water to root zone")

    return alerts


def run_ai_prediction(telemetry_dict: dict, pump_flow: float = 2.5) -> dict:
    profile = live_state.get("profile", {})
    farm_area = float(profile.get("farm_area", 1.5) or 1.5)
    area_unit = str(profile.get("area_unit", "Acres"))

    if predictor is None:
        return {
            "status": "degraded",
            "timestamp": datetime.now().isoformat(),
            "predicted_water_required_l": 0.0,
            "farm_total_water_l": 0.0,
            "farm_area": farm_area,
            "farm_area_unit": area_unit,
            "irrigation_required": False,
            "action": "PUMP_OFF",
            "pump_flow_lpm": pump_flow,
            "pump_runtime_min": 0.0,
            "pump_runtime_sec": 0.0,
            "status_reason": "AI Models not loaded on server"
        }

    input_dict = {
        'Temperature': float(telemetry_dict.get('temperature', 25.0)),
        'Humidity': float(telemetry_dict.get('humidity', 50.0)),
        'Soil Value': int(telemetry_dict.get('soil_value', 1000)),
        'Light Value': int(telemetry_dict.get('light_value', 1000)),
        'Water Level': int(telemetry_dict.get('water_level', 2000)),
        'Rain': str(telemetry_dict.get('rain', 'NO')).upper(),
        'Soil Status': str(telemetry_dict.get('soil_status', 'DRY')).upper(),
        'Tank Status': str(telemetry_dict.get('tank_status', 'AVAILABLE')).upper(),
        'Crop': telemetry_dict.get('crop', profile.get('crop', 'Wheat')),
        'Season': telemetry_dict.get('season', 'Winter'),
        'Growth Stage': telemetry_dict.get('growth_stage', profile.get('growth_stage', 'Vegetative')),
        'Soil Type': telemetry_dict.get('soil_type', profile.get('soil_type', 'Loamy'))
    }

    res = predictor.predict_single(input_dict, pump_flow_lpm=pump_flow)
    predicted_water = res['predicted_water_required_l']
    irrigation_required = res['irrigation_required']
    action = res['action']
    reason = res['status_reason']

    # Rain-Aware Weather Forecast Integration
    weather = get_weather_forecast(
        lat=profile.get("latitude", 18.5204),
        lon=profile.get("longitude", 73.8567)
    )

    # If upcoming rain is forecasted (>75% chance & >=1.5mm) and soil is not critically dry (<3800 ADC)
    soil_adc = input_dict['Soil Value']
    if weather.get("rain_probability_next_6h", 0) > 75 and weather.get("expected_rain_mm_next_6h", 0) >= 1.5 and soil_adc < 3800:
        irrigation_required = False
        action = "PUMP_OFF"
        reason = f"Rain-Aware Forecast: {weather['rain_probability_next_6h']}% rain chance ({weather['expected_rain_mm_next_6h']}mm). Irrigation deferred."
    elif weather.get("rain_probability_next_6h", 0) > 40 and irrigation_required:
        predicted_water = round(predicted_water * 0.70, 2)
        reason = f"{reason} (Volume reduced 30% due to {weather['rain_probability_next_6h']}% rain forecast)"

    # Compute runtime based on finalized volume
    runtime_min = round(predicted_water / pump_flow, 2) if (irrigation_required and action == "PUMP_ON") else 0.0
    runtime_sec = round(runtime_min * 60.0, 1)

    # Personalized Farm Total Water Requirement
    farm_total_water = round(predicted_water * farm_area, 2)

    return {
        "status": "success",
        "timestamp": datetime.now().isoformat(),
        "predicted_water_required_l": predicted_water,
        "farm_total_water_l": farm_total_water,
        "farm_area": farm_area,
        "farm_area_unit": area_unit,
        "irrigation_required": irrigation_required,
        "action": action,
        "pump_flow_lpm": pump_flow,
        "pump_runtime_min": runtime_min,
        "pump_runtime_sec": runtime_sec,
        "status_reason": reason
    }


@app.get("/api/profile")
def get_farmer_profile_endpoint():
    """Returns saved farmer/farm profile for personalized monitoring."""
    return live_state.get("profile", load_farmer_profile())


@app.post("/api/profile")
def update_farmer_profile_endpoint(profile: FarmerProfile):
    """
    Saves farmer profile (name, plot, crop, area, soil type, irrigation requirements)
    and synchronizes it across AI models, telemetry inputs, and dashboard analytics.
    """
    p_dict = profile.model_dump()
    save_farmer_profile(p_dict)
    live_state["profile"] = p_dict
    
    # Sync relevant profile fields to telemetry
    if p_dict.get("crop"):
        live_state["telemetry"]["crop"] = p_dict["crop"]
    if p_dict.get("growth_stage"):
        live_state["telemetry"]["growth_stage"] = p_dict["growth_stage"]
    if p_dict.get("soil_type"):
        live_state["telemetry"]["soil_type"] = p_dict["soil_type"]
    if p_dict.get("pump_flow_lpm"):
        live_state["telemetry"]["pump_flow_lpm"] = p_dict["pump_flow_lpm"]

    clean_tel = sanitize_telemetry(live_state["telemetry"], threshold=live_state["control"]["soil_threshold"])
    live_state["telemetry"].update(clean_tel)
    pred = run_ai_prediction(live_state["telemetry"], pump_flow=clean_tel['pump_flow_lpm'])
    live_state["prediction"] = pred
    target_pump, pump_reason, desired_pump = evaluate_pump_target(
        live_state["control"]["mode"],
        pred,
        live_state["telemetry"],
        live_state["control"]["manual_pump_state"],
        live_state["control"]["soil_threshold"]
    )
    live_state["control"]["target_pump_state"] = target_pump
    live_state["control"]["desired_pump_state"] = desired_pump
    live_state["control"]["pump_reason"] = pump_reason
    live_state["alerts"] = generate_system_alerts(
        live_state["telemetry"],
        target_pump,
        live_state["device_online"],
        live_state["control"]["mode"]
    )

    return {
        "status": "success",
        "profile": p_dict,
        "prediction": pred,
        "control": live_state["control"],
        "alerts": live_state["alerts"]
    }


@app.get("/api/weather")
def get_live_weather():
    """Returns current weather condition and 6h rainfall forecast for farm location."""
    profile = live_state.get("profile", {})
    return get_weather_forecast(
        lat=profile.get("latitude", 18.5204),
        lon=profile.get("longitude", 73.8567)
    )


@app.get("/")
def read_root():
    if os.path.exists("index.html"):
        return FileResponse("index.html")
    return {
        "system": "Smart AIoT Irrigation API Server",
        "status": "online" if predictor is not None else "degraded (models not trained)",
        "timestamp": datetime.now().isoformat(),
        "endpoints": {
            "health": "/health",
            "predict": "/predict (POST)",
            "telemetry_sync": "/api/telemetry (GET/POST)",
            "control_sync": "/api/control (POST)",
            "profile": "/api/profile (GET/POST)",
            "weather": "/api/weather (GET)",
            "docs": "/docs"
        }
    }


@app.get("/health")
def health_check():
    if predictor is None:
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail="Model files not loaded. Run train.py first."
        )
    return {
        "status": "healthy",
        "models": {
            "regressor": predictor.metadata.get("best_regressor_name"),
            "classifier": predictor.metadata.get("best_classifier_name"),
            "training_time": predictor.metadata.get("training_timestamp")
        }
    }


@app.post("/predict", response_model=PredictionResponse)
def predict_irrigation(telemetry: TelemetryInput):
    clean_tel = sanitize_telemetry(telemetry.model_dump(), threshold=live_state["control"]["soil_threshold"])
    pred = run_ai_prediction(clean_tel, pump_flow=clean_tel['pump_flow_lpm'])
    
    # Update live state
    live_state["telemetry"].update(clean_tel)
    live_state["prediction"] = pred
    target, pump_reason, desired_pump = evaluate_pump_target(
        live_state["control"]["mode"],
        pred,
        live_state["telemetry"],
        live_state["control"]["manual_pump_state"],
        live_state["control"]["soil_threshold"]
    )
    live_state["control"]["target_pump_state"] = target
    live_state["control"]["desired_pump_state"] = desired_pump
    live_state["control"]["pump_reason"] = pump_reason
    live_state["alerts"] = generate_system_alerts(
        live_state["telemetry"],
        target,
        live_state["device_online"],
        live_state["control"]["mode"]
    )
    
    return PredictionResponse(**pred)


@app.post("/api/telemetry")
def esp32_telemetry_sync(telemetry: TelemetryInput):
    """
    Main ESP32 Sync Endpoint:
    Receives live ESP32 hardware telemetry, sanitizes sensor inputs, calculates AI model predictions,
    applies hardware safety rules, and returns pump command & operational thresholds to ESP32.
    """
    now_iso = datetime.now().isoformat()
    clean_tel = sanitize_telemetry(telemetry.model_dump(), threshold=live_state["control"]["soil_threshold"])
    
    # Compute AI Prediction
    pred = run_ai_prediction(clean_tel, pump_flow=clean_tel['pump_flow_lpm'])
    
    # Update global live state
    live_state["device_online"] = True
    live_state["last_seen_timestamp"] = now_iso
    live_state["telemetry"].update(clean_tel)
    live_state["prediction"] = pred
    
    actual_reported = bool(clean_tel.get("actual_pump_state", False))
    live_state["actual_pump_state"] = actual_reported
    live_state["control"]["actual_pump_state"] = actual_reported

    target_pump, pump_reason, desired_pump = evaluate_pump_target(
        live_state["control"]["mode"],
        pred,
        live_state["telemetry"],
        live_state["control"]["manual_pump_state"],
        live_state["control"]["soil_threshold"]
    )
    live_state["control"]["target_pump_state"] = target_pump
    live_state["control"]["desired_pump_state"] = desired_pump
    live_state["control"]["pump_reason"] = pump_reason
    live_state["alerts"] = generate_system_alerts(
        live_state["telemetry"],
        target_pump,
        live_state["device_online"],
        live_state["control"]["mode"]
    )

    # Diagnostic logging for pump decisions, safety lockouts, and hardware sync (Task 8)
    print(f"[{datetime.now().strftime('%H:%M:%S')}] ESP32-SYNC | Mode:{live_state['control']['mode']} | Soil:{clean_tel['soil_value']} | Tank:{clean_tel['water_level']} | Target:{'ON' if target_pump else 'OFF'} | Desired:{'ON' if desired_pump else 'OFF'} | ActualRelay:{'ON' if actual_reported else 'OFF'} | Reason:{pump_reason}")

    return {
        "status": "success",
        "timestamp": now_iso,
        "prediction": pred,
        "target_pump_state": target_pump,
        "desired_pump_state": desired_pump,
        "actual_pump_state": actual_reported,
        "action": "PUMP_ON" if target_pump else "PUMP_OFF",
        "predicted_water_required_l": pred["predicted_water_required_l"],
        "farm_total_water_l": pred["farm_total_water_l"],
        "pump_runtime_sec": pred["pump_runtime_sec"],
        "status_reason": pump_reason,
        "mode": live_state["control"]["mode"],
        "soil_threshold": live_state["control"]["soil_threshold"],
        "crop": live_state["telemetry"]["crop"],
        "season": live_state["telemetry"]["season"],
        "weather": live_state.get("weather"),
        "profile": live_state.get("profile"),
        "alerts": live_state["alerts"]
    }


@app.get("/api/telemetry")
def get_live_telemetry():
    """
    Web Dashboard Polling Endpoint:
    Returns full state containing latest ESP32 telemetry, AI predictions, weather forecast, profile, and active controls.
    """
    # Check device online status (timeout 12 seconds with clock skew tolerance)
    if live_state["last_seen_timestamp"]:
        try:
            last_dt = datetime.fromisoformat(live_state["last_seen_timestamp"])
            diff_sec = (datetime.now() - last_dt).total_seconds()
            live_state["device_online"] = (0 <= diff_sec <= 12.0)
        except Exception:
            live_state["device_online"] = False
    else:
        live_state["device_online"] = False

    # When hardware is offline, enforce physical disengagement fail-safe on dashboard
    if not live_state["device_online"]:
        live_state["control"]["target_pump_state"] = False
        live_state["control"]["actual_pump_state"] = False
        live_state["actual_pump_state"] = False
        live_state["control"]["pump_reason"] = "Safety Disengaged: ESP32 Hardware Disconnected (Failsafe Active)"

    # Refresh weather forecast
    profile = live_state.get("profile", {})
    live_state["weather"] = get_weather_forecast(
        lat=profile.get("latitude", 18.5204),
        lon=profile.get("longitude", 73.8567)
    )

    # Refresh alerts
    live_state["alerts"] = generate_system_alerts(
        live_state["telemetry"],
        live_state["control"]["target_pump_state"],
        live_state["device_online"],
        live_state["control"]["mode"]
    )

    return live_state


@app.post("/api/control")
def update_control_and_settings(ctrl: ControlInput):
    """
    Updates operational mode, pump manual override, soil threshold, or farm settings across server and ESP32.
    """
    if ctrl.mode is not None:
        if ctrl.mode in ["AI_AUTO", "THRESHOLD_AUTO", "MANUAL"]:
            live_state["control"]["mode"] = ctrl.mode
    
    if ctrl.manual_pump_state is not None:
        live_state["control"]["manual_pump_state"] = ctrl.manual_pump_state
        if ctrl.mode is None:
            live_state["control"]["mode"] = "MANUAL"

    if ctrl.soil_threshold is not None:
        live_state["control"]["soil_threshold"] = ctrl.soil_threshold

    for key in ["crop", "season", "growth_stage", "soil_type"]:
        val = getattr(ctrl, key)
        if val is not None:
            live_state["telemetry"][key] = val

    # Sanitize and re-run prediction & evaluate pump target with updated settings
    clean_tel = sanitize_telemetry(live_state["telemetry"], threshold=live_state["control"]["soil_threshold"])
    live_state["telemetry"].update(clean_tel)
    pred = run_ai_prediction(live_state["telemetry"], pump_flow=clean_tel['pump_flow_lpm'])
    live_state["prediction"] = pred
    target_pump, pump_reason, desired_pump = evaluate_pump_target(
        live_state["control"]["mode"],
        pred,
        live_state["telemetry"],
        live_state["control"]["manual_pump_state"],
        live_state["control"]["soil_threshold"]
    )
    live_state["control"]["target_pump_state"] = target_pump
    live_state["control"]["desired_pump_state"] = desired_pump
    live_state["control"]["pump_reason"] = pump_reason
    live_state["alerts"] = generate_system_alerts(
        live_state["telemetry"],
        target_pump,
        live_state["device_online"],
        live_state["control"]["mode"]
    )

    return {
        "status": "success",
        "control": live_state["control"],
        "telemetry": live_state["telemetry"],
        "prediction": pred,
        "alerts": live_state["alerts"]
    }


@app.post("/batch_predict")
def batch_predict(telemetries: List[TelemetryInput]):
    if predictor is None:
        raise HTTPException(status_code=503, detail="Models not loaded")

    results = []
    for tel in telemetries:
        res = run_ai_prediction(tel.model_dump(), pump_flow=tel.pump_flow_lpm or 2.5)
        results.append(res)
    return {"status": "success", "count": len(results), "predictions": results}


if __name__ == '__main__':
    import uvicorn
    # Bind to 0.0.0.0 so ESP32 on local WiFi network can communicate with the server
    uvicorn.run(app, host="0.0.0.0", port=8000)
