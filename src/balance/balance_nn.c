// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_nn.h"

#include <stdlib.h>

#include "ball/ball.h"
#include "platform/platform.h"
#include "command/command.h"

// CMSIS-NN includes
#include "arm_nnfunctions.h"
#include "arm_nn_types.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

// Neural network configuration
#define NN_INPUT_SIZE          6   // x_error, y_error, x_integral, y_integral, x_derivative, y_derivative
#define NN_HIDDEN1_SIZE        12  // First hidden layer size
#define NN_HIDDEN2_SIZE        12  // Second hidden layer size
#define NN_OUTPUT_SIZE         3   // platform_a, platform_b, platform_c
#define NN_BUFFER_SIZE         512 // Buffer for intermediate calculations (increased for 2 hidden layers)

// Quantization parameters (integer math)
#define INPUT_SCALE_SHIFT      10      // 2^10 = 1024, equivalent to 1/1024 scaling
#define OUTPUT_SCALE_SHIFT     7       // 2^7 = 128, equivalent to 128x scaling
#define WEIGHT_SCALE_SHIFT     7       // 2^7 = 128, equivalent to 1/128 scaling

// Activation function parameters
#define ACTIVATION_MIN         -32768
#define ACTIVATION_MAX         32767


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

typedef struct {
    // Input data
    int16_t input_data[NN_INPUT_SIZE];
    
    // Hidden layers
    int16_t hidden1_output[NN_HIDDEN1_SIZE];
    int16_t hidden2_output[NN_HIDDEN2_SIZE];
    
    // Output layer
    int16_t output_data[NN_OUTPUT_SIZE];
    
    // Working buffer for CMSIS-NN
    int16_t buffer[NN_BUFFER_SIZE];
    
    // Network state
    bool initialized;
    int16_t error_history_x[5];  // Store last 5 error values for derivative
    int16_t error_history_y[5];  // Store last 5 error values for derivative
    int16_t error_history_index; // Index for circular buffer
    int32_t integral_x;
    int32_t integral_y;
    
} balance_nn_state_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************

static balance_nn_state_t nn_state;

// Neural network weights and biases (trained offline)
// These would typically be loaded from flash memory
static const int8_t fc1_weights[NN_INPUT_SIZE * NN_HIDDEN1_SIZE] = {
    // Weights for input to first hidden layer (6x12)
    // This is a placeholder - actual weights would be trained offline
    10, -5, 8, -3, 12, -7,  // Hidden1 neuron 1
    15, -10, 12, -8, 18, -12,  // Hidden1 neuron 2
    8, -12, 6, -10, 14, -16,  // Hidden1 neuron 3
    12, -8, 10, -6, 16, -10,  // Hidden1 neuron 4
    6, -14, 4, -12, 10, -18,  // Hidden1 neuron 5
    14, -6, 12, -4, 18, -8,  // Hidden1 neuron 6
    10, -10, 8, -8, 12, -12,  // Hidden1 neuron 7
    16, -4, 14, -2, 20, -6,  // Hidden1 neuron 8
    4, -16, 2, -14, 8, -20,  // Hidden1 neuron 9
    18, -2, 16, 0, 22, -4,  // Hidden1 neuron 10
    8, -12, 6, -10, 12, -16,  // Hidden1 neuron 11
    12, -8, 10, -6, 16, -10   // Hidden1 neuron 12
};

static const int64_t fc1_bias[NN_HIDDEN1_SIZE] = {
    // Bias values for first hidden layer
    100, -50, 80, -30, 120, -70, 60, -40, 150, -100, 120, -80
};

static const int8_t fc2_weights[NN_HIDDEN1_SIZE * NN_HIDDEN2_SIZE] = {
    // Weights for first hidden to second hidden layer (12x12)
    // This is a placeholder - actual weights would be trained offline
    // Hidden2 neuron 1 weights
    20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0, -2,
    // Hidden2 neuron 2 weights
    -15, -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8,
    // Hidden2 neuron 3 weights
    10, 8, 6, 4, 2, 0, -2, -4, -6, -8, -10, -12,
    // Hidden2 neuron 4 weights
    -5, -3, -1, 1, 3, 5, 7, 9, 11, 13, 15, 17,
    // Hidden2 neuron 5 weights
    15, 13, 11, 9, 7, 5, 3, 1, -1, -3, -5, -7,
    // Hidden2 neuron 6 weights
    -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12,
    // Hidden2 neuron 7 weights
    5, 3, 1, -1, -3, -5, -7, -9, -11, -13, -15, -17,
    // Hidden2 neuron 8 weights
    -20, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0, 2,
    // Hidden2 neuron 9 weights
    12, 10, 8, 6, 4, 2, 0, -2, -4, -6, -8, -10,
    // Hidden2 neuron 10 weights
    -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14,
    // Hidden2 neuron 11 weights
    18, 16, 14, 12, 10, 8, 6, 4, 2, 0, -2, -4,
    // Hidden2 neuron 12 weights
    -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10
};

static const int64_t fc2_bias[NN_HIDDEN2_SIZE] = {
    // Bias values for second hidden layer
    50, -30, 40, -20, 60, -40, 30, -50, 45, -35, 55, -25
};

static const int8_t fc3_weights[NN_HIDDEN2_SIZE * NN_OUTPUT_SIZE] = {
    // Weights for second hidden to output layer (12x3)
    // This is a placeholder - actual weights would be trained offline
    // Output neuron 1 (platform_a) weights
    25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30,
    // Output neuron 2 (platform_b) weights
    -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35,
    // Output neuron 3 (platform_c) weights
    10, 8, 6, 4, 2, 0, -2, -4, -6, -8, -10, -12
};

static const int64_t fc3_bias[NN_OUTPUT_SIZE] = {
    // Bias values for output layer
    0, 0, 0
};

// Quantization parameters
static const cmsis_nn_per_tensor_quant_params fc1_quant_params = {
    .multiplier = 1073741824,  // 2^30
    .shift = 30
};

static const cmsis_nn_per_tensor_quant_params fc2_quant_params = {
    .multiplier = 1073741824,  // 2^30
    .shift = 30
};

static const cmsis_nn_per_tensor_quant_params fc3_quant_params = {
    .multiplier = 1073741824,  // 2^30
    .shift = 30
};

// Network dimensions
static const cmsis_nn_dims input_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_INPUT_SIZE
};

static const cmsis_nn_dims hidden1_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN1_SIZE
};

static const cmsis_nn_dims hidden2_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN2_SIZE
};

static const cmsis_nn_dims output_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_OUTPUT_SIZE
};

static const cmsis_nn_dims fc1_filter_dims = {
    .n = NN_HIDDEN1_SIZE,
    .h = 1,
    .w = 1,
    .c = NN_INPUT_SIZE
};

static const cmsis_nn_dims fc2_filter_dims = {
    .n = NN_HIDDEN2_SIZE,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN1_SIZE
};

static const cmsis_nn_dims fc1_bias_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN1_SIZE
};

static const cmsis_nn_dims fc2_bias_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN2_SIZE
};

static const cmsis_nn_dims fc3_filter_dims = {
    .n = NN_OUTPUT_SIZE,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN2_SIZE
};

static const cmsis_nn_dims fc3_bias_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_OUTPUT_SIZE
};

// Fully connected layer parameters
static const cmsis_nn_fc_params fc1_params = {
    .input_offset = 0,
    .filter_offset = 0,
    .output_offset = 0,
    .activation = {
        .min = ACTIVATION_MIN,
        .max = ACTIVATION_MAX
    }
};

static const cmsis_nn_fc_params fc2_params = {
    .input_offset = 0,
    .filter_offset = 0,
    .output_offset = 0,
    .activation = {
        .min = ACTIVATION_MIN,
        .max = ACTIVATION_MAX
    }
};

static const cmsis_nn_fc_params fc3_params = {
    .input_offset = 0,
    .filter_offset = 0,
    .output_offset = 0,
    .activation = {
        .min = ACTIVATION_MIN,
        .max = ACTIVATION_MAX
    }
};

// CMSIS-NN context
static cmsis_nn_context nn_context;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void prepare_input_data(q15_t target_x, q15_t target_y, q15_t ball_x, q15_t ball_y);
static void apply_activation_function(int16_t *data, int32_t size);
static void denormalize_output(int16_t *output, int16_t *denorm_output);


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_NN_CMD_Print_State( void );
static void BALANCE_NN_CMD_Print_Inputs( void );
static void BALANCE_NN_CMD_Print_Outputs( void );
static void BALANCE_NN_CMD_Test_Network( void );
static void BALANCE_NN_CMD_Reset_Network( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_NN_Initialize( void )
{
    // Initialize neural network state
    memset(&nn_state, 0, sizeof(nn_state));
    
    // Initialize context buffer
    nn_context.buf = nn_state.buffer;
    nn_context.size = sizeof(nn_state.buffer);
    
    // Reset error tracking
    nn_state.error_history_index = 0;
    nn_state.integral_x = 0;
    nn_state.integral_y = 0;
    
    // Initialize error history
    for (int i = 0; i < 5; i++) {
        nn_state.error_history_x[i] = 0;
        nn_state.error_history_y[i] = 0;
    }
    
    nn_state.initialized = true;
    
    // Register command portal functions
    CMD_RegisterCommand( "nn", BALANCE_NN_CMD_Print_State );
    CMD_RegisterCommand( "nni", BALANCE_NN_CMD_Print_Inputs );
    CMD_RegisterCommand( "nno", BALANCE_NN_CMD_Print_Outputs );
    CMD_RegisterCommand( "nnt", BALANCE_NN_CMD_Test_Network );
    CMD_RegisterCommand( "nnr", BALANCE_NN_CMD_Reset_Network );
}

void BALANCE_NN_Reset( void )
{
    // Reset neural network state
    memset(&nn_state, 0, sizeof(nn_state));
    
    // Re-initialize context buffer
    nn_context.buf = nn_state.buffer;
    nn_context.size = sizeof(nn_state.buffer);
    
    // Reset error tracking
    nn_state.error_history_index = 0;
    nn_state.integral_x = 0;
    nn_state.integral_y = 0;
    
    // Initialize error history
    for (int i = 0; i < 5; i++) {
        nn_state.error_history_x[i] = 0;
        nn_state.error_history_y[i] = 0;
    }
    
    nn_state.initialized = true;
}

void BALANCE_NN_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    if (!nn_state.initialized) {
        BALANCE_NN_Initialize();
    }
    
    // Get current ball position
    ball_data_t ball_data = BALL_Position_Get();
    
    if (!ball_data.detected) {
        // No ball detected, don't move platform
        return;
    }
    
    // Prepare input data for neural network
    prepare_input_data(target_x, target_y, ball_data.x, ball_data.y);
    
    // First fully connected layer (input to first hidden)
    arm_fully_connected_s16(
        &nn_context,
        &fc1_params,
        &fc1_quant_params,
        &input_dims,
        nn_state.input_data,
        &fc1_filter_dims,
        (const int8_t*)fc1_weights,
        &fc1_bias_dims,
        fc1_bias,
        &hidden1_dims,
        nn_state.hidden1_output
    );
    
    // Apply activation function to first hidden layer
    apply_activation_function(nn_state.hidden1_output, NN_HIDDEN1_SIZE);
    
    // Second fully connected layer (first hidden to second hidden)
    arm_fully_connected_s16(
        &nn_context,
        &fc2_params,
        &fc2_quant_params,
        &hidden1_dims,
        nn_state.hidden1_output,
        &fc2_filter_dims,
        (const int8_t*)fc2_weights,
        &fc2_bias_dims,
        fc2_bias,
        &hidden2_dims,
        nn_state.hidden2_output
    );
    
    // Apply activation function to second hidden layer
    apply_activation_function(nn_state.hidden2_output, NN_HIDDEN2_SIZE);
    
    // Third fully connected layer (second hidden to output)
    arm_fully_connected_s16(
        &nn_context,
        &fc3_params,
        &fc3_quant_params,
        &hidden2_dims,
        nn_state.hidden2_output,
        &fc3_filter_dims,
        (const int8_t*)fc3_weights,
        &fc3_bias_dims,
        fc3_bias,
        &output_dims,
        nn_state.output_data
    );
    
    // Denormalize output and apply to platform
    int16_t denorm_output[NN_OUTPUT_SIZE];
    denormalize_output(nn_state.output_data, denorm_output);
    
    // Apply platform control (clamp to valid range)
    int16_t platform_a = denorm_output[0];
    int16_t platform_b = denorm_output[1];
    int16_t platform_c = denorm_output[2];
    
    // Clamp to valid platform range (-1500 to 1500)
    if (platform_a < -1500) platform_a = -1500;
    if (platform_a > 1500) platform_a = 1500;
    if (platform_b < -1500) platform_b = -1500;
    if (platform_b > 1500) platform_b = 1500;
    if (platform_c < -1500) platform_c = -1500;
    if (platform_c > 1500) platform_c = 1500;
    
    // Apply platform control using ABC coordinates
    PLATFORM_Position_ABC_Set(platform_a, platform_b, platform_c);
}

void BALANCE_NN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    // This function can be used for debugging and visualization
    // For now, it's empty but can be extended to output debug data
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void prepare_input_data(q15_t target_x, q15_t target_y, q15_t ball_x, q15_t ball_y)
{
    // Calculate current error
    int16_t error_x = target_x - ball_x;
    int16_t error_y = target_y - ball_y;
    
    // Update integral terms
    nn_state.integral_x += error_x;
    nn_state.integral_y += error_y;
    
    // Clamp integral to prevent overflow
    if (nn_state.integral_x > 10000) nn_state.integral_x = 10000;
    if (nn_state.integral_x < -10000) nn_state.integral_x = -10000;
    if (nn_state.integral_y > 10000) nn_state.integral_y = 10000;
    if (nn_state.integral_y < -10000) nn_state.integral_y = -10000;
    
    // Store current error in history
    nn_state.error_history_x[nn_state.error_history_index] = error_x;
    nn_state.error_history_y[nn_state.error_history_index] = error_y;
    
    // Calculate derivative (current error - error from 5th previous iteration)
    int16_t derivative_x = 0;
    int16_t derivative_y = 0;
    
    if (nn_state.error_history_index >= 4) {
        // We have enough history, calculate derivative
        int16_t old_index = (nn_state.error_history_index - 4) % 5;
        derivative_x = error_x - nn_state.error_history_x[old_index];
        derivative_y = error_y - nn_state.error_history_y[old_index];
    }
    
    // Update history index
    nn_state.error_history_index = (nn_state.error_history_index + 1) % 5;
    
    // Prepare input data for neural network
    // Normalize inputs using integer scaling (right shift for division)
    nn_state.input_data[0] = (int16_t)(error_x >> INPUT_SCALE_SHIFT);          // x_error
    nn_state.input_data[1] = (int16_t)(error_y >> INPUT_SCALE_SHIFT);          // y_error
    nn_state.input_data[2] = (int16_t)(nn_state.integral_x >> INPUT_SCALE_SHIFT); // x_integral
    nn_state.input_data[3] = (int16_t)(nn_state.integral_y >> INPUT_SCALE_SHIFT); // y_integral
    nn_state.input_data[4] = (int16_t)(derivative_x >> INPUT_SCALE_SHIFT);     // x_derivative
    nn_state.input_data[5] = (int16_t)(derivative_y >> INPUT_SCALE_SHIFT);     // y_derivative
}

static void apply_activation_function(int16_t *data, int32_t size)
{
    // Apply ReLU activation function
    for (int32_t i = 0; i < size; i++) {
        if (data[i] < 0) {
            data[i] = 0;
        }
    }
}

static void denormalize_output(int16_t *output, int16_t *denorm_output)
{
    // Denormalize output from neural network using integer scaling (left shift for multiplication)
    for (int32_t i = 0; i < NN_OUTPUT_SIZE; i++) {
        denorm_output[i] = (int16_t)(output[i] << OUTPUT_SCALE_SHIFT);
    }
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_NN_CMD_Print_State( void )
{
    CMD_PrintString( "NN State: ", true );
    CMD_PrintString( "Initialized: ", true );
    CMD_PrintHex_U16( (uint16_t)nn_state.initialized, true );
    CMD_PrintString( " Error Index: ", true );
    CMD_PrintHex_U16( (uint16_t)nn_state.error_history_index, true );
    CMD_PrintString( " Integral X: ", true );
    CMD_PrintHex_U32( (uint32_t)nn_state.integral_x, true );
    CMD_PrintString( " Integral Y: ", true );
    CMD_PrintHex_U32( (uint32_t)nn_state.integral_y, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_NN_CMD_Print_Inputs( void )
{
    CMD_PrintString( "NN Inputs: ", true );
    for (int i = 0; i < NN_INPUT_SIZE; i++) {
        CMD_PrintString( "I", true );
        CMD_PrintHex_U16( (uint16_t)i, true );
        CMD_PrintString( ": ", true );
        CMD_PrintHex_U16( (uint16_t)nn_state.input_data[i], true );
        CMD_PrintString( " ", true );
    }
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_NN_CMD_Print_Outputs( void )
{
    CMD_PrintString( "NN Outputs: ", true );
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        CMD_PrintString( "O", true );
        CMD_PrintHex_U16( (uint16_t)i, true );
        CMD_PrintString( ": ", true );
        CMD_PrintHex_U16( (uint16_t)nn_state.output_data[i], true );
        CMD_PrintString( " ", true );
    }
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_NN_CMD_Test_Network( void )
{
    char arg_buffer[10];
    int16_t test_inputs[6] = {100, -50, 200, -100, 10, -5}; // Default test values
    
    // Check if we have command line arguments
    if (CMD_GetArgc() >= 7) {
        // Parse 6 input arguments
        for (int i = 0; i < 6; i++) {
            CMD_GetArgv(i + 1, arg_buffer, sizeof(arg_buffer));
            test_inputs[i] = (int16_t)atoi(arg_buffer);
        }
        CMD_PrintString( "Testing Neural Network with custom inputs...\r\n", true );
    } else {
        CMD_PrintString( "Testing Neural Network with default inputs...\r\n", true );
        CMD_PrintString( "Usage: nnt <x_error> <y_error> <x_integral> <y_integral> <x_derivative> <y_derivative>\r\n", true );
    }
    
    // Set test inputs
    nn_state.input_data[0] = test_inputs[0];  // x_error
    nn_state.input_data[1] = test_inputs[1];  // y_error
    nn_state.input_data[2] = test_inputs[2];  // x_integral
    nn_state.input_data[3] = test_inputs[3];  // y_integral
    nn_state.input_data[4] = test_inputs[4];  // x_derivative
    nn_state.input_data[5] = test_inputs[5];  // y_derivative
    
    // Run neural network inference
    // First fully connected layer
    arm_fully_connected_s16(
        &nn_context,
        &fc1_params,
        &fc1_quant_params,
        &input_dims,
        nn_state.input_data,
        &fc1_filter_dims,
        (const int8_t*)fc1_weights,
        &fc1_bias_dims,
        fc1_bias,
        &hidden1_dims,
        nn_state.hidden1_output
    );
    
    // Apply activation function to first hidden layer
    apply_activation_function(nn_state.hidden1_output, NN_HIDDEN1_SIZE);
    
    // Second fully connected layer
    arm_fully_connected_s16(
        &nn_context,
        &fc2_params,
        &fc2_quant_params,
        &hidden1_dims,
        nn_state.hidden1_output,
        &fc2_filter_dims,
        (const int8_t*)fc2_weights,
        &fc2_bias_dims,
        fc2_bias,
        &hidden2_dims,
        nn_state.hidden2_output
    );
    
    // Apply activation function to second hidden layer
    apply_activation_function(nn_state.hidden2_output, NN_HIDDEN2_SIZE);
    
    // Third fully connected layer
    arm_fully_connected_s16(
        &nn_context,
        &fc3_params,
        &fc3_quant_params,
        &hidden2_dims,
        nn_state.hidden2_output,
        &fc3_filter_dims,
        (const int8_t*)fc3_weights,
        &fc3_bias_dims,
        fc3_bias,
        &output_dims,
        nn_state.output_data
    );
    
    // Denormalize output
    int16_t denorm_output[NN_OUTPUT_SIZE];
    denormalize_output(nn_state.output_data, denorm_output);
    
    CMD_PrintString( "Test Results - Platform ABC: ", true );
    CMD_PrintHex_U16( (uint16_t)denorm_output[0], true );
    CMD_PrintString( " ", true );
    CMD_PrintHex_U16( (uint16_t)denorm_output[1], true );
    CMD_PrintString( " ", true );
    CMD_PrintHex_U16( (uint16_t)denorm_output[2], true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_NN_CMD_Reset_Network( void )
{
    CMD_PrintString( "Resetting Neural Network...\r\n", true );
    BALANCE_NN_Reset();
    CMD_PrintString( "Neural Network Reset Complete\r\n", true );
}
