// ******************************************************************
// Neural Network Implementation
// Generated from trained Keras model for Ball Balancing Control
// ******************************************************************

#include "balance_nn_weights.h"
#include <math.h>

float nn_relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

// Matrix-vector multiplication with floating point arithmetic
// Keras uses: output = input @ weights + bias
static void nn_matmul_float(const float* weights, const float* input, float* output, uint32_t rows, uint32_t cols) {
    for (uint32_t i = 0; i < rows; i++) {
        float sum = 0.0f;
        for (uint32_t j = 0; j < cols; j++) {
            sum += input[j] * weights[j * rows + i];  // Transposed weights for input @ weights
        }
        output[i] = sum;
    }
}

void nn_forward(const float* input, float* output) {
    float input_output[NN_INPUT_OUTPUT_SIZE];
    float hidden1[NN_HIDDEN1_SIZE];
    float hidden2[NN_HIDDEN2_SIZE];
    float hidden3[NN_HIDDEN3_SIZE];
    float temp_output[NN_HIDDEN1_SIZE];

    // Layer 1: Input to Input_Output (linear activation)
    nn_matmul_float(INPUT_WEIGHTS, input, input_output, NN_INPUT_OUTPUT_SIZE, NN_INPUT_SIZE);
    for (int i = 0; i < NN_INPUT_OUTPUT_SIZE; i++) {
        input_output[i] = input_output[i] + INPUT_BIAS[i];
    }

    // Layer 2: Input_Output to Hidden1 (relu activation)
    nn_matmul_float(HIDDEN1_WEIGHTS, input_output, temp_output, NN_HIDDEN1_SIZE, NN_INPUT_OUTPUT_SIZE);
    for (int i = 0; i < NN_HIDDEN1_SIZE; i++) {
        hidden1[i] = nn_relu(temp_output[i] + HIDDEN1_BIAS[i]);
    }

    // Layer 3: Hidden1 to Hidden2 (relu activation)
    nn_matmul_float(HIDDEN2_WEIGHTS, hidden1, temp_output, NN_HIDDEN2_SIZE, NN_HIDDEN1_SIZE);
    for (int i = 0; i < NN_HIDDEN2_SIZE; i++) {
        hidden2[i] = nn_relu(temp_output[i] + HIDDEN2_BIAS[i]);
    }

    // Layer 4: Hidden2 to Hidden3 (relu activation)
    nn_matmul_float(HIDDEN3_WEIGHTS, hidden2, temp_output, NN_HIDDEN3_SIZE, NN_HIDDEN2_SIZE);
    for (int i = 0; i < NN_HIDDEN3_SIZE; i++) {
        hidden3[i] = nn_relu(temp_output[i] + HIDDEN3_BIAS[i]);
    }

    // Layer 5: Hidden3 to Output (linear activation)
    nn_matmul_float(OUTPUT_WEIGHTS, hidden3, output, NN_OUTPUT_SIZE, NN_HIDDEN3_SIZE);
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        output[i] = output[i] + OUTPUT_BIAS[i];
    }
}
