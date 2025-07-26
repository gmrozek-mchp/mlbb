import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import matplotlib.pyplot as plt
import glob
import os

# Q15 format constants
Q15_SCALE = 2**15  # 2^15 for Q15 format
Q15_MAX = 2**15 - 1  # Maximum positive value (32767)
Q15_MIN = -2**15    # Minimum negative value (-32768)

def float_to_q15(value):
    """Convert float value to Q15 format"""
    return np.clip(int(value * Q15_SCALE), Q15_MIN, Q15_MAX)

def q15_to_float(q15_value):
    """Convert Q15 value to float"""
    return q15_value / Q15_SCALE

def array_float_to_q15(array):
    """Convert float array to Q15 array with clamping"""
    return np.clip((array * Q15_SCALE).astype(np.int16), Q15_MIN, Q15_MAX)

def array_q15_to_float(array):
    """Convert Q15 array to float array"""
    return array.astype(np.float32) / Q15_SCALE

def create_neural_network():
    """
    Create a fully connected neural network for Q15 format control with:
    - 6 input nodes (Q15 sensor inputs)
    - 1 hidden layer with 24 nodes
    - 3 output nodes (Q15 control signals)
    Input and hidden layers use linear activation, output uses tanh for bounded control.
    """
    
    # Create the model
    model = keras.Sequential([
        # Input layer (6 nodes) - Q15 sensor inputs
        layers.Dense(6, activation='linear', input_shape=(6,), name='sensor_input'),
        
        # First hidden layer (12 nodes) - control logic
        layers.Dense(12, activation='relu', name='control_logic_1'),
        
        # Second hidden layer (12 nodes) - control logic
        layers.Dense(12, activation='relu', name='control_logic_2'),

        # Output layer (3 nodes) - Q15 bounded control signals
        layers.Dense(3, activation='tanh', name='control_output')  # Tanh for bounded outputs [-1, 1]
    ])
    
    # Compile the model for Q15 control applications
    model.compile(
        optimizer='adam',
        loss='huber',  # Huber loss for robust control accuracy
        metrics=['mae']  # Mean absolute error for monitoring
    )
    
    return model

def load_and_preprocess_data(csv_files, input_cols, output_cols):
    """
    Load and preprocess CSV data for training
    
    Args:
        csv_files: List of CSV file paths
        input_cols: List of input column names
        output_cols: List of output column names
    
    Returns:
        X_train, X_val, y_train, y_val: Training and validation data
    """
    print("Loading and preprocessing data...")
    
    all_data = []
    
    for file_path in csv_files:
        print(f"Loading {file_path}...")
        try:
            # Load CSV file
            df = pd.read_csv(file_path)
            
            # Check if required columns exist
            missing_cols = [col for col in input_cols + output_cols if col not in df.columns]
            if missing_cols:
                print(f"Warning: Missing columns in {file_path}: {missing_cols}")
                continue
            
            # Extract input and output data
            X = df[input_cols].values
            y = df[output_cols].values
            
            # Remove rows with NaN values
            valid_mask = ~(np.isnan(X).any(axis=1) | np.isnan(y).any(axis=1))
            X = X[valid_mask]
            y = y[valid_mask]
            
            if len(X) > 0:
                all_data.append((X, y))
                print(f"  Loaded {len(X)} samples from {file_path}")
            else:
                print(f"  No valid data in {file_path}")
                
        except Exception as e:
            print(f"Error loading {file_path}: {e}")
            continue
    
    if not all_data:
        raise ValueError("No valid data found in any CSV files")
    
    # Combine all data
    X_combined = np.vstack([X for X, y in all_data])
    y_combined = np.vstack([y for X, y in all_data])

    X_combined = np.multiply(X_combined, [8,8,0.1,0.1,8,8])
    y_combined = np.multiply(y_combined, [4,4,4])

    print(f"Total samples loaded: {len(X_combined)}")
    print(f"Input shape: {X_combined.shape}")
    print(f"Output shape: {y_combined.shape}")
    
    # CSV data is already in Q15 format, just clamp to ensure valid range
    X_combined = np.clip(X_combined, Q15_MIN, Q15_MAX)
    y_combined = np.clip(y_combined, Q15_MIN, Q15_MAX)
        
    # Split into train and validation
    split_idx = int(0.8 * len(X_combined))
    X_train_q15 = X_combined[:split_idx]
    y_train_q15 = y_combined[:split_idx]
    X_val_q15 = X_combined[split_idx:]
    y_val_q15 = y_combined[split_idx:]
    
    # Convert Q15 data to float for training
    X_train = array_q15_to_float(X_train_q15)
    y_train = array_q15_to_float(y_train_q15)
    X_val = array_q15_to_float(X_val_q15)
    y_val = array_q15_to_float(y_val_q15)
    
    return X_train, X_val, y_train, y_val

def train_model(model, X_train, y_train, X_val, y_val, epochs=50):
    """Train the model with Q15 data"""
    print("\n" + "=" * 60)
    print("TRAINING THE MODEL")
    print("=" * 60)
    
    # Train the model
    history = model.fit(
        X_train, y_train,
        validation_data=(X_val, y_val),
        epochs=epochs,
        batch_size=32,
        verbose=1
    )
    
    return history

def plot_training_history(history):
    """Plot training history"""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4))
    
    # Plot loss
    ax1.plot(history.history['loss'], label='Training Loss')
    ax1.plot(history.history['val_loss'], label='Validation Loss')
    ax1.set_title('Model Loss')
    ax1.set_xlabel('Epoch')
    ax1.set_ylabel('Loss')
    ax1.legend()
    ax1.grid(True)
    
    # Plot MAE
    ax2.plot(history.history['mae'], label='Training MAE')
    ax2.plot(history.history['val_mae'], label='Validation MAE')
    ax2.set_title('Model MAE')
    ax2.set_xlabel('Epoch')
    ax2.set_ylabel('MAE')
    ax2.legend()
    ax2.grid(True)
    
    plt.tight_layout()
    plt.show()

def test_model(model, X_val, y_val, input_cols, output_cols):
    """Test the trained model"""
    print("\n" + "=" * 60)
    print("MODEL TESTING")
    print("=" * 60)
    
    # Make predictions
    predictions_float = model.predict(X_val)
    
    # Convert to Q15 format
    predictions_q15 = array_float_to_q15(predictions_float)
    y_val_q15 = array_float_to_q15(y_val)
    
    # Calculate metrics
    mse = np.mean((predictions_float - y_val) ** 2)
    mae = np.mean(np.abs(predictions_float - y_val))
    
    print(f"Test MSE: {mse:.6f}")
    print(f"Test MAE: {mae:.6f}")
    
    # Show sample predictions
    print("\nSample Predictions:")
    print("Input format:", input_cols)
    print("Output format:", output_cols)
    print()
    
    for i in range(5):
        print(f"Sample {i+1}:")
        print(f"  Input (Q15): {array_float_to_q15(X_val[i])}")
        print(f"  Input (float): {X_val[i]}")
        print(f"  Predicted (Q15): {predictions_q15[i]}")
        print(f"  Predicted (float): {predictions_float[i]}")
        print(f"  Actual (Q15): {y_val_q15[i]}")
        print(f"  Actual (float): {y_val[i]}")
        print()
    
    # Q15 output statistics and bounds verification
    print("Q15 Output Statistics:")
    print(f"Q15 Min output value: {np.min(predictions_q15)}")
    print(f"Q15 Max output value: {np.max(predictions_q15)}")
    print(f"Q15 Mean output value: {np.mean(predictions_q15):.0f}")
    print(f"Q15 Std output value: {np.std(predictions_q15):.0f}")
    print(f"Float Min output value: {np.min(predictions_float):.3f}")
    print(f"Float Max output value: {np.max(predictions_float):.3f}")
    print(f"Float Mean output value: {np.mean(predictions_float):.3f}")
    print(f"Float Std output value: {np.std(predictions_float):.3f}")
    print(f"All float outputs bounded [-1, 1]: {np.all((predictions_float >= -1) & (predictions_float <= 1))}")
    print(f"All Q15 outputs in range: {np.all((predictions_q15 >= Q15_MIN) & (predictions_q15 <= Q15_MAX))}")

def main():
    """Main function to train the neural network on CSV data"""
    
    # Define input and output columns
    input_cols = ['error_x', 'error_y', 'integral_x', 'integral_y', 'derivative_x_2', 'derivative_y_2']
    output_cols = ['platform_a', 'platform_b', 'platform_c']
    
    print("Q15 Neural Network Training on CSV Data")
    print("=" * 60)
    print(f"Input columns: {input_cols}")
    print(f"Output columns: {output_cols}")
    
    # Find all CSV files in the processed directory
    csv_files = glob.glob("../processed/human??_with_error.csv")
    print(f"\nFound {len(csv_files)} CSV files:")
    for file in csv_files:
        print(f"  {os.path.basename(file)}")
    
    if not csv_files:
        print("No CSV files found!")
        return
    
    # Load and preprocess data
    X_train, X_val, y_train, y_val = load_and_preprocess_data(csv_files, input_cols, output_cols)
    
    # Create the model
    print("\nCreating neural network...")
    model = create_neural_network()
    
    # Print model summary
    print("\n" + "=" * 60)
    print("NEURAL NETWORK ARCHITECTURE")
    print("=" * 60)
    model.summary()
    
    # Train the model
    history = train_model(model, X_train, y_train, X_val, y_val, epochs=30)
    
    # Plot training history
    plot_training_history(history)
    
    # Test the model
    test_model(model, X_val, y_val, input_cols, output_cols)
    
    # Save the model in native Keras format
    model_path = "q15_control_model_trained.keras"
    model.save(model_path)
    print(f"\nModel saved to: {model_path}")
    
    print("\nTraining completed successfully!")
    print("The model is ready for deployment in your embedded control system.")

if __name__ == "__main__":
    main() 