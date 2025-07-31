#!/usr/bin/env python3
"""
Extract weights from C file and compare with Keras
"""

import re
import numpy as np

def extract_c_weights():
    """Extract weights from the C header file"""
    
    with open('balance_nn_weights.h', 'r') as f:
        content = f.read()
    
    # Extract INPUT_WEIGHTS
    input_weights_match = re.search(r'static const float INPUT_WEIGHTS\[\] = \{(.*?)\};', content, re.DOTALL)
    if input_weights_match:
        input_weights_str = input_weights_match.group(1)
        input_weights = [float(x.strip().replace('f', '')) for x in input_weights_str.split(',') if x.strip()]
        input_weights = np.array(input_weights).reshape(6, 6)
        print("INPUT_WEIGHTS shape:", input_weights.shape)
        print("INPUT_WEIGHTS:", input_weights.flatten())
    else:
        print("Could not find INPUT_WEIGHTS")
        return
    
    # Extract INPUT_BIAS
    input_bias_match = re.search(r'static const float INPUT_BIAS\[\] = \{(.*?)\};', content, re.DOTALL)
    if input_bias_match:
        input_bias_str = input_bias_match.group(1)
        input_bias = [float(x.strip().replace('f', '')) for x in input_bias_str.split(',') if x.strip()]
        input_bias = np.array(input_bias)
        print("\nINPUT_BIAS shape:", input_bias.shape)
        print("INPUT_BIAS:", input_bias)
    else:
        print("Could not find INPUT_BIAS")
        return
    
    # Extract HIDDEN_1_WEIGHTS
    hidden1_weights_match = re.search(r'static const float HIDDEN_1_WEIGHTS\[\] = \{(.*?)\};', content, re.DOTALL)
    if hidden1_weights_match:
        hidden1_weights_str = hidden1_weights_match.group(1)
        hidden1_weights = [float(x.strip().replace('f', '')) for x in hidden1_weights_str.split(',') if x.strip()]
        hidden1_weights = np.array(hidden1_weights).reshape(6, 12)
        print("\nHIDDEN_1_WEIGHTS shape:", hidden1_weights.shape)
        print("HIDDEN_1_WEIGHTS:", hidden1_weights.flatten())
    else:
        print("Could not find HIDDEN_1_WEIGHTS")
        return
    
    # Extract HIDDEN_1_BIAS
    hidden1_bias_match = re.search(r'static const float HIDDEN_1_BIAS\[\] = \{(.*?)\};', content, re.DOTALL)
    if hidden1_bias_match:
        hidden1_bias_str = hidden1_bias_match.group(1)
        hidden1_bias = [float(x.strip().replace('f', '')) for x in hidden1_bias_str.split(',') if x.strip()]
        hidden1_bias = np.array(hidden1_bias)
        print("\nHIDDEN_1_BIAS shape:", hidden1_bias.shape)
        print("HIDDEN_1_BIAS:", hidden1_bias)
    else:
        print("Could not find HIDDEN_1_BIAS")
        return
    
    return input_weights, input_bias, hidden1_weights, hidden1_bias

def compare_with_keras():
    """Compare C weights with Keras weights"""
    
    # Get C weights
    c_weights = extract_c_weights()
    if c_weights is None:
        return
    
    c_input_weights, c_input_bias, c_hidden1_weights, c_hidden1_bias = c_weights
    
    # Get Keras weights
    import tensorflow as tf
    from tensorflow import keras
    
    model = keras.models.load_model('ball_balance_control_model.keras')
    
    keras_input_weights = model.layers[0].get_weights()[0]  # (6, 6)
    keras_input_bias = model.layers[0].get_weights()[1]     # (6,)
    keras_hidden1_weights = model.layers[1].get_weights()[0]  # (6, 12)
    keras_hidden1_bias = model.layers[1].get_weights()[1]     # (12,)
    
    print("\n=== Weight Comparison ===")
    print("INPUT_WEIGHTS difference:", np.abs(c_input_weights - keras_input_weights).max())
    print("INPUT_BIAS difference:", np.abs(c_input_bias - keras_input_bias).max())
    print("HIDDEN_1_WEIGHTS difference:", np.abs(c_hidden1_weights - keras_hidden1_weights).max())
    print("HIDDEN_1_BIAS difference:", np.abs(c_hidden1_bias - keras_hidden1_bias).max())
    
    # Test with same input
    test_input = np.array([0.01, 0.02, 0.0, 0.0, 0.001, 0.002])
    
    # Keras computation
    keras_output = model.layers[0](test_input.reshape(1, -1))
    print("\nKeras dense_input output:", keras_output.numpy()[0])
    
    # C-style computation
    c_output = np.dot(c_input_weights.T, test_input) + c_input_bias
    print("C-style dense_input output:", c_output)
    
    print("Difference:", np.abs(keras_output.numpy()[0] - c_output).max())

if __name__ == "__main__":
    compare_with_keras() 