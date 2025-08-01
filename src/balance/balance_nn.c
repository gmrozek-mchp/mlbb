// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_nn.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "ball/ball.h"
#include "platform/platform.h"
#include "command/command.h"

// Include the generated neural network weights
#include "balance_nn_weights.h"

// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

// Q15 format constants
#define Q15_SCALE 32767.0f
#define Q15_MAX 32767
#define Q15_MIN -32768

// Neural network input/output indices
#define NN_INPUT_ERROR_X 0
#define NN_INPUT_ERROR_Y 1
#define NN_INPUT_ERROR_X_PREV2 2
#define NN_INPUT_ERROR_Y_PREV2 3
#define NN_INPUT_ERROR_X_PREV4 4
#define NN_INPUT_ERROR_Y_PREV4 5

#define NN_OUTPUT_PLATFORM_A 0
#define NN_OUTPUT_PLATFORM_B 1
#define NN_OUTPUT_PLATFORM_C 2

// Note: Integral thresholds removed as they are no longer used

// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

typedef struct {
    float error_x_prev[10];  // Previous error values for historical inputs
    float error_y_prev[10];
    uint8_t error_index;
    bool initialized;
} balance_nn_state_t;

// ******************************************************************
// Section: Private Variables
// ******************************************************************

static balance_nn_state_t nn_state;

// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void nn_calculate_derivative_4(float* derivative_x, float* derivative_y);
static void nn_update_integral(float error_x, float error_y);
static void nn_prepare_inputs(q15_t target_x, q15_t target_y, q15_t ball_x, q15_t ball_y, float* inputs);
static q15_t float_to_q15(float value);
static float q15_to_float(q15_t value);

// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_NN_TestCommand(void);

// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_NN_Initialize( void )
{
    // Initialize neural network state
    memset(&nn_state, 0, sizeof(nn_state));
    nn_state.initialized = true;
    
    // Initialize error history
    nn_state.error_index = 0;
    for (int i = 0; i < 10; i++) {
        nn_state.error_x_prev[i] = 0;
        nn_state.error_y_prev[i] = 0;
    }

    CMD_RegisterCommand("nntest", BALANCE_NN_TestCommand);
}

void BALANCE_NN_Reset( void )
{
    // Reset neural network state
    memset(&nn_state, 0, sizeof(nn_state));
    nn_state.initialized = true;
    
    // Reset error history
    nn_state.error_index = 0;
    for (int i = 0; i < 10; i++) {
        nn_state.error_x_prev[i] = 0;
        nn_state.error_y_prev[i] = 0;
    }
}

void BALANCE_NN_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    if (!ball_detected || !nn_state.initialized) {
        // No ball detected or not initialized, set platforms to neutral
        PLATFORM_Position_ABC_Set(0, 0, 0);
        return;
    }
    
    // Prepare neural network inputs (convert to float)
    float nn_inputs[NN_INPUT_SIZE];
    nn_prepare_inputs(target_x, target_y, ball_x, ball_y, nn_inputs);
    
    // Run neural network forward pass (float)
    float nn_outputs[NN_OUTPUT_SIZE];
    nn_forward(nn_inputs, nn_outputs);
    
    // Convert outputs back to Q15 and apply to platforms
    q15_t platform_a = float_to_q15(nn_outputs[NN_OUTPUT_PLATFORM_A]);
    q15_t platform_b = float_to_q15(nn_outputs[NN_OUTPUT_PLATFORM_B]);
    q15_t platform_c = float_to_q15(nn_outputs[NN_OUTPUT_PLATFORM_C]);
    
    PLATFORM_Position_ABC_Set(platform_a, platform_b, platform_c);
}

void BALANCE_NN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    if (!ball_detected) {
        return;
    }
    
    // Calculate current errors (convert to float)
    float error_x = q15_to_float(target_x) - q15_to_float(ball_x);
    float error_y = q15_to_float(target_y) - q15_to_float(ball_y);
    
    // Update error history for neural network inputs
    nn_state.error_x_prev[nn_state.error_index] = error_x;
    nn_state.error_y_prev[nn_state.error_index] = error_y;
    nn_state.error_index = (nn_state.error_index + 1) % 10;
    
    // Note: Data visualization removed for now - can be implemented later
    // with proper command interface functions
}

// ******************************************************************
// Section: Private Functions
// ******************************************************************

// Note: Integral and derivative functions removed as they are no longer used
// The neural network now uses historical error values directly

static void nn_prepare_inputs(q15_t target_x, q15_t target_y, q15_t ball_x, q15_t ball_y, float* inputs)
{
    // Calculate current errors (convert from Q15 to float)
    float error_x = q15_to_float(target_x) - q15_to_float(ball_x);
    float error_y = q15_to_float(target_y) - q15_to_float(ball_y);
    
    // Get historical error values
    int idx_prev2 = (nn_state.error_index - 2 + 10) % 10;
    int idx_prev4 = (nn_state.error_index - 4 + 10) % 10;
    
    float error_x_prev2 = (idx_prev2 >= 0) ? nn_state.error_x_prev[idx_prev2] : 0.0f;
    float error_y_prev2 = (idx_prev2 >= 0) ? nn_state.error_y_prev[idx_prev2] : 0.0f;
    float error_x_prev4 = (idx_prev4 >= 0) ? nn_state.error_x_prev[idx_prev4] : 0.0f;
    float error_y_prev4 = (idx_prev4 >= 0) ? nn_state.error_y_prev[idx_prev4] : 0.0f;
    
    // Prepare neural network inputs (all in float)
    inputs[NN_INPUT_ERROR_X] = error_x;
    inputs[NN_INPUT_ERROR_Y] = error_y;
    inputs[NN_INPUT_ERROR_X_PREV2] = error_x_prev2;
    inputs[NN_INPUT_ERROR_Y_PREV2] = error_y_prev2;
    inputs[NN_INPUT_ERROR_X_PREV4] = error_x_prev4;
    inputs[NN_INPUT_ERROR_Y_PREV4] = error_y_prev4;
}

static q15_t float_to_q15(float value)
{
    // Convert float to Q15 format with clamping
    int32_t q15_value = (int32_t)(value * Q15_SCALE);
    
    // Clamp to Q15 range
    if (q15_value > Q15_MAX) q15_value = Q15_MAX;
    if (q15_value < Q15_MIN) q15_value = Q15_MIN;
    
    return (q15_t)q15_value;
}

static float q15_to_float(q15_t value)
{
    // Convert Q15 format to float
    return (float)value / Q15_SCALE;
}

// ******************************************************************
// Section: Neural Network Implementation
// ******************************************************************

float nn_relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

// Matrix-vector multiplication with floating point arithmetic
static void nn_matmul_float(const float* weights, const float* input, float* output, uint32_t rows, uint32_t cols) {
    for (uint32_t i = 0; i < rows; i++) {
        float sum = 0.0f;
        for (uint32_t j = 0; j < cols; j++) {
            sum += weights[i * cols + j] * input[j];
        }
        output[i] = sum;
    }
}

void nn_forward(const float* input, float* output) {
    float input_processed[NN_INPUT_SIZE];
    float hidden[NN_HIDDEN_SIZE];

    // Layer 0: Input processing (dense_input layer) - 6x6
    for (int i = 0; i < NN_INPUT_SIZE; i++) {
        float sum = 0.0f;
        for (int j = 0; j < NN_INPUT_SIZE; j++) {
            sum += INPUT_WEIGHTS[j * NN_INPUT_SIZE + i] * input[j];  // Transposed weights
        }
        input_processed[i] = sum + INPUT_BIAS[i];
    }

    // Layer 1: Processed Input to Hidden (6x24)
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        float sum = 0.0f;
        for (int j = 0; j < NN_INPUT_SIZE; j++) {
            sum += HIDDEN_WEIGHTS[j * NN_HIDDEN_SIZE + i] * input_processed[j];  // Transposed weights
        }
        hidden[i] = sum + HIDDEN_BIAS[i];  // Linear activation (no ReLU)
    }

    // Layer 2: Hidden to Output (24x3)
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sum = 0.0f;
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            sum += OUTPUT_WEIGHTS[j * NN_OUTPUT_SIZE + i] * hidden[j];  // Transposed weights
        }
        output[i] = sum + OUTPUT_BIAS[i];
    }
}

// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_NN_TestCommand(void)
{
    // Test neural network with known inputs
    CMD_PrintString("=== Neural Network Test ===\r\n", true);
    
    // Test case 1: Small errors with historical values
    float test_inputs_1[NN_INPUT_SIZE] = {
        0.01f,   // error_x
        0.02f,   // error_y
        0.008f,  // error_x_prev2
        0.015f,  // error_y_prev2
        0.005f,  // error_x_prev4
        0.012f   // error_y_prev4
    };
    
    float test_outputs_1[NN_OUTPUT_SIZE];
    nn_forward(test_inputs_1, test_outputs_1);
    
    CMD_PrintString("Test 1 - Small errors:\r\n", true);
    CMD_PrintString("  Input (float): [", true);
    CMD_PrintFloat(test_inputs_1[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_1[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_1[2], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_1[3], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_1[4], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_1[5], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_1[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_1[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_1[2], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_1[0]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_1[1]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_1[2]), false, 0, true);
    CMD_PrintString("]\r\n", true);
    
    // Test case 2: Large errors with historical values
    float test_inputs_2[NN_INPUT_SIZE] = {
        0.5f,    // error_x
        -0.3f,   // error_y
        0.4f,    // error_x_prev2
        -0.25f,  // error_y_prev2
        0.3f,    // error_x_prev4
        -0.2f    // error_y_prev4
    };
    
    float test_outputs_2[NN_OUTPUT_SIZE];
    nn_forward(test_inputs_2, test_outputs_2);
    
    CMD_PrintString("Test 2 - Large errors:\r\n", true);
    CMD_PrintString("  Input (float): [", true);
    CMD_PrintFloat(test_inputs_2[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_2[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_2[2], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_2[3], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_2[4], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_2[5], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_2[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_2[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_2[2], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_2[0]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_2[1]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_2[2]), false, 0, true);
    CMD_PrintString("]\r\n", true);
    
    // Test case 3: Zero inputs
    float test_inputs_3[NN_INPUT_SIZE] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float test_outputs_3[NN_OUTPUT_SIZE];
    nn_forward(test_inputs_3, test_outputs_3);
    
    CMD_PrintString("Test 3 - Zero inputs:\r\n", true);
    CMD_PrintString("  Input (float): [0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_3[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_3[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_3[2], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_3[0]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_3[1]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_3[2]), false, 0, true);
    CMD_PrintString("]\r\n", true);
    
    // Test case 4: Maximum values
    float test_inputs_4[NN_INPUT_SIZE] = {
        1.0f,    // error_x
        1.0f,    // error_y
        0.8f,    // error_x_prev2
        0.9f,    // error_y_prev2
        0.6f,    // error_x_prev4
        0.7f     // error_y_prev4
    };
    
    float test_outputs_4[NN_OUTPUT_SIZE];
    nn_forward(test_inputs_4, test_outputs_4);
    
    CMD_PrintString("Test 4 - Maximum values:\r\n", true);
    CMD_PrintString("  Input (float): [", true);
    CMD_PrintFloat(test_inputs_4[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_4[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_4[2], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_4[3], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_4[4], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_4[5], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_4[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_4[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_outputs_4[2], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_4[0]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_4[1]), false, 0, true);
    CMD_PrintString(", ", true);
    CMD_PrintDecimal_S32(float_to_q15(test_outputs_4[2]), false, 0, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("=== Test Complete ===\r\n", true);
}
