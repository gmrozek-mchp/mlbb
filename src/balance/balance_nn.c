// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_nn.h"

#include "ball/ball.h"
#include "platform/platform.h"

// CMSIS-NN includes
#include "arm_nnfunctions.h"
#include "arm_nn_types.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

// Neural network configuration
#define NN_INPUT_SIZE          8   // ball_x, ball_y, target_x, target_y, error_x, error_y, integral_x, integral_y
#define NN_HIDDEN_SIZE         16  // Hidden layer size
#define NN_OUTPUT_SIZE         2   // platform_x, platform_y
#define NN_BUFFER_SIZE         256 // Buffer for intermediate calculations

// Quantization parameters (16-bit fixed point)
#define INPUT_SCALE            0.001f  // Scale factor for input normalization
#define OUTPUT_SCALE           100.0f  // Scale factor for output denormalization
#define WEIGHT_SCALE           0.01f   // Scale factor for weights

// Activation function parameters
#define ACTIVATION_MIN         -32768
#define ACTIVATION_MAX         32767


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

typedef struct {
    // Input data
    int16_t input_data[NN_INPUT_SIZE];
    
    // Hidden layer
    int16_t hidden_output[NN_HIDDEN_SIZE];
    
    // Output layer
    int16_t output_data[NN_OUTPUT_SIZE];
    
    // Working buffer for CMSIS-NN
    int16_t buffer[NN_BUFFER_SIZE];
    
    // Network state
    bool initialized;
    int16_t last_error_x;
    int16_t last_error_y;
    int32_t integral_x;
    int32_t integral_y;
    
} balance_nn_state_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************

static balance_nn_state_t nn_state;

// Neural network weights and biases (trained offline)
// These would typically be loaded from flash memory
static const int8_t fc1_weights[NN_INPUT_SIZE * NN_HIDDEN_SIZE] = {
    // Weights for input to hidden layer (8x16)
    // This is a placeholder - actual weights would be trained offline
    10, -5, 8, -3, 12, -7, 6, -4,  // Hidden neuron 1
    15, -10, 12, -8, 18, -12, 10, -6,  // Hidden neuron 2
    8, -12, 6, -10, 14, -16, 8, -12,  // Hidden neuron 3
    12, -8, 10, -6, 16, -10, 12, -8,  // Hidden neuron 4
    6, -14, 4, -12, 10, -18, 6, -14,  // Hidden neuron 5
    14, -6, 12, -4, 18, -8, 14, -6,  // Hidden neuron 6
    10, -10, 8, -8, 12, -12, 10, -10,  // Hidden neuron 7
    16, -4, 14, -2, 20, -6, 16, -4,  // Hidden neuron 8
    4, -16, 2, -14, 8, -20, 4, -16,  // Hidden neuron 9
    18, -2, 16, 0, 22, -4, 18, -2,  // Hidden neuron 10
    8, -12, 6, -10, 12, -16, 8, -12,  // Hidden neuron 11
    12, -8, 10, -6, 16, -10, 12, -8,  // Hidden neuron 12
    6, -14, 4, -12, 10, -18, 6, -14,  // Hidden neuron 13
    14, -6, 12, -4, 18, -8, 14, -6,  // Hidden neuron 14
    10, -10, 8, -8, 12, -12, 10, -10,  // Hidden neuron 15
    16, -4, 14, -2, 20, -6, 16, -4   // Hidden neuron 16
};

static const int64_t fc1_bias[NN_HIDDEN_SIZE] = {
    // Bias values for hidden layer
    100, -50, 80, -30, 120, -70, 60, -40,
    150, -100, 120, -80, 180, -120, 100, -60
};

static const int8_t fc2_weights[NN_HIDDEN_SIZE * NN_OUTPUT_SIZE] = {
    // Weights for hidden to output layer (16x2)
    // This is a placeholder - actual weights would be trained offline
    // Output neuron 1 (platform_x) weights
    20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0, -2, -4, -6, -8, -10,
    // Output neuron 2 (platform_y) weights
    -15, -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14, 16
};

static const int64_t fc2_bias[NN_OUTPUT_SIZE] = {
    // Bias values for output layer
    0, 0
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

// Network dimensions
static const cmsis_nn_dims input_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_INPUT_SIZE
};

static const cmsis_nn_dims hidden_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN_SIZE
};

static const cmsis_nn_dims output_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_OUTPUT_SIZE
};

static const cmsis_nn_dims fc1_filter_dims = {
    .n = NN_HIDDEN_SIZE,
    .h = 1,
    .w = 1,
    .c = NN_INPUT_SIZE
};

static const cmsis_nn_dims fc2_filter_dims = {
    .n = NN_OUTPUT_SIZE,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN_SIZE
};

static const cmsis_nn_dims fc1_bias_dims = {
    .n = 1,
    .h = 1,
    .w = 1,
    .c = NN_HIDDEN_SIZE
};

static const cmsis_nn_dims fc2_bias_dims = {
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

// CMSIS-NN context
static cmsis_nn_context nn_context;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void prepare_input_data(q15_t target_x, q15_t target_y);
static void apply_activation_function(int16_t *data, int32_t size);
static void denormalize_output(int16_t *output, int16_t *denorm_output);


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************


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
    nn_state.last_error_x = 0;
    nn_state.last_error_y = 0;
    nn_state.integral_x = 0;
    nn_state.integral_y = 0;
    
    nn_state.initialized = true;
}

void BALANCE_NN_Reset( void )
{
    // Reset neural network state
    memset(&nn_state, 0, sizeof(nn_state));
    
    // Re-initialize context buffer
    nn_context.buf = nn_state.buffer;
    nn_context.size = sizeof(nn_state.buffer);
    
    // Reset error tracking
    nn_state.last_error_x = 0;
    nn_state.last_error_y = 0;
    nn_state.integral_x = 0;
    nn_state.integral_y = 0;
    
    nn_state.initialized = true;
}

void BALANCE_NN_Run( q15_t target_x, q15_t target_y, q15_t ball_x, q15_t ball_y )
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
    prepare_input_data(target_x, target_y);
    
    // First fully connected layer (input to hidden)
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
        &hidden_dims,
        nn_state.hidden_output
    );
    
    // Apply activation function to hidden layer
    apply_activation_function(nn_state.hidden_output, NN_HIDDEN_SIZE);
    
    // Second fully connected layer (hidden to output)
    arm_fully_connected_s16(
        &nn_context,
        &fc2_params,
        &fc2_quant_params,
        &hidden_dims,
        nn_state.hidden_output,
        &fc2_filter_dims,
        (const int8_t*)fc2_weights,
        &fc2_bias_dims,
        fc2_bias,
        &output_dims,
        nn_state.output_data
    );
    
    // Denormalize output and apply to platform
    int16_t denorm_output[NN_OUTPUT_SIZE];
    denormalize_output(nn_state.output_data, denorm_output);
    
    // Apply platform control (clamp to valid range)
    int16_t platform_x = denorm_output[0];
    int16_t platform_y = denorm_output[1];
    
    // Clamp to valid platform range (-1500 to 1500)
    if (platform_x < -1500) platform_x = -1500;
    if (platform_x > 1500) platform_x = 1500;
    if (platform_y < -1500) platform_y = -1500;
    if (platform_y > 1500) platform_y = 1500;
    
    // Apply platform control
    PLATFORM_Position_XY_Set(platform_x, platform_y);
}

void BALANCE_NN_DataVisualizer( void )
{
    // This function can be used for debugging and visualization
    // For now, it's empty but can be extended to output debug data
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void prepare_input_data(q15_t target_x, q15_t target_y)
{
    // Get current ball position
    ball_data_t ball_data = BALL_Position_Get();
    
    // Calculate current error
    int16_t error_x = target_x - ball_data.x;
    int16_t error_y = target_y - ball_data.y;
    
    // Update integral (simple accumulation)
    nn_state.integral_x += error_x;
    nn_state.integral_y += error_y;
    
    // Clamp integral to prevent overflow
    if (nn_state.integral_x > 10000) nn_state.integral_x = 10000;
    if (nn_state.integral_x < -10000) nn_state.integral_x = -10000;
    if (nn_state.integral_y > 10000) nn_state.integral_y = 10000;
    if (nn_state.integral_y < -10000) nn_state.integral_y = -10000;
    
    // Prepare input data for neural network
    // Normalize inputs to 16-bit fixed point format
    nn_state.input_data[0] = (int16_t)(ball_data.x * INPUT_SCALE);      // ball_x
    nn_state.input_data[1] = (int16_t)(ball_data.y * INPUT_SCALE);      // ball_y
    nn_state.input_data[2] = (int16_t)(target_x * INPUT_SCALE);         // target_x
    nn_state.input_data[3] = (int16_t)(target_y * INPUT_SCALE);         // target_y
    nn_state.input_data[4] = (int16_t)(error_x * INPUT_SCALE);          // error_x
    nn_state.input_data[5] = (int16_t)(error_y * INPUT_SCALE);          // error_y
    nn_state.input_data[6] = (int16_t)(nn_state.integral_x * INPUT_SCALE); // integral_x
    nn_state.input_data[7] = (int16_t)(nn_state.integral_y * INPUT_SCALE); // integral_y
    
    // Update last error for next iteration
    nn_state.last_error_x = error_x;
    nn_state.last_error_y = error_y;
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
    // Denormalize output from neural network
    for (int32_t i = 0; i < NN_OUTPUT_SIZE; i++) {
        denorm_output[i] = (int16_t)(output[i] * OUTPUT_SCALE);
    }
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************
