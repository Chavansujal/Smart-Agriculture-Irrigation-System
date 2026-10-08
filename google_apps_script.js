/**
 * Google Apps Script for Smart Irrigation System Integration
 * 
 * Instructions:
 * 1. Open your Google Sheet.
 * 2. Go to Extensions > Apps Script.
 * 3. Paste this code into Code.gs.
 * 4. Update ML_API_URL with your hosted FastAPI backend server URL (e.g. ngrok or public IP).
 * 5. Click Deploy > New deployment > Select 'Web app'.
 * 6. Set 'Execute as': Me, 'Who has access': Anyone.
 * 7. Deploy and copy the Web App URL.
 */

// Replace with your ML API Server URL (e.g., http://YOUR_SERVER_IP:8000/predict or ngrok HTTPS URL)
const ML_API_URL = "http://YOUR_SERVER_IP:8000/predict";

/**
 * Triggered automatically when ESP32 or HTTP client sends POST request to Web App URL.
 */
function doPost(e) {
  try {
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

    // Create Headers if sheet is empty
    if (sheet.getLastRow() === 0) {
      sheet.appendRow([
        "Timestamp",
        "Temperature (°C)",
        "Humidity (%)",
        "Soil Value",
        "Light Value",
        "Water Level",
        "Rain",
        "Soil Status",
        "Tank Status",
        "Crop",
        "Season",
        "Growth Stage",
        "Soil Type",
        "Water Required (L)",
        "Irrigation Required",
        "Action",
        "Pump Runtime (sec)",
        "Status Reason"
      ]);
    }

    // Parse incoming JSON telemetry from ESP32
    const telemetryData = JSON.parse(e.postData.contents);

    // Forward telemetry to ML Model API
    const options = {
      method: "post",
      contentType: "application/json",
      payload: JSON.stringify(telemetryData),
      muteHttpExceptions: true
    };

    const response = UrlFetchApp.fetch(ML_API_URL, options);
    const mlResult = JSON.parse(response.getContentText());

    // Extract values
    const timestamp = new Date();
    const temp = telemetryData.temperature || 0;
    const humidity = telemetryData.humidity || 0;
    const soilVal = telemetryData.soil_value || 0;
    const lightVal = telemetryData.light_value || 0;
    const waterLvl = telemetryData.water_level || 0;
    const rain = telemetryData.rain || "NO";
    const soilStatus = telemetryData.soil_status || "DRY";
    const tankStatus = telemetryData.tank_status || "AVAILABLE";
    const crop = telemetryData.crop || "Wheat";
    const season = telemetryData.season || "Winter";
    const growthStage = telemetryData.growth_stage || "Vegetative";
    const soilType = telemetryData.soil_type || "Loamy";

    const waterReqL = mlResult.predicted_water_required_l || 0.0;
    const irrReq = mlResult.irrigation_required ? "YES" : "NO";
    const action = mlResult.action || "PUMP_OFF";
    const runtimeSec = mlResult.pump_runtime_sec || 0.0;
    const reason = mlResult.status_reason || "OK";

    // Append to Google Sheet
    sheet.appendRow([
      timestamp,
      temp,
      humidity,
      soilVal,
      lightVal,
      waterLvl,
      rain,
      soilStatus,
      tankStatus,
      crop,
      season,
      growthStage,
      soilType,
      waterReqL,
      irrReq,
      action,
      runtimeSec,
      reason
    ]);

    // Return prediction response back to ESP32
    return ContentService.createTextOutput(JSON.stringify(mlResult))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (error) {
    return ContentService.createTextOutput(JSON.stringify({
      status: "error",
      message: error.toString()
    })).setMimeType(ContentService.MimeType.JSON);
  }
}

/**
 * Simple GET endpoint test
 */
function doGet(e) {
  return ContentService.createTextOutput("Smart Irrigation Google Apps Script Web App is Live!");
}
