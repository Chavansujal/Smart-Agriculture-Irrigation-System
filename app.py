import os
import time
from datetime import datetime
from typing import Optional, List
from fastapi import FastAPI, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from pydantic import BaseModel, Field
from predict import SmartIrrigationPredictor

app = FastAPI(
    title="Smart AIoT Irrigation API",
    description="Backend API syncing ESP32 Hardware, AI Inference Engine, and Web Dashboard",
    version="2.0.0"
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

# In-memory Live State Store for ESP32 Hardware & Web Dashboard Synchronization
live_state = {
    "device_online": False,
    "last_seen_timestamp": None,
    "telemetry": {
        "temperature": 26.5,
        "humidity": 45.0,
        "soil_value": 450,
        "light_value": 1200,
        "water_level": 1800,
        "rain": "NO",
        "soil_status": "DRY",
        "tank_status": "AVAILABLE",
        "crop": "Wheat",
        "season": "Winter",
        "growth_stage": "Vegetative",
        "soil_type": "Loamy",
        "pump_flow_lpm": 2.5
    },
    "prediction": {
        "status": "success",
        "timestamp": datetime.now().isoformat(),
        "predicted_water_required_l": 0.0,
        "irrigation_required": False,
        "action": "PUMP_OFF",
        "pump_flow_lpm": 2.5,
        "pump_runtime_min": 0.0,
        "pump_runtime_sec": 0.0,
        "status_reason": "System Initializing"
    },
    "control": {
        "mode": "AI_AUTO",  # Modes: 'AI_AUTO', 'THRESHOLD_AUTO', 'MANUAL'
        "manual_pump_state": False,
        "soil_threshold": 2500,
        "target_pump_state": False
    }
}


def evaluate_pump_target(mode: str, prediction: dict, telemetry: dict, manual_state: bool, threshold: int) -> bool:
    """Evaluates the target physical pump state based on active operational mode."""
    if mode == "AI_AUTO":
        return prediction.get("action") == "PUMP_ON"
    elif mode == "THRESHOLD_AUTO":
        soil_val = telemetry.get("soil_value", 0)
        rain = str(telemetry.get("rain", "NO")).upper()
        tank = str(telemetry.get("tank_status", "AVAILABLE")).upper()
        if tank == "EMPTY" or rain == "YES":
            return False
        return soil_val > threshold
    elif mode == "MANUAL":
        return manual_state
    return False


def run_ai_prediction(telemetry_dict: dict, pump_flow: float = 2.5) -> dict:
    if predictor is None:
        return {
            "status": "degraded",
            "timestamp": datetime.now().isoformat(),
            "predicted_water_required_l": 0.0,
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
        'Crop': telemetry_dict.get('crop', 'Wheat'),
        'Season': telemetry_dict.get('season', 'Winter'),
        'Growth Stage': telemetry_dict.get('growth_stage', 'Vegetative'),
        'Soil Type': telemetry_dict.get('soil_type', 'Loamy')
    }

    res = predictor.predict_single(input_dict, pump_flow_lpm=pump_flow)
    return {
        "status": "success",
        "timestamp": datetime.now().isoformat(),
        "predicted_water_required_l": res['predicted_water_required_l'],
        "irrigation_required": res['irrigation_required'],
        "action": res['action'],
        "pump_flow_lpm": res['pump_flow_lpm'],
        "pump_runtime_min": res['pump_runtime_min'],
        "pump_runtime_sec": res['pump_runtime_sec'],
        "status_reason": res['status_reason']
    }


class TelemetryInput(BaseModel):
    temperature: float = Field(..., description="Ambient temperature in deg C")
    humidity: float = Field(..., description="Relative humidity in percent")
    soil_value: int = Field(..., description="Soil moisture raw ADC reading (0-4095)")
    light_value: int = Field(..., description="Light LDR raw sensor value")
    water_level: int = Field(..., description="Tank water level sensor value")
    rain: str = Field(default="NO", description="Rain sensor status ('YES' or 'NO')")
    soil_status: str = Field(default="DRY", description="Soil status ('DRY', 'WET', 'OFF')")
    tank_status: str = Field(default="AVAILABLE", description="Tank status ('AVAILABLE', 'EMPTY')")
    crop: Optional[str] = Field(default="Wheat", description="Crop type")
    season: Optional[str] = Field(default="Winter", description="Current season")
    growth_stage: Optional[str] = Field(default="Vegetative", description="Crop growth stage")
    soil_type: Optional[str] = Field(default="Loamy", description="Soil classification")
    pump_flow_lpm: Optional[float] = Field(default=2.5, description="Water pump flow rate in Liters per minute")


class PredictionResponse(BaseModel):
    status: str
    timestamp: str
    predicted_water_required_l: float
    irrigation_required: bool
    action: str
    pump_flow_lpm: float
    pump_runtime_min: float
    pump_runtime_sec: float
    status_reason: str


class ControlInput(BaseModel):
    mode: Optional[str] = Field(None, description="Mode: 'AI_AUTO', 'THRESHOLD_AUTO', 'MANUAL'")
    manual_pump_state: Optional[bool] = Field(None, description="Manual pump toggle ON/OFF")
    soil_threshold: Optional[int] = Field(None, description="Soil dryness threshold value")
    crop: Optional[str] = Field(None, description="Crop name")
    season: Optional[str] = Field(None, description="Season name")
    growth_stage: Optional[str] = Field(None, description="Growth stage")
    soil_type: Optional[str] = Field(None, description="Soil type")


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
    pred = run_ai_prediction(telemetry.model_dump(), pump_flow=telemetry.pump_flow_lpm or 2.5)
    
    # Update live state
    live_state["telemetry"].update(telemetry.model_dump())
    live_state["prediction"] = pred
    target = evaluate_pump_target(
        live_state["control"]["mode"],
        pred,
        live_state["telemetry"],
        live_state["control"]["manual_pump_state"],
        live_state["control"]["soil_threshold"]
    )
    live_state["control"]["target_pump_state"] = target
    
    return PredictionResponse(**pred)


@app.post("/api/telemetry")
def esp32_telemetry_sync(telemetry: TelemetryInput):
    """
    Main ESP32 Sync Endpoint:
    Receives live ESP32 hardware telemetry, calculates AI model predictions,
    updates real-time server state, and returns prediction & physical pump command to ESP32.
    """
    now_iso = datetime.now().isoformat()
    t_dict = telemetry.model_dump()
    
    # Compute AI Prediction
    pred = run_ai_prediction(t_dict, pump_flow=telemetry.pump_flow_lpm or 2.5)
    
    # Update global live state
    live_state["device_online"] = True
    live_state["last_seen_timestamp"] = now_iso
    live_state["telemetry"].update(t_dict)
    live_state["prediction"] = pred
    
    target_pump = evaluate_pump_target(
        live_state["control"]["mode"],
        pred,
        live_state["telemetry"],
        live_state["control"]["manual_pump_state"],
        live_state["control"]["soil_threshold"]
    )
    live_state["control"]["target_pump_state"] = target_pump

    return {
        "status": "success",
        "timestamp": now_iso,
        "prediction": pred,
        "target_pump_state": target_pump,
        "action": pred["action"],
        "predicted_water_required_l": pred["predicted_water_required_l"],
        "pump_runtime_sec": pred["pump_runtime_sec"],
        "status_reason": pred["status_reason"],
        "mode": live_state["control"]["mode"],
        "crop": live_state["telemetry"]["crop"],
        "season": live_state["telemetry"]["season"]
    }


@app.get("/api/telemetry")
def get_live_telemetry():
    """
    Web Dashboard Polling Endpoint:
    Returns full state containing latest ESP32 telemetry, AI predictions, and active controls.
    """
    # Check device online status (timeout 10 seconds)
    if live_state["last_seen_timestamp"]:
        try:
            last_dt = datetime.fromisoformat(live_state["last_seen_timestamp"])
            diff_sec = (datetime.now() - last_dt).total_seconds()
            live_state["device_online"] = (diff_sec <= 10.0)
        except Exception:
            live_state["device_online"] = False
    else:
        live_state["device_online"] = False

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

    if ctrl.soil_threshold is not None:
        live_state["control"]["soil_threshold"] = ctrl.soil_threshold

    for key in ["crop", "season", "growth_stage", "soil_type"]:
        val = getattr(ctrl, key)
        if val is not None:
            live_state["telemetry"][key] = val

    # Re-run prediction & evaluate pump target with updated settings
    pred = run_ai_prediction(live_state["telemetry"])
    live_state["prediction"] = pred
    target_pump = evaluate_pump_target(
        live_state["control"]["mode"],
        pred,
        live_state["telemetry"],
        live_state["control"]["manual_pump_state"],
        live_state["control"]["soil_threshold"]
    )
    live_state["control"]["target_pump_state"] = target_pump

    return {
        "status": "success",
        "control": live_state["control"],
        "telemetry": live_state["telemetry"],
        "prediction": pred
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
