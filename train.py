import os
import json
import joblib
import numpy as np
import pandas as pd
from datetime import datetime

from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.compose import ColumnTransformer
from sklearn.pipeline import Pipeline
from sklearn.impute import SimpleImputer

# Regression Models
from sklearn.ensemble import RandomForestRegressor, GradientBoostingRegressor
from sklearn.tree import DecisionTreeRegressor
from sklearn.linear_model import Ridge
from xgboost import XGBRegressor
from lightgbm import LGBMRegressor
from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score

# Classification Models
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.linear_model import LogisticRegression
from xgboost import XGBClassifier
from lightgbm import LGBMClassifier
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, roc_auc_score


def load_and_preprocess_dataset(filepath):
    print("=" * 70)
    print(f"1. Loading dataset from '{filepath}'...")
    df = pd.read_csv(filepath)
    print(f"Total rows in CSV: {len(df)}")
    print(f"Data source breakdown:\n{df['Data Source'].value_counts()}")

    # Separate Synthetic (training data) from User Actual (unlabeled test data)
    synthetic_df = df[df['Data Source'] == 'Synthetic'].copy()
    user_actual_df = df[df['Data Source'] == 'User Actual'].copy()

    print(f"\nSynthetic rows (Training dataset): {len(synthetic_df)}")
    print(f"User Actual rows (Unlabeled field test logs): {len(user_actual_df)}")
    
    # Construct binary classification target for 'Irrigation Required'
    # Domain Logic: Irrigation is required if soil is DRY or water requirement > 8.0 L,
    # provided it is NOT raining and water tank is AVAILABLE.
    synthetic_df['Irrigation Required_Target'] = (
        ((synthetic_df['Soil Status'] == 'DRY') | (synthetic_df['Water Required (L)'] > 8.0)) &
        (synthetic_df['Rain'] == 'NO') &
        (synthetic_df['Tank Status'] == 'AVAILABLE')
    ).astype(int)

    print("\nIrrigation Required Target Class Distribution (Synthetic):")
    cls_counts = synthetic_df['Irrigation Required_Target'].value_counts()
    print(f"  Class 0 (No Irrigation): {cls_counts.get(0, 0)} ({cls_counts.get(0, 0)/len(synthetic_df)*100:.1f}%)")
    print(f"  Class 1 (Irrigation Required): {cls_counts.get(1, 0)} ({cls_counts.get(1, 0)/len(synthetic_df)*100:.1f}%)")

    return synthetic_df, user_actual_df


def build_preprocessor(num_features, cat_features):
    num_transformer = Pipeline([
        ('imputer', SimpleImputer(strategy='median')),
        ('scaler', StandardScaler())
    ])

    cat_transformer = Pipeline([
        ('imputer', SimpleImputer(strategy='most_frequent')),
        ('onehot', OneHotEncoder(handle_unknown='ignore', sparse_output=False))
    ])

    preprocessor = ColumnTransformer([
        ('num', num_transformer, num_features),
        ('cat', cat_transformer, cat_features)
    ])

    return preprocessor


def train_and_eval_regressors(X_train_proc, X_test_proc, y_train, y_test):
    print("\n" + "=" * 70)
    print("2. Training and Comparing REGRESSION Models (Target: Water Required (L))")
    print("=" * 70)

    regressors = {
        'Random Forest': RandomForestRegressor(n_estimators=100, random_state=42),
        'XGBoost': XGBRegressor(n_estimators=100, random_state=42, verbosity=0),
        'LightGBM': LGBMRegressor(n_estimators=100, random_state=42, verbose=-1),
        'Gradient Boosting': GradientBoostingRegressor(n_estimators=100, random_state=42),
        'Decision Tree': DecisionTreeRegressor(random_state=42),
        'Ridge Regression': Ridge(alpha=1.0)
    }

    results = []
    best_name = None
    best_model = None
    best_rmse = float('inf')

    for name, model in regressors.items():
        model.fit(X_train_proc, y_train)
        y_pred = model.predict(X_test_proc)

        rmse = np.sqrt(mean_squared_error(y_test, y_pred))
        mae = mean_absolute_error(y_test, y_pred)
        r2 = r2_score(y_test, y_pred)

        results.append({
            'Model': name,
            'RMSE (L)': rmse,
            'MAE (L)': mae,
            'R² Score': r2
        })

        if rmse < best_rmse:
            best_rmse = rmse
            best_name = name
            best_model = model

    results_df = pd.DataFrame(results).sort_values(by='RMSE (L)')
    print(results_df.to_string(index=False))
    print(f"\n>>> Best Regression Model: {best_name} (RMSE: {best_rmse:.4f} L)")

    return best_name, best_model, results_df


def train_and_eval_classifiers(X_train_proc, X_test_proc, y_train, y_test):
    print("\n" + "=" * 70)
    print("3. Training and Comparing CLASSIFICATION Models (Target: Irrigation Required)")
    print("=" * 70)

    classifiers = {
        'Random Forest': RandomForestClassifier(n_estimators=100, random_state=42),
        'XGBoost': XGBClassifier(n_estimators=100, random_state=42, eval_metric='logloss'),
        'LightGBM': LGBMClassifier(n_estimators=100, random_state=42, verbose=-1),
        'Gradient Boosting': GradientBoostingClassifier(n_estimators=100, random_state=42),
        'Decision Tree': DecisionTreeClassifier(random_state=42),
        'Logistic Regression': LogisticRegression(max_iter=1000, random_state=42)
    }

    results = []
    best_name = None
    best_model = None
    best_f1 = -1.0

    for name, model in classifiers.items():
        model.fit(X_train_proc, y_train)
        y_pred = model.predict(X_test_proc)
        y_proba = model.predict_proba(X_test_proc)[:, 1] if hasattr(model, "predict_proba") else y_pred

        acc = accuracy_score(y_test, y_pred)
        prec = precision_score(y_test, y_pred, zero_division=0)
        rec = recall_score(y_test, y_pred, zero_division=0)
        f1 = f1_score(y_test, y_pred, zero_division=0)
        roc_auc = roc_auc_score(y_test, y_proba)

        results.append({
            'Model': name,
            'Accuracy': acc,
            'Precision': prec,
            'Recall': rec,
            'F1-Score': f1,
            'ROC-AUC': roc_auc
        })

        if f1 > best_f1:
            best_f1 = f1
            best_name = name
            best_model = model

    results_df = pd.DataFrame(results).sort_values(by='F1-Score', ascending=False)
    print(results_df.to_string(index=False))
    print(f"\n>>> Best Classification Model: {best_name} (F1-Score: {best_f1:.4f})")

    return best_name, best_model, results_df


def save_artifacts(models_dir, preprocessor, best_regressor_name, best_regressor, best_classifier_name, best_classifier, num_features, cat_features, reg_results, cls_results):
    os.makedirs(models_dir, exist_ok=True)
    
    prep_path = os.path.join(models_dir, 'preprocessor.joblib')
    reg_path = os.path.join(models_dir, 'best_regression_model.joblib')
    cls_path = os.path.join(models_dir, 'best_classification_model.joblib')
    meta_path = os.path.join(models_dir, 'model_metadata.json')

    joblib.dump(preprocessor, prep_path)
    joblib.dump(best_regressor, reg_path)
    joblib.dump(best_classifier, cls_path)

    metadata = {
        'training_timestamp': datetime.now().isoformat(),
        'num_features': num_features,
        'cat_features': cat_features,
        'best_regressor_name': best_regressor_name,
        'best_classifier_name': best_classifier_name,
        'regression_metrics': reg_results.to_dict(orient='records'),
        'classification_metrics': cls_results.to_dict(orient='records')
    }

    with open(meta_path, 'w') as f:
        json.dump(metadata, f, indent=2)

    print("\n" + "=" * 70)
    print("4. Artifacts Successfully Saved:")
    print(f"  - Preprocessor: {prep_path}")
    print(f"  - Regression Model: {reg_path}")
    print(f"  - Classification Model: {cls_path}")
    print(f"  - Metadata: {meta_path}")
    print("=" * 70)


def evaluate_user_actual_samples(user_actual_df, preprocessor, regressor, classifier, num_features, cat_features):
    print("\n" + "=" * 70)
    print("5. Evaluating Inference Pipeline on 11 User Actual Field Telemetry Rows")
    print("=" * 70)

    if len(user_actual_df) == 0:
        print("No User Actual rows found.")
        return

    # Fill missing crop/season info for field testing preview
    sample_df = user_actual_df.copy()
    sample_df['Crop'] = sample_df['Crop'].fillna('Wheat')
    sample_df['Season'] = sample_df['Season'].fillna('Winter')
    sample_df['Growth Stage'] = sample_df['Growth Stage'].fillna('Vegetative')
    sample_df['Soil Type'] = sample_df['Soil Type'].fillna('Loamy')
    sample_df['Tank Status'] = sample_df['Tank Status'].fillna('AVAILABLE')
    sample_df['Soil Status'] = sample_df['Soil Status'].fillna('DRY')

    X_actual = sample_df[num_features + cat_features]
    X_actual_proc = preprocessor.transform(X_actual)

    predicted_water = regressor.predict(X_actual_proc)
    predicted_irr_req = classifier.predict(X_actual_proc)

    pump_flow_lpm = 2.5  # standard ESP32 pump flow rate (2.5 L/min)
    results = []

    for i, (_, row) in enumerate(sample_df.iterrows()):
        water_l = max(0.0, float(predicted_water[i]))
        irr_needed = bool(predicted_irr_req[i] == 1)
        
        # Apply safety overrides
        if row['Rain'] == 'YES' or row['Tank Status'] == 'EMPTY' or row['Soil Status'] == 'WET':
            action = "PUMP_OFF"
            irr_needed = False
            runtime_min = 0.0
            runtime_sec = 0.0
        else:
            action = "PUMP_ON" if irr_needed else "PUMP_OFF"
            runtime_min = water_l / pump_flow_lpm if irr_needed else 0.0
            runtime_sec = runtime_min * 60.0

        results.append({
            'Row': i + 1,
            'Temp (°C)': row['Temperature'],
            'Hum (%)': row['Humidity'],
            'Soil Val': row['Soil Value'],
            'Soil Stat': row['Soil Status'],
            'Rain': row['Rain'],
            'Tank': row['Tank Status'],
            'Pred Water (L)': round(water_l, 2),
            'Irr Required': irr_needed,
            'Action': action,
            'Runtime (sec)': round(runtime_sec, 1)
        })

    eval_df = pd.DataFrame(results)
    print(eval_df.to_string(index=False))


def main():
    filepath = 'smart_irrigation_augmented_training_data.csv'
    models_dir = 'models'

    num_features = ['Temperature', 'Humidity', 'Soil Value', 'Light Value', 'Water Level']
    cat_features = ['Rain', 'Soil Status', 'Tank Status', 'Crop', 'Season', 'Growth Stage', 'Soil Type']

    # 1. Load data & separate Synthetic from User Actual
    synthetic_df, user_actual_df = load_and_preprocess_dataset(filepath)

    X = synthetic_df[num_features + cat_features]
    y_reg = synthetic_df['Water Required (L)']
    y_cls = synthetic_df['Irrigation Required_Target']

    # 2. Train/Test Split (80/20)
    X_train, X_test, y_reg_train, y_reg_test, y_cls_train, y_cls_test = train_test_split(
        X, y_reg, y_cls, test_size=0.2, random_state=42, stratify=y_cls
    )
    print(f"\nTrain set size: {len(X_train)} rows | Test set size: {len(X_test)} rows")

    # 3. Fit Preprocessor
    preprocessor = build_preprocessor(num_features, cat_features)
    X_train_proc = preprocessor.fit_transform(X_train)
    X_test_proc = preprocessor.transform(X_test)

    # 4. Train & Compare Regressors
    best_reg_name, best_reg_model, reg_results = train_and_eval_regressors(
        X_train_proc, X_test_proc, y_reg_train, y_reg_test
    )

    # 5. Train & Compare Classifiers
    best_cls_name, best_cls_model, cls_results = train_and_eval_classifiers(
        X_train_proc, X_test_proc, y_cls_train, y_cls_test
    )

    # 6. Fit preprocessor on full Synthetic dataset for deployment
    preprocessor.fit(X)
    X_all_proc = preprocessor.transform(X)
    best_reg_model.fit(X_all_proc, y_reg)
    best_cls_model.fit(X_all_proc, y_cls)

    # 7. Save Models and Metadata
    save_artifacts(
        models_dir, preprocessor,
        best_reg_name, best_reg_model,
        best_cls_name, best_cls_model,
        num_features, cat_features,
        reg_results, cls_results
    )

    # 8. Test prediction on User Actual samples
    evaluate_user_actual_samples(
        user_actual_df, preprocessor, best_reg_model, best_cls_model, num_features, cat_features
    )


if __name__ == '__main__':
    main()
