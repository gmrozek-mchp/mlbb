// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_nn.h"

#include <math.h>
#include <string.h>

#include "arm_math_types.h"
#include "platform/platform.h"
#include "command/command.h"

// Include the generated neural network weights
#include "balance_nn_weights.h"

// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

// Neural network input/output indices for ball balancing control
#define NN_INPUT_ERROR_X 0
#define NN_INPUT_ERROR_SUM_X 1
#define NN_INPUT_ERROR_DELTA_X 2
#define NN_INPUT_ERROR_Y 3
#define NN_INPUT_ERROR_SUM_Y 4
#define NN_INPUT_ERROR_DELTA_Y 5

#define NN_OUTPUT_PLATFORM_A 0
#define NN_OUTPUT_PLATFORM_B 1
#define NN_OUTPUT_PLATFORM_C 2

// Error history parameters
#define NN_ERROR_HISTORY_SIZE (10)
#define NN_ERROR_DELTA_FILTER_SIZE  (5)

// Integral anti-windup parameters
#define NN_NEAR_TARGET_THRESHOLD (512)  // Q15 units - ball is "near" target
#define NN_SLOW_MOVEMENT_THRESHOLD (5) // Q15 units - ball is moving "slow"


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

typedef struct {
    float error_history_x[NN_ERROR_HISTORY_SIZE];  // Previous error values for historical inputs
    float error_history_y[NN_ERROR_HISTORY_SIZE];
    float error_sum_x;
    float error_sum_y;
    uint8_t error_history_index;
} balance_nn_state_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************

static balance_nn_state_t nn_state;

// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void nn_prepare_inputs(q15_t target_x, q15_t target_y, q15_t ball_x, q15_t ball_y, float* inputs);


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_NN_TestCommand(void);


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_NN_Initialize( void )
{
    BALANCE_NN_Reset();

    CMD_RegisterCommand("nntest", BALANCE_NN_TestCommand);
}

void BALANCE_NN_Reset( void )
{
    // Reset neural network state
    memset(&nn_state, 0, sizeof(nn_state));
    
    // Reset error history
    nn_state.error_history_index = 0;
    for (int i = 0; i < NN_ERROR_HISTORY_SIZE; i++) {
        nn_state.error_history_x[i] = 0;
        nn_state.error_history_y[i] = 0;
    }
}

void BALANCE_NN_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    if ( !ball_detected ) {
        // No ball detected, set platforms to neutral
        PLATFORM_Position_XY_Set(0, 0 );
        return;
    }
    
    // Prepare neural network inputs (convert to float)
    float nn_inputs[NN_INPUT_SIZE];
    nn_prepare_inputs(target_x, target_y, ball_x, ball_y, nn_inputs);
    
    // Run neural network forward pass (float)
    float nn_outputs[NN_OUTPUT_SIZE];
    nn_forward(nn_inputs, nn_outputs);
    
    // Apply output to platform X control
    float platform_a = nn_outputs[NN_OUTPUT_PLATFORM_A];
    float platform_b = nn_outputs[NN_OUTPUT_PLATFORM_B];
    float platform_c = nn_outputs[NN_OUTPUT_PLATFORM_C];

    if( platform_a > Q15_MAX )
    {
        platform_a = Q15_MAX;
    }
    else if( platform_a < Q15_MIN )
    {
        platform_a = Q15_MIN;
    }

    if( platform_b > Q15_MAX )
    {
        platform_b = Q15_MAX;
    }
    else if( platform_b < Q15_MIN )
    {
        platform_b = Q15_MIN;
    }

    if( platform_c > Q15_MAX )
    {
        platform_c = Q15_MAX;
    }
    else if( platform_c < Q15_MIN )
    {
        platform_c = Q15_MIN;
    }

    PLATFORM_Position_ABC_Set( (q15_t)platform_a, (q15_t)platform_b, (q15_t)platform_c );
}

void BALANCE_NN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    // TODO: Implement data visualization
}

// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void nn_prepare_inputs(q15_t target_x, q15_t target_y, q15_t ball_x, q15_t ball_y, float* inputs)
{
    // Calculate current error
    float error_x = (float)target_x - (float)ball_x;
    float error_y = (float)target_y - (float)ball_y;
    
    // Calculate error delta (derivative-like term)
    int idx_prev = (nn_state.error_history_index - NN_ERROR_DELTA_FILTER_SIZE + NN_ERROR_HISTORY_SIZE) % NN_ERROR_HISTORY_SIZE;
    float error_prev_x = nn_state.error_history_x[idx_prev];
    float error_prev_y = nn_state.error_history_y[idx_prev];

    float error_delta_x = error_x - error_prev_x;
    float error_delta_y = error_y - error_prev_y;

    // Integral anti-windup logic: only update integral when ball is near target and moving slow
    bool near_target = (fabs(error_x) < NN_NEAR_TARGET_THRESHOLD) && (fabs(error_y) < NN_NEAR_TARGET_THRESHOLD);
    bool moving_slow = (fabs(error_delta_x) < NN_SLOW_MOVEMENT_THRESHOLD) && (fabs(error_delta_y) < NN_SLOW_MOVEMENT_THRESHOLD);
    bool integral_enabled = near_target && moving_slow;
    
    // calculate the integral term (only update when conditions are met)
    if( integral_enabled )
    {
        nn_state.error_sum_x += error_x;
        nn_state.error_sum_y += error_y;
    }
    
    nn_state.error_history_x[nn_state.error_history_index] = error_x;
    nn_state.error_history_y[nn_state.error_history_index] = error_y;
    nn_state.error_history_index = (nn_state.error_history_index + 1) % NN_ERROR_HISTORY_SIZE;
    
    // Prepare neural network inputs for ball balancing control
    inputs[NN_INPUT_ERROR_X] = error_x;
    inputs[NN_INPUT_ERROR_SUM_X] = nn_state.error_sum_x;
    inputs[NN_INPUT_ERROR_DELTA_X] = error_delta_x;
    inputs[NN_INPUT_ERROR_Y] = error_y;
    inputs[NN_INPUT_ERROR_SUM_Y] = nn_state.error_sum_y;
    inputs[NN_INPUT_ERROR_DELTA_Y] = error_delta_y;
}


// ******************************************************************
// Section: Neural Network Implementation
// ******************************************************************

// Neural network implementation is now in balance_nn_weights.c
// This includes nn_forward() and nn_relu() functions


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_NN_TestCommand(void)
{
    // Test neural network with known inputs
    CMD_PrintString("=== Neural Network Test ===\r\n", true);
    
    // Test case 1: Small errors with historical values
    float test_inputs_1[NN_INPUT_SIZE] = {
        10.0f,   // error_x
        100.0f,   // error_sum_x
        0.0f,  // error_delta_x
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
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_1[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintFloat(test_outputs_1[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    // Test case 2: Large errors with historical values
    float test_inputs_2[NN_INPUT_SIZE] = {
        1000.0f,    // error_x
        0.0f,   // error_sum_x
        50.0f,    // error_delta_x
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
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_2[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintFloat(test_outputs_2[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    // Test case 3: Zero inputs
    float test_inputs_3[NN_INPUT_SIZE] = {0.0f, 0.0f, 0.0f};
    float test_outputs_3[NN_OUTPUT_SIZE];
    nn_forward(test_inputs_3, test_outputs_3);
    
    CMD_PrintString("Test 3 - Zero inputs:\r\n", true);
    CMD_PrintString("  Input (float): [0.0000, 0.0000, 0.0000]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_3[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintFloat(test_outputs_3[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    // Test case 4: Maximum values
    float test_inputs_4[NN_INPUT_SIZE] = {
        -1000.0f,    // error_x
        -1000.0f,   // error_sum_x
        -30.0f,    // error_delta_x
    };
    
    float test_outputs_4[NN_OUTPUT_SIZE];
    nn_forward(test_inputs_4, test_outputs_4);
    
    CMD_PrintString("Test 4 - Negative values:\r\n", true);
    CMD_PrintString("  Input (float): [", true);
    CMD_PrintFloat(test_inputs_4[0], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_4[1], 4, true);
    CMD_PrintString(", ", true);
    CMD_PrintFloat(test_inputs_4[2], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (float): [", true);
    CMD_PrintFloat(test_outputs_4[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("  Output (Q15): [", true);
    CMD_PrintFloat(test_outputs_4[0], 4, true);
    CMD_PrintString("]\r\n", true);
    
    CMD_PrintString("=== Test Complete ===\r\n", true);
}
