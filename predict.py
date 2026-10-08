import os
import json
import joblib
import pandas as pd
import argparse


class SmartIrrigationPredictor:
    """
    Inference Pipeline for Smart Irrigation System.
    Loads fitted preprocessor, trained regression model, and trained classification model.
    """
    def __init__(self, models_dir='models'):
        self.models_dir = models_dir
        prep_path = os.path.join(models_dir, 'preprocessor.joblib')
        reg_path = os.path.join(models_dir, 'best_regression_model.joblib')
        cls_path = os.path.join(models_dir, 'best_classification_model.joblib')
        meta_path = os.path.join(models_dir, 'model_metadata.json')

        if not (os.path.exists(prep_path) and os.path.exists(reg_path) and os.path.exists(cls_path)):
            raise FileNotFoundError(f"Model artifacts missing in '{models_dir}'. Run train.py first.")

        self.preprocessor = joblib.load(prep_path)
        self.regressor = joblib.load(reg_path)
        self.classifier = joblib.load(cls_path)

        with open(meta_path, 'r') as f:
            self.metadata = json.load(f)

        self.num_features = self.metadata.get('num_features', ['Temperature', 'Humidity', 'Soil Value', 'Light Value', 'Water Level'])
        self.cat_features = self.metadata.get('cat_features', ['Rain', 'Soil Status', 'Tank Status', 'Crop', 'Season', 'Growth Stage', 'Soil Type'])

    def predict_single(self, input_dict, pump_flow_lpm=2.5):
        """
        Takes a single dictionary of feature values and returns prediction output.
        """
        df = pd.DataFrame([input_dict])
        return self.predict_dataframe(df, pump_flow_lpm=pump_flow_lpm)[0]

    def predict_dataframe(self, df_input, pump_flow_lpm=2.5):
        """
        Takes a DataFrame containing input features and returns list of prediction dictionaries.
        """
        df = df_input.copy()

        # Fill defaults for any missing categorical values
        defaults = {
            'Temperature': 25.0,
            'Humidity': 50.0,
            'Soil Value': 1000,
            'Light Value': 1000,
            'Water Level': 2000,
            'Rain': 'NO',
            'Soil Status': 'DRY',
            'Tank Status': 'AVAILABLE',
            'Crop': 'Wheat',
            'Season': 'Winter',
            'Growth Stage': 'Vegetative',
            'Soil Type': 'Loamy'
        }

        for col, default_val in defaults.items():
            if col not in df.columns or df[col].isnull().all():
                df[col] = default_val
            else:
                df[col] = df[col].fillna(default_val)

        X = df[self.num_features + self.cat_features]
        X_proc = self.preprocessor.transform(X)

        raw_water_pred = self.regressor.predict(X_proc)
        raw_irr_pred = self.classifier.predict(X_proc)

        results = []
        for i, row in df.iterrows():
            predicted_water = max(0.0, float(raw_water_pred[i]))
            model_irr_req = bool(raw_irr_pred[i] == 1)

            rain = str(row['Rain']).upper().strip()
            tank_status = str(row['Tank Status']).upper().strip()
            soil_status = str(row['Soil Status']).upper().strip()

            # Safety Overrides
            # 1. Tank Empty -> Cannot pump
            # 2. Raining -> No irrigation needed
            # 3. Soil Wet -> No irrigation needed
            if tank_status == 'EMPTY' or rain == 'YES' or soil_status == 'WET':
                irrigation_required = False
                action = "PUMP_OFF"
                runtime_min = 0.0
                runtime_sec = 0.0
                reason = "Safety Override: Tank Empty / Rain active / Soil Wet" if (tank_status == 'EMPTY' or rain == 'YES' or soil_status == 'WET') else "Not Required"
            else:
                irrigation_required = model_irr_req
                action = "PUMP_ON" if irrigation_required else "PUMP_OFF"
                runtime_min = (predicted_water / pump_flow_lpm) if irrigation_required else 0.0
                runtime_sec = runtime_min * 60.0
                reason = "Irrigation Required" if irrigation_required else "Moisture Adequate"

            results.append({
                'predicted_water_required_l': round(predicted_water, 2),
                'irrigation_required': irrigation_required,
                'action': action,
                'pump_flow_lpm': pump_flow_lpm,
                'pump_runtime_min': round(runtime_min, 2),
                'pump_runtime_sec': round(runtime_sec, 1),
                'status_reason': reason
            })

        return results


def main():
    parser = argparse.ArgumentParser(description="Smart Irrigation Model Inference CLI")
    parser.add_argument('--temp', type=float, default=26.5, help="Temperature in deg C")
    parser.add_argument('--humidity', type=float, default=45.0, help="Humidity in percent")
    parser.add_argument('--soil_val', type=int, default=450, help="Soil Moisture Value (0-4095)")
    parser.add_argument('--light', type=int, default=1200, help="Light Sensor Value")
    parser.add_argument('--water_lvl', type=int, default=1800, help="Water Level Sensor Value")
    parser.add_argument('--rain', type=str, default="NO", choices=["YES", "NO"], help="Rain Status")
    parser.add_argument('--soil_status', type=str, default="DRY", choices=["DRY", "WET", "OFF"], help="Soil Status")
    parser.add_argument('--tank_status', type=str, default="AVAILABLE", choices=["AVAILABLE", "EMPTY"], help="Tank Status")
    parser.add_argument('--crop', type=str, default="Wheat", help="Crop Name")
    parser.add_argument('--season', type=str, default="Winter", help="Season")
    parser.add_argument('--growth_stage', type=str, default="Vegetative", help="Growth Stage")
    parser.add_argument('--soil_type', type=str, default="Loamy", help="Soil Type")
    parser.add_argument('--pump_flow', type=float, default=2.5, help="Pump Flow Rate in L/min")

    args = parser.parse_args()

    input_data = {
        'Temperature': args.temp,
        'Humidity': args.humidity,
        'Soil Value': args.soil_val,
        'Light Value': args.light,
        'Water Level': args.water_lvl,
        'Rain': args.rain,
        'Soil Status': args.soil_status,
        'Tank Status': args.tank_status,
        'Crop': args.crop,
        'Season': args.season,
        'Growth Stage': args.growth_stage,
        'Soil Type': args.soil_type
    }

    predictor = SmartIrrigationPredictor()
    result = predictor.predict_single(input_data, pump_flow_lpm=args.pump_flow)

    print("=" * 60)
    print("SMART IRRIGATION INFERENCE RESULT")
    print("=" * 60)
    print("Input Features:")
    for k, v in input_data.items():
        print(f"  - {k}: {v}")
    print("-" * 60)
    print("Output Predictions:")
    for k, v in result.items():
        print(f"  - {k}: {v}")
    print("=" * 60)


if __name__ == '__main__':
    main()
