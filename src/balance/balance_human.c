// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_human.h"

#include <stdint.h>
#include <stdlib.h>

#include "arm_math_types.h"

#include "command/command.h"

#include "nunchuk/nunchuk.h"
#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

// Full joystick range
#define JOYSTICK_LINEAR_RANGE   (25)    // Range around the center where the joystick is linear with minimum gain
#define JOYSTICK_FULL_RANGE     (100)   // Full joystick range

// Scaling factor for softened joystick
#define JOYSTICK_GAIN_MIN       (20)
#define JOYSTICK_GAIN_MAX       (75)

#define BALANCE_HUMAN_DELTA_FILTER_SIZE_DEFAULT  (5)

#define BALANCE_HUMAN_HISTORY_DEPTH (10)

// Integral anti-windup parameters
#define BALANCE_HUMAN_NEAR_TARGET_THRESHOLD (512)  // Q15 units - ball is "near" target
#define BALANCE_HUMAN_SLOW_MOVEMENT_THRESHOLD (5) // Q15 units - ball is moving "slow"


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

typedef struct
{
    size_t delta_filter_size;

    q31_t error_history[BALANCE_HUMAN_HISTORY_DEPTH];
    size_t error_history_index;

    // Integral anti-windup parameters
    q15_t target;           /**< Current target position */
    q15_t actual;           /**< Current actual position */

    bool integral_enabled;   /**< Whether integral term should be updated */

    q31_t error;
    q31_t error_sum;
    q31_t error_delta;
    
    q15_t output;
} human_q15_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************

static human_q15_t human_x;
static human_q15_t human_y;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void BALANCE_HUMAN_Initialize_Instance( human_q15_t *human );
static void BALANCE_HUMAN_Reset_Instance( human_q15_t *human );
static void BALANCE_HUMAN_Run_Instance( human_q15_t *human, q15_t target, q15_t actual );

static q15_t BALANCE_HUMAN_SoftenJoystickValue(int16_t raw_value);


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_HUMAN_CMD_Print_State( void );
static void BALANCE_HUMAN_CMD_Print_Constants( void );
static void BALANCE_HUMAN_CMD_Set_DeltaFilterSize( void );
static void BALANCE_HUMAN_CMD_Print_IntegralStatus( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_HUMAN_Initialize( void )
{
    BALANCE_HUMAN_Initialize_Instance( &human_x );
    BALANCE_HUMAN_Initialize_Instance( &human_y );

    CMD_RegisterCommand( "human", BALANCE_HUMAN_CMD_Print_State );
    CMD_RegisterCommand( "humank", BALANCE_HUMAN_CMD_Print_Constants );
    CMD_RegisterCommand( "humandfs", BALANCE_HUMAN_CMD_Set_DeltaFilterSize );
    CMD_RegisterCommand( "humani", BALANCE_HUMAN_CMD_Print_IntegralStatus );
}

void BALANCE_HUMAN_Reset( void )
{
    BALANCE_HUMAN_Reset_Instance( &human_x );
    BALANCE_HUMAN_Reset_Instance( &human_y );

    NUNCHUK_Zero_Set();
}

void BALANCE_HUMAN_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    if( ball_detected )
    {
        BALANCE_HUMAN_Run_Instance( &human_x, target_x, ball_x );
        BALANCE_HUMAN_Run_Instance( &human_y, target_y, ball_y );
    }

    nunchuk_data_t nunchukData = NUNCHUK_Data_Get();

    human_x.output = BALANCE_HUMAN_SoftenJoystickValue(nunchukData.joystick_x);
    human_y.output = BALANCE_HUMAN_SoftenJoystickValue(nunchukData.joystick_y);

    PLATFORM_Position_XY_Set( human_x.output, human_y.output );
}

void BALANCE_HUMAN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    static uint8_t dv_data[42];

    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    dv_data[0] = 0x03;

    dv_data[1] = (uint8_t)'H';

    dv_data[2] = (uint8_t)ball_detected;

    dv_data[3] = (uint8_t)target_x;
    dv_data[4] = (uint8_t)(target_x >> 8);
    dv_data[5] = (uint8_t)target_y;
    dv_data[6] = (uint8_t)(target_y >> 8);

    dv_data[7] = (uint8_t)ball_x;
    dv_data[8] = (uint8_t)(ball_x >> 8);
    dv_data[9] = (uint8_t)ball_y;
    dv_data[10] = (uint8_t)(ball_y >> 8);

    dv_data[11] = (uint8_t)human_x.error;
    dv_data[12] = (uint8_t)(human_x.error >> 8);
    dv_data[13] = (uint8_t)human_y.error;
    dv_data[14] = (uint8_t)(human_y.error >> 8);

    dv_data[15] = (uint8_t)human_x.error_sum;
    dv_data[16] = (uint8_t)(human_x.error_sum >> 8);
    dv_data[17] = (uint8_t)(human_x.error_sum >> 16);
    dv_data[18] = (uint8_t)(human_x.error_sum >> 24);
    dv_data[19] = (uint8_t)human_y.error_sum;
    dv_data[20] = (uint8_t)(human_y.error_sum >> 8);
    dv_data[21] = (uint8_t)(human_y.error_sum >> 16);
    dv_data[22] = (uint8_t)(human_y.error_sum >> 24);

    dv_data[23] = (uint8_t)human_x.error_delta;
    dv_data[24] = (uint8_t)(human_x.error_delta >> 8);
    dv_data[25] = (uint8_t)human_y.error_delta;
    dv_data[26] = (uint8_t)(human_y.error_delta >> 8);

    dv_data[27] = (uint8_t)human_x.output;
    dv_data[28] = (uint8_t)(human_x.output >> 8);
    dv_data[29] = (uint8_t)((q31_t)human_x.output >> 16);
    dv_data[30] = (uint8_t)((q31_t)human_x.output >> 24);
    dv_data[31] = (uint8_t)human_y.output;
    dv_data[32] = (uint8_t)(human_y.output >> 8);
    dv_data[33] = (uint8_t)((q31_t)human_y.output >> 16);
    dv_data[34] = (uint8_t)((q31_t)human_y.output >> 24);

    dv_data[35] = (uint8_t)platform_abc.a;
    dv_data[36] = (uint8_t)(platform_abc.a >> 8);
    dv_data[37] = (uint8_t)platform_abc.b;
    dv_data[38] = (uint8_t)(platform_abc.b >> 8);
    dv_data[39] = (uint8_t)platform_abc.c;
    dv_data[40] = (uint8_t)(platform_abc.c >> 8);

    dv_data[41] = ~0x03;

    CMD_PrintByteArray( dv_data, sizeof(dv_data), false );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void BALANCE_HUMAN_Initialize_Instance( human_q15_t *human )
{
    human->delta_filter_size = BALANCE_HUMAN_DELTA_FILTER_SIZE_DEFAULT;
    BALANCE_HUMAN_Reset_Instance( human );
}

static void BALANCE_HUMAN_Reset_Instance( human_q15_t *human )
{
    human->error_history_index = 0;
    human->error_sum = 0;
    human->target = 0;
    human->actual = 0;
    human->integral_enabled = false;
    human->output = 0;

    for( size_t i = 0; i < BALANCE_HUMAN_HISTORY_DEPTH; i++ )
    {
        human->error_history[i] = 0;
    }
}

static void BALANCE_HUMAN_Run_Instance( human_q15_t *human, q15_t target, q15_t actual )
{
    int16_t error_delta_index;

    // Update target and actual positions for integral anti-windup logic
    human->target = target;
    human->actual = actual;

    // calculate the error
    human->error = ((q31_t)human->target - human->actual);

    // calculate the index to use for error delta calculation
    error_delta_index = human->error_history_index - human->delta_filter_size;
    if( error_delta_index < 0 )
    {
        error_delta_index += BALANCE_HUMAN_HISTORY_DEPTH;
    }

    // calculate the error delta
    human->error_delta = human->error - human->error_history[error_delta_index];
    // Integral anti-windup logic: only update integral when ball is near target and moving slow
    bool near_target = (abs((int32_t)human->error) < BALANCE_HUMAN_NEAR_TARGET_THRESHOLD);
    bool moving_slow = (abs((int32_t)human->error_delta) < BALANCE_HUMAN_SLOW_MOVEMENT_THRESHOLD);
    human->integral_enabled = near_target && moving_slow;
    
    // calculate the integral term (only update when conditions are met)
    if( human->integral_enabled )
    {
        human->error_sum += human->error;
    }

    // update the error history
    human->error_history[human->error_history_index] = human->error;
    human->error_history_index++;

    // wrap the error history index
    if( human->error_history_index >= BALANCE_HUMAN_HISTORY_DEPTH )
    {
        human->error_history_index = 0;
    }
}

/**
 * @brief Softens joystick input around the center by applying non-linear mapping
 * @param raw_value Raw joystick value (-128 to 127)
 * @return Softened joystick value scaled for platform control
 */
 static q15_t BALANCE_HUMAN_SoftenJoystickValue(int16_t raw_value)
 {
     int16_t abs_value;
     int16_t sign;
     int16_t gain;
     
     // Handle sign and get absolute value
     if (raw_value < 0)
     {
         sign = -1;
         abs_value = -raw_value;
     }
     else
     {
         sign = 1;
         abs_value = raw_value;
     }
     
     gain = JOYSTICK_GAIN_MIN;
     if( abs_value > JOYSTICK_LINEAR_RANGE )
     {
         gain += (abs_value - JOYSTICK_LINEAR_RANGE) * (JOYSTICK_GAIN_MAX - JOYSTICK_GAIN_MIN) / (JOYSTICK_FULL_RANGE - JOYSTICK_LINEAR_RANGE);
     }
 
     // Apply sign and gain for platform
     return (q15_t)(sign * abs_value * gain);
 }
 
 
// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_HUMAN_CMD_Print_State( void )
{
    q15_t ex = human_x.error_history[human_x.error_history_index];
    q15_t ey = human_y.error_history[human_y.error_history_index];
    platform_xy_t platform_xy = PLATFORM_Position_XY_Get();

    CMD_PrintString( "ex: ", true );
    CMD_PrintHex_U16( (uint16_t)ex, true );
    CMD_PrintString( " ey: ", true );
    CMD_PrintHex_U16( (uint16_t)ey, true );
    CMD_PrintString( " px: ", true );
    CMD_PrintHex_U16( (uint16_t)platform_xy.x, true );
    CMD_PrintString( " py: ", true );
    CMD_PrintHex_U16( (uint16_t)platform_xy.y, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_HUMAN_CMD_Print_Constants( void )
{
    CMD_PrintString( " D Filter Size: 0x", true );
    CMD_PrintHex_U16( (uint16_t)human_x.delta_filter_size, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_HUMAN_CMD_Set_DeltaFilterSize( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        size_t delta_filter_size;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        delta_filter_size = (size_t)atoi( arg_buffer );

        if( delta_filter_size > BALANCE_HUMAN_HISTORY_DEPTH )
        {
            delta_filter_size = BALANCE_HUMAN_HISTORY_DEPTH;
        }

        human_x.delta_filter_size = delta_filter_size;
        human_y.delta_filter_size = delta_filter_size;

        BALANCE_HUMAN_Reset();
    }

    BALANCE_HUMAN_CMD_Print_Constants();
}

static void BALANCE_HUMAN_CMD_Print_IntegralStatus( void )
{
    CMD_PrintString( "X - Target: ", true );
    CMD_PrintHex_U16( (uint16_t)human_x.target, true );
    CMD_PrintString( " Actual: ", true );
    CMD_PrintHex_U16( (uint16_t)human_x.actual, true );
    CMD_PrintString( " Integral: ", true );
    CMD_PrintString( human_x.integral_enabled ? "ON" : "OFF", true );
    CMD_PrintString( "\r\n", true );
    
    CMD_PrintString( "Y - Target: ", true );
    CMD_PrintHex_U16( (uint16_t)human_y.target, true );
    CMD_PrintString( " Actual: ", true );
    CMD_PrintHex_U16( (uint16_t)human_y.actual, true );
    CMD_PrintString( " Integral: ", true );
    CMD_PrintString( human_y.integral_enabled ? "ON" : "OFF", true );
    CMD_PrintString( "\r\n", true );
}
