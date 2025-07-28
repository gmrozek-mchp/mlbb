#!/usr/bin/env python3
"""
Test script for the neural network implementation in balance_nn.c
This script simulates the C implementation to verify correctness.
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# Neural network configuration (matching C implementation)
NN_INPUT_SIZE = 8
NN_HIDDEN_SIZE = 16
NN_OUTPUT_SIZE = 2
INPUT_SCALE = 0.001
OUTPUT_SCALE = 100.0

# Placeholder weights (matching C implementation)
fc1_weights = np.array([
    [10, -5, 8, -3, 12, -7, 6, -4],      # Hidden neuron 1
    [15, -10, 12, -8, 18, -12, 10, -6],  # Hidden neuron 2
    [8, -12, 6, -10, 14, -16, 8, -12],   # Hidden neuron 3
    [12, -8, 10, -6, 16, -10, 12, -8],   # Hidden neuron 4
    [6, -14, 4, -12, 10, -18, 6, -14],  # Hidden neuron 5
    [14, -6, 12, -4, 18, -8, 14, -6],   # Hidden neuron 6
    [10, -10, 8, -8, 12, -12, 10, -10], # Hidden neuron 7
    [16, -4, 14, -2, 20, -6, 16, -4],   # Hidden neuron 8
    [4, -16, 2, -14, 8, -20, 4, -16],   # Hidden neuron 9
    [18, -2, 16, 0, 22, -4, 18, -2],    # Hidden neuron 10
    [8, -12, 6, -10, 12, -16, 8, -12],  # Hidden neuron 11
    [12, -8, 10, -6, 16, -10, 12, -8],  # Hidden neuron 12
    [6, -14, 4, -12, 10, -18, 6, -14],  # Hidden neuron 13
    [14, -6, 12, -4, 18, -8, 14, -6],   # Hidden neuron 14
    [10, -10, 8, -8, 12, -12, 10, -10], # Hidden neuron 15
    [16, -4, 14, -2, 20, -6, 16, -4]    # Hidden neuron 16
], dtype=np.int8)

fc1_bias = np.array([
    100, -50, 80, -30, 120, -70, 60, -40,
    150, -100, 120, -80, 180, -120, 100, -60
], dtype=np.int64)

fc2_weights = np.array([
    [20, -15],  # Output neuron 1 (platform_x)
    [18, -12],  # Output neuron 1 (platform_x)
    [16, -10],  # Output neuron 1 (platform_x)
    [14, -8],   # Output neuron 1 (platform_x)
    [12, -6],   # Output neuron 1 (platform_x)
    [10, -4],   # Output neuron 1 (platform_x)
    [8, -2],    # Output neuron 1 (platform_x)
    [6, 0],     # Output neuron 1 (platform_x)
    [4, 2],     # Output neuron 1 (platform_x)
    [2, 4],     # Output neuron 1 (platform_x)
    [0, 6],     # Output neuron 1 (platform_x)
    [-2, 8],    # Output neuron 1 (platform_x)
    [-4, 10],   # Output neuron 1 (platform_x)
    [-6, 12],   # Output neuron 1 (platform_x)
    [-8, 14],   # Output neuron 1 (platform_x)
    [-10, 16]   # Output neuron 1 (platform_x)
], dtype=np.int8)

fc2_bias = np.array([0, 0], dtype=np.int64)

def prepare_input_data(ball_x, ball_y, target_x, target_y, integral_x=0, integral_y=0):
    """Prepare input data for neural network (matching C implementation)"""
    error_x = target_x - ball_x
    error_y = target_y - ball_y
    
    # Normalize inputs to 16-bit fixed point format
    input_data = np.array([
        int(ball_x * INPUT_SCALE),      # ball_x
        int(ball_y * INPUT_SCALE),      # ball_y
        int(target_x * INPUT_SCALE),    # target_x
        int(target_y * INPUT_SCALE),    # target_y
        int(error_x * INPUT_SCALE),     # error_x
        int(error_y * INPUT_SCALE),     # error_y
        int(integral_x * INPUT_SCALE),  # integral_x
        int(integral_y * INPUT_SCALE)   # integral_y
    ], dtype=np.int16)
    
    return input_data

def fully_connected_s16(input_data, weights, bias):
    """Simulate CMSIS-NN fully connected layer with s16 quantization"""
    # Matrix multiplication with quantization
    output = np.dot(weights, input_data.astype(np.int32)) + bias
    
    # Clamp to 16-bit range
    output = np.clip(output, -32768, 32767).astype(np.int16)
    
    return output

def apply_relu(data):
    """Apply ReLU activation function"""
    return np.maximum(data, 0).astype(np.int16)

def denormalize_output(output):
    """Denormalize output from neural network"""
    return (output.astype(np.float32) * OUTPUT_SCALE).astype(np.int16)

def run_neural_network(ball_x, ball_y, target_x, target_y, integral_x=0, integral_y=0):
    """Run the complete neural network (matching C implementation)"""
    # Prepare input data
    input_data = prepare_input_data(ball_x, ball_y, target_x, target_y, integral_x, integral_y)
    
    # First fully connected layer (input to hidden)
    hidden_output = fully_connected_s16(input_data, fc1_weights, fc1_bias)
    
    # Apply ReLU activation function
    hidden_output = apply_relu(hidden_output)
    
    # Second fully connected layer (hidden to output)
    output_data = fully_connected_s16(hidden_output, fc2_weights, fc2_bias)
    
    # Denormalize output
    denorm_output = denormalize_output(output_data)
    
    # Clamp to valid platform range (-1500 to 1500)
    platform_x = np.clip(denorm_output[0], -1500, 1500)
    platform_y = np.clip(denorm_output[1], -1500, 1500)
    
    return platform_x, platform_y

def test_neural_network():
    """Test the neural network with sample data"""
    print("Testing Neural Network Implementation")
    print("=" * 40)
    
    # Test cases
    test_cases = [
        (3000, 3000, 3000, 3000, 0, 0),  # Ball at target
        (3100, 2900, 3000, 3000, 0, 0),  # Ball offset from target
        (2800, 3200, 3000, 3000, 0, 0),  # Ball far from target
        (3000, 3000, 3100, 2900, 0, 0),  # Target moved
    ]
    
    for i, (ball_x, ball_y, target_x, target_y, integral_x, integral_y) in enumerate(test_cases):
        platform_x, platform_y = run_neural_network(ball_x, ball_y, target_x, target_y, integral_x, integral_y)
        
        print(f"Test {i+1}:")
        print(f"  Ball: ({ball_x}, {ball_y})")
        print(f"  Target: ({target_x}, {target_y})")
        print(f"  Platform: ({platform_x}, {platform_y})")
        print()

def analyze_training_data():
    """Analyze training data to understand input/output patterns"""
    print("Analyzing Training Data")
    print("=" * 40)
    
    # Load training data
    try:
        df = pd.read_csv('../processed/human01_with_error.csv')
        
        # Extract relevant columns
        ball_x = df['ball_x'].values
        ball_y = df['ball_y'].values
        target_x = df['target_x'].values
        target_y = df['target_y'].values
        platform_x = df['platform_x'].values
        platform_y = df['platform_y'].values
        error_x = df['error_x'].values
        error_y = df['error_y'].values
        integral_x = df['integral_x'].values
        integral_y = df['integral_y'].values
        
        print(f"Dataset size: {len(df)} samples")
        print(f"Ball X range: {ball_x.min()} to {ball_x.max()}")
        print(f"Ball Y range: {ball_y.min()} to {ball_y.max()}")
        print(f"Target X range: {target_x.min()} to {target_x.max()}")
        print(f"Target Y range: {target_y.min()} to {target_y.max()}")
        print(f"Platform X range: {platform_x.min()} to {platform_x.max()}")
        print(f"Platform Y range: {platform_y.min()} to {platform_y.max()}")
        print(f"Error X range: {error_x.min()} to {error_x.max()}")
        print(f"Error Y range: {error_y.min()} to {error_y.max()}")
        
        # Test neural network with real data
        print("\nTesting with real data samples:")
        for i in range(0, min(5, len(df))):
            platform_x_pred, platform_y_pred = run_neural_network(
                ball_x[i], ball_y[i], target_x[i], target_y[i], 
                integral_x[i], integral_y[i]
            )
            
            print(f"Sample {i+1}:")
            print(f"  Actual platform: ({platform_x[i]}, {platform_y[i]})")
            print(f"  Predicted platform: ({platform_x_pred}, {platform_y_pred})")
            print()
            
    except FileNotFoundError:
        print("Training data not found. Skipping analysis.")

def main():
    """Main function"""
    test_neural_network()
    analyze_training_data()

if __name__ == "__main__":
    main() 