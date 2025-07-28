#!/usr/bin/env python3
"""
Simple test script for the neural network implementation in balance_nn.c
This script simulates the C implementation without external dependencies.
"""

# Neural network configuration (matching C implementation)
NN_INPUT_SIZE = 8
NN_HIDDEN_SIZE = 16
NN_OUTPUT_SIZE = 2
INPUT_SCALE = 0.001
OUTPUT_SCALE = 100.0

# Placeholder weights (matching C implementation)
fc1_weights = [
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
]

fc1_bias = [
    100, -50, 80, -30, 120, -70, 60, -40,
    150, -100, 120, -80, 180, -120, 100, -60
]

fc2_weights = [
    [20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0, -2, -4, -6, -8, -10],  # Output neuron 1 (platform_x)
    [-15, -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14, 16]  # Output neuron 2 (platform_y)
]

fc2_bias = [0, 0]

def prepare_input_data(ball_x, ball_y, target_x, target_y, integral_x=0, integral_y=0):
    """Prepare input data for neural network (matching C implementation)"""
    error_x = target_x - ball_x
    error_y = target_y - ball_y
    
    # Normalize inputs to 16-bit fixed point format
    input_data = [
        int(ball_x * INPUT_SCALE),      # ball_x
        int(ball_y * INPUT_SCALE),      # ball_y
        int(target_x * INPUT_SCALE),    # target_x
        int(target_y * INPUT_SCALE),    # target_y
        int(error_x * INPUT_SCALE),     # error_x
        int(error_y * INPUT_SCALE),     # error_y
        int(integral_x * INPUT_SCALE),  # integral_x
        int(integral_y * INPUT_SCALE)   # integral_y
    ]
    
    return input_data

def fully_connected_s16(input_data, weights, bias):
    """Simulate CMSIS-NN fully connected layer with s16 quantization"""
    output = []
    
    for i in range(len(weights)):
        # Matrix multiplication
        sum_val = 0
        for j in range(len(input_data)):
            sum_val += weights[i][j] * input_data[j]
        
        # Add bias
        sum_val += bias[i]
        
        # Clamp to 16-bit range
        if sum_val > 32767:
            sum_val = 32767
        elif sum_val < -32768:
            sum_val = -32768
        
        output.append(sum_val)
    
    return output

def apply_relu(data):
    """Apply ReLU activation function"""
    return [max(0, x) for x in data]

def denormalize_output(output):
    """Denormalize output from neural network"""
    return [int(x * OUTPUT_SCALE) for x in output]

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
    platform_x = max(-1500, min(1500, denorm_output[0]))
    platform_y = max(-1500, min(1500, denorm_output[1]))
    
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

def test_input_preparation():
    """Test input data preparation"""
    print("Testing Input Data Preparation")
    print("=" * 40)
    
    ball_x, ball_y = 3000, 3000
    target_x, target_y = 3100, 2900
    integral_x, integral_y = 100, -50
    
    input_data = prepare_input_data(ball_x, ball_y, target_x, target_y, integral_x, integral_y)
    
    print(f"Ball: ({ball_x}, {ball_y})")
    print(f"Target: ({target_x}, {target_y})")
    print(f"Integral: ({integral_x}, {integral_y})")
    print(f"Input data: {input_data}")
    print()

def main():
    """Main function"""
    test_input_preparation()
    test_neural_network()
    print("Neural network implementation test completed successfully!")

if __name__ == "__main__":
    main() 