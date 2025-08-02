#!/usr/bin/env python3
"""
Script to convert trained Keras model to C code for embedded deployment.
Extracts weights and biases from the trained neural network and generates C code.
"""

import numpy as np
import tensorflow as tf
from tensorflow import keras
import os

def convert_model_to_c(model_path, output_file):
    """
    Convert trained Keras model to C code
    
    Args:
        model_path: Path to the trained .keras model file
        output_file: Path to output C header file
    """
    
    print(f"Loading model from {model_path}...")
    model = keras.models.load_model(model_path)
    
    print("Model architecture:")
    model.summary()
    
    # Extract weights and biases
    weights = []
    biases = []
    layer_info = []
    
    for i, layer in enumerate(model.layers):
        if hasattr(layer, 'weights') and layer.weights:
            layer_weights = layer.weights[0].numpy()
            layer_bias = layer.weights[1].numpy()
            
            print(f"Layer: {layer.name}")
            print(f"  Input size: {layer_weights.shape[0]}")
            print(f"  Output size: {layer_weights.shape[1]}")
            print(f"  Activation: {layer.activation.__name__ if hasattr(layer, 'activation') else 'linear'}")
            print(f"  Weights shape: {layer_weights.shape}")
            print(f"  Bias shape: {layer_bias.shape}")
            
            layer_info.append({
                'name': layer.name,
                'input_size': layer_weights.shape[0],
                'output_size': layer_weights.shape[1],
                'activation': layer.activation.__name__ if hasattr(layer, 'activation') else 'linear',
                'weights': layer_weights.flatten(),
                'bias': layer_bias.flatten()
            })
    
    # Generate C header file
    generate_c_header(output_file, layer_info)
    
    print(f"C code generated and saved to {output_file}")
    print("Conversion completed!")
    print("Header file: nn_weights.h")
    print("Implementation file: nn_weights.c")
    print("The neural network is ready for embedded deployment!")

def generate_c_header(output_file, layer_info):
    """Generate C header file with neural network weights and architecture"""
    
    with open(output_file, 'w') as f:
        f.write("// ******************************************************************\n")
        f.write("// Neural Network Weights and Biases\n")
        f.write("// Generated from trained Keras model\n")
        f.write("// ******************************************************************\n\n")
        
        f.write("#ifndef NN_WEIGHTS_H\n")
        f.write("#define NN_WEIGHTS_H\n\n")
        
        f.write("#include <stdint.h>\n")
        f.write("#include <arm_math_types.h>\n\n")
        
        # Network architecture - handle the actual model structure
        f.write("// Network Architecture\n")
        f.write(f"#define NN_INPUT_SIZE {layer_info[0]['input_size']}\n")
        # Handle different model architectures
        if len(layer_info) == 5:  # 3-layer model (4->4->24->12->8->2)
            f.write(f"#define NN_INPUT_OUTPUT_SIZE {layer_info[0]['output_size']}\n")
            f.write(f"#define NN_HIDDEN1_SIZE {layer_info[1]['output_size']}\n")
            f.write(f"#define NN_HIDDEN2_SIZE {layer_info[2]['output_size']}\n")
            f.write(f"#define NN_HIDDEN3_SIZE {layer_info[3]['output_size']}\n")
            f.write(f"#define NN_OUTPUT_SIZE {layer_info[4]['output_size']}\n\n")
        elif len(layer_info) == 4:  # 2-layer model (4->4->12->6->2)
            f.write(f"#define NN_INPUT_OUTPUT_SIZE {layer_info[0]['output_size']}\n")
            f.write(f"#define NN_HIDDEN1_SIZE {layer_info[1]['output_size']}\n")
            f.write(f"#define NN_HIDDEN2_SIZE {layer_info[2]['output_size']}\n")
            f.write(f"#define NN_OUTPUT_SIZE {layer_info[3]['output_size']}\n\n")
        else:
            raise ValueError(f"Unsupported model architecture with {len(layer_info)} layers")
        
        # Generate weights and biases for each layer
        for i, layer in enumerate(layer_info):
            layer_name = layer['name'].replace('dense_', '').replace('_', '_').upper()
            
            # Weights
            f.write(f"// {layer['name']} weights ({layer['input_size']}x{layer['output_size']})\n")
            f.write(f"static const float {layer_name}_WEIGHTS[] = {{\n")
            
            weights = layer['weights']
            for j, weight in enumerate(weights):
                f.write(f"    {weight:.8f}f")
                if j < len(weights) - 1:
                    f.write(",")
                if (j + 1) % 4 == 0:
                    f.write("\n")
            f.write("\n};\n\n")
            
            # Bias
            f.write(f"// {layer['name']} bias ({layer['output_size']})\n")
            f.write(f"static const float {layer_name}_BIAS[] = {{\n")
            
            bias = layer['bias']
            for j, b in enumerate(bias):
                f.write(f"    {b:.8f}f")
                if j < len(bias) - 1:
                    f.write(",")
                if (j + 1) % 4 == 0:
                    f.write("\n")
            f.write("\n};\n\n")
        
        # Neural network functions
        f.write("// Neural Network Functions\n")
        f.write("void nn_forward(const float* input, float* output);\n")
        f.write("float nn_relu(float x);\n\n")
        
        f.write("#endif // NN_WEIGHTS_H\n")

def generate_c_implementation(output_file):
    """Generate C implementation file with neural network forward pass"""
    
    impl_file = output_file.replace('.h', '.c')
    
    with open(impl_file, 'w') as f:
        f.write("// ******************************************************************\n")
        f.write("// Neural Network Implementation\n")
        f.write("// Generated from trained Keras model\n")
        f.write("// ******************************************************************\n\n")
        
        f.write('#include "nn_weights.h"\n')
        f.write("#include <math.h>\n\n")
        
        f.write("float nn_relu(float x) {\n")
        f.write("    return (x > 0.0f) ? x : 0.0f;\n")
        f.write("}\n\n")
        
        f.write("// Matrix-vector multiplication with floating point arithmetic\n")
        f.write("// Keras uses: output = input @ weights + bias\n")
        f.write("static void nn_matmul_float(const float* weights, const float* input, float* output, uint32_t rows, uint32_t cols) {\n")
        f.write("    for (uint32_t i = 0; i < rows; i++) {\n")
        f.write("        float sum = 0.0f;\n")
        f.write("        for (uint32_t j = 0; j < cols; j++) {\n")
        f.write("            sum += input[j] * weights[j * rows + i];  // Transposed weights for input @ weights\n")
        f.write("        }\n")
        f.write("        output[i] = sum;\n")
        f.write("    }\n")
        f.write("}\n\n")
        
        f.write("void nn_forward(const float* input, float* output) {\n")
        f.write("    float input_output[NN_INPUT_OUTPUT_SIZE];\n")
        f.write("    float hidden1[NN_HIDDEN1_SIZE];\n")
        f.write("    float hidden2[NN_HIDDEN2_SIZE];\n")
        f.write("    float temp_output[NN_HIDDEN1_SIZE];\n\n")
        
        f.write("    // Layer 1: Input to Input_Output (linear activation)\n")
        f.write("    nn_matmul_float(INPUT_WEIGHTS, input, input_output, NN_INPUT_OUTPUT_SIZE, NN_INPUT_SIZE);\n")
        f.write("    for (int i = 0; i < NN_INPUT_OUTPUT_SIZE; i++) {\n")
        f.write("        input_output[i] = input_output[i] + INPUT_BIAS[i];\n")
        f.write("    }\n\n")
        
        f.write("    // Layer 2: Input_Output to Hidden1 (relu activation)\n")
        f.write("    nn_matmul_float(HIDDEN1_WEIGHTS, input_output, temp_output, NN_HIDDEN1_SIZE, NN_INPUT_OUTPUT_SIZE);\n")
        f.write("    for (int i = 0; i < NN_HIDDEN1_SIZE; i++) {\n")
        f.write("        hidden1[i] = nn_relu(temp_output[i] + HIDDEN1_BIAS[i]);\n")
        f.write("    }\n\n")
        
        f.write("    // Layer 3: Hidden1 to Hidden2 (relu activation)\n")
        f.write("    nn_matmul_float(HIDDEN2_WEIGHTS, hidden1, temp_output, NN_HIDDEN2_SIZE, NN_HIDDEN1_SIZE);\n")
        f.write("    for (int i = 0; i < NN_HIDDEN2_SIZE; i++) {\n")
        f.write("        hidden2[i] = nn_relu(temp_output[i] + HIDDEN2_BIAS[i]);\n")
        f.write("    }\n\n")
        
        f.write("    // Layer 4: Hidden2 to Output (linear activation)\n")
        f.write("    nn_matmul_float(OUTPUT_WEIGHTS, hidden2, output, NN_OUTPUT_SIZE, NN_HIDDEN2_SIZE);\n")
        f.write("    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {\n")
        f.write("        output[i] = output[i] + OUTPUT_BIAS[i];\n")
        f.write("    }\n")
        f.write("}\n")

if __name__ == "__main__":
    print("Converting trained neural network to C code...")
    
    # Check if model file exists
    model_file = "ball_balance_control_model.keras"
    if not os.path.exists(model_file):
        print(f"Error: Model file {model_file} not found!")
        print("Please train the model first using train_q15_control.py")
        exit(1)
    
    # Convert model to C code
    convert_model_to_c(model_file, "nn_weights.h")
    generate_c_implementation("nn_weights.h") 