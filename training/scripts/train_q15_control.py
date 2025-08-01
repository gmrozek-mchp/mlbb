import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import matplotlib.pyplot as plt
import glob
import os

def create_neural_network():
    """
    Create a neural network for ball balancing control:
    - 4 inputs: error_x, error_delta_x, error_y, error_delta_y
    - 2 outputs: platform_x, platform_y
    """
    
    model = keras.Sequential([
        # Input layer (4 nodes)
        layers.Dense(4, activation='linear', input_shape=(4,), name='dense_input'),
        
        # First hidden layer (24 nodes)
        layers.Dense(24, activation='relu', name='dense_hidden1'),
        
        # Second hidden layer (12 nodes)
        layers.Dense(12, activation='relu', name='dense_hidden2'),
        
        # Third hidden layer (8 nodes)
        layers.Dense(8, activation='relu', name='dense_hidden3'),

        # Output layer (2 nodes) - platform control signals
        layers.Dense(2, activation='linear', name='dense_output')
    ])
    
    # Compile the model
    model.compile(
        optimizer=tf.keras.optimizers.Nadam(learning_rate=0.005),  # Nadam optimizer - best performer
        loss='mae',  # Mean absolute error - best performing loss function
        metrics=['mae']  # Mean absolute error
    )
    
    return model

def load_and_preprocess_data(csv_files, input_cols, output_cols):
    """
    Load and preprocess CSV data for training
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
            X = df[input_cols].values.astype(np.float32)
            y = df[output_cols].values.astype(np.float32)
            
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

    print(f"Total samples loaded: {len(X_combined)}")
    print(f"Input shape: {X_combined.shape}")
    print(f"Output shape: {y_combined.shape}")
    
    # Print data statistics
    print(f"Input data range: [{X_combined.min():.3f}, {X_combined.max():.3f}]")
    print(f"Output data range: [{y_combined.min():.3f}, {y_combined.max():.3f}]")
    
    # Randomize the data order to improve training
    print("Randomizing data order...")
    indices = np.arange(len(X_combined))
    np.random.shuffle(indices)
    X_combined = X_combined[indices]
    y_combined = y_combined[indices]
    print("Data randomization completed.")
        
    # Split into train and validation
    split_idx = int(0.8 * len(X_combined))
    X_train = X_combined[:split_idx]
    y_train = y_combined[:split_idx]
    X_val = X_combined[split_idx:]
    y_val = y_combined[split_idx:]
    
    return X_train, X_val, y_train, y_val

def train_model(model, X_train, y_train, X_val, y_val, epochs=100):
    """Train the model"""
    print("\n" + "=" * 60)
    print("TRAINING THE MODEL")
    print("=" * 60)
    
    # Create learning rate scheduler
    initial_lr = 0.01  # Moderate initial LR for stable training
    decay_factor = 0.95
    min_lr = 0.0001
    
    def lr_schedule(epoch):
        lr = initial_lr * (decay_factor ** epoch)
        return max(lr, min_lr)
    
    # Train the model
    history = model.fit(
        X_train, y_train,
        validation_data=(X_val, y_val),
        epochs=epochs,
        batch_size=16,  # Smaller batch size for better gradient updates
        verbose=1,
        callbacks=[tf.keras.callbacks.LearningRateScheduler(lr_schedule)]
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
    predictions = model.predict(X_val)
    
    # Calculate metrics
    mse = np.mean((predictions - y_val) ** 2)
    mae = np.mean(np.abs(predictions - y_val))
    
    print(f"Test MSE: {mse:.6f}")
    print(f"Test MAE: {mae:.6f}")
    
    # Show sample predictions
    print("\nSample Predictions:")
    print("Input format:", input_cols)
    print("Output format:", output_cols)
    print()
    
    for i in range(5):
        j=i*500
        print(f"Sample {j+1}:")
        print(f"  Input: {X_val[j]}")
        print(f"  Predicted: {predictions[j]}")
        print(f"  Actual: {y_val[j]}")
        print()

def main():
    """Main function to train the neural network"""
    
    # Define input and output columns
    input_cols = ['error_x', 'error_delta_x', 'error_y', 'error_delta_y']
    output_cols = ['platform_x', 'platform_y']
    
    print("Neural Network Training for Ball Balancing Control")
    print("=" * 60)
    print(f"Input columns: {input_cols}")
    print(f"Output columns: {output_cols}")
    
    # Find all CSV files in the processed directory
    csv_files = glob.glob("../rawdata/human*.csv")
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
    history = train_model(model, X_train, y_train, X_val, y_val, epochs=100)
    
    # Plot training history
    plot_training_history(history)
    
    # Test the model
    test_model(model, X_val, y_val, input_cols, output_cols)
    
    # Save the model
    model_path = "ball_balance_control_model.keras"
    model.save(model_path)
    print(f"\nModel saved to: {model_path}")
    
    print("\nTraining completed successfully!")
    print("The model is ready for deployment in your embedded control system.")

if __name__ == "__main__":
    main() 