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
#define NN_INPUT_ERROR_DELTA_X 1
#define NN_INPUT_ERROR_Y 2
#define NN_INPUT_ERROR_DELTA_Y 3

#define NN_OUTPUT_PLATFORM_X 0
#define NN_OUTPUT_PLATFORM_Y 1

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
    q15_t output_x;
    q15_t output_y;
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
    float platform_x = nn_outputs[NN_OUTPUT_PLATFORM_X];
    float platform_y = nn_outputs[NN_OUTPUT_PLATFORM_Y];

    if( platform_x > Q15_MAX )
    {
        nn_state.output_x = Q15_MAX;
    }
    else if( platform_x < Q15_MIN )
    {
        nn_state.output_x = Q15_MIN;
    }
    else 
    {
        nn_state.output_x = (q15_t)platform_x;
    }

    if( platform_y > Q15_MAX )
    {
        nn_state.output_y = Q15_MAX;
    }
    else if( platform_y < Q15_MIN )
    {
        nn_state.output_y = Q15_MIN;
    }
    else
    {
        nn_state.output_y = (q15_t)platform_y;
    }

    PLATFORM_Position_XY_Set( nn_state.output_x, nn_state.output_y );
}

void BALANCE_NN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    static uint8_t dv_data[22];

    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    dv_data[0] = 0x03;

    dv_data[1] = (uint8_t)'N';

    dv_data[2] = (uint8_t)ball_detected;

    dv_data[3] = (uint8_t)target_x;
    dv_data[4] = (uint8_t)(target_x >> 8);
    dv_data[5] = (uint8_t)target_y;
    dv_data[6] = (uint8_t)(target_y >> 8);

    dv_data[7] = (uint8_t)ball_x;
    dv_data[8] = (uint8_t)(ball_x >> 8);
    dv_data[9] = (uint8_t)ball_y;
    dv_data[10] = (uint8_t)(ball_y >> 8);

    dv_data[11] = (uint8_t)nn_state.output_x;
    dv_data[12] = (uint8_t)(nn_state.output_x >> 8);
    dv_data[13] = (uint8_t)nn_state.output_y;
    dv_data[14] = (uint8_t)(nn_state.output_y >> 8);

    dv_data[15] = (uint8_t)platform_abc.a;
    dv_data[16] = (uint8_t)(platform_abc.a >> 8);
    dv_data[17] = (uint8_t)platform_abc.b;
    dv_data[18] = (uint8_t)(platform_abc.b >> 8);
    dv_data[19] = (uint8_t)platform_abc.c;
    dv_data[20] = (uint8_t)(platform_abc.c >> 8);

    dv_data[21] = ~0x03;

    CMD_PrintByteArray( dv_data, sizeof(dv_data), false );
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
    inputs[NN_INPUT_ERROR_DELTA_X] = error_delta_x;
    inputs[NN_INPUT_ERROR_Y] = error_y;
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
