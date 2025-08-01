// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_pid.h"

#include <stdint.h>
#include <stdlib.h>

#include "arm_math_types.h"

#include "command/command.h"

#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

#define BALANCE_PID_CONSTANT_Kp_DEFAULT (600)
#define BALANCE_PID_CONSTANT_Ki_DEFAULT (20)
#define BALANCE_PID_CONSTANT_Kd_DEFAULT (8000)

#define BALANCE_PID_OUTPUT_SCALE_FACTOR_DEFAULT     (256)

#define BALANCE_PID_DELTA_FILTER_SIZE_DEFAULT  (5)

#define BALANCE_PID_HISTORY_DEPTH (10)

// Integral anti-windup parameters
#define BALANCE_PID_NEAR_TARGET_THRESHOLD (512)  // Q15 units - ball is "near" target
#define BALANCE_PID_SLOW_MOVEMENT_THRESHOLD (5) // Q15 units - ball is moving "slow"


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

typedef struct
{
    uint16_t Kp;            /**< The proportional gain. */
    uint16_t Ki;            /**< The integral gain. */
    uint16_t Kd;            /**< The derivative gain. */

    uint16_t output_scale_factor;
    
    size_t delta_filter_size;

    q31_t error_history[BALANCE_PID_HISTORY_DEPTH];
    size_t error_history_index;

    // Integral anti-windup parameters
    q15_t target;           /**< Current target position */
    q15_t actual;           /**< Current actual position */

    bool integral_enabled;   /**< Whether integral term should be updated */

    q31_t error;
    q31_t error_sum;
    q31_t error_delta;

    q31_t p_term;
    q31_t i_term;
    q31_t d_term;

    q31_t output;
} pid_q15_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************

static pid_q15_t pid_x;
static pid_q15_t pid_y;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void BALANCE_PID_Initialize_Instance( pid_q15_t *pid );
static void BALANCE_PID_Reset_Instance( pid_q15_t *pid );
static void BALANCE_PID_Run_Instance( pid_q15_t *pid, q15_t target, q15_t actual );


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_PID_CMD_Print_State( void );
static void BALANCE_PID_CMD_Print_Constants( void );
static void BALANCE_PID_CMD_Set_Kp( void );
static void BALANCE_PID_CMD_Set_Ki( void );
static void BALANCE_PID_CMD_Set_Kd( void );
static void BALANCE_PID_CMD_Set_OutputScaleFactor( void );
static void BALANCE_PID_CMD_Set_DeltaFilterSize( void );
static void BALANCE_PID_CMD_Print_IntegralStatus( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_PID_Initialize( void )
{
    BALANCE_PID_Initialize_Instance( &pid_x );
    BALANCE_PID_Initialize_Instance( &pid_y );

    CMD_RegisterCommand( "pid", BALANCE_PID_CMD_Print_State );
    CMD_RegisterCommand( "pidk", BALANCE_PID_CMD_Print_Constants );
    CMD_RegisterCommand( "kp", BALANCE_PID_CMD_Set_Kp );
    CMD_RegisterCommand( "ki", BALANCE_PID_CMD_Set_Ki );
    CMD_RegisterCommand( "kd", BALANCE_PID_CMD_Set_Kd );
    CMD_RegisterCommand( "osf", BALANCE_PID_CMD_Set_OutputScaleFactor );
    CMD_RegisterCommand( "dfs", BALANCE_PID_CMD_Set_DeltaFilterSize );
    CMD_RegisterCommand( "pidi", BALANCE_PID_CMD_Print_IntegralStatus );
}

void BALANCE_PID_Reset( void )
{
    BALANCE_PID_Reset_Instance( &pid_x );
    BALANCE_PID_Reset_Instance( &pid_y );
}

void BALANCE_PID_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    if( ball_detected )
    {
        BALANCE_PID_Run_Instance( &pid_x, target_x, ball_x );
        BALANCE_PID_Run_Instance( &pid_y, target_y, ball_y );
    }
    else
    {
        BALANCE_PID_Reset();
    }   

    q31_t output_x = pid_x.output;
    q31_t output_y = pid_y.output;

    if( output_x > INT16_MAX )
    {
        output_x = INT16_MAX;
    }
    else if( output_x < INT16_MIN )
    {
        output_x = (q31_t)INT16_MIN;
    }

    if( output_y > INT16_MAX )
    {
        output_y = INT16_MAX;
    }
    else if( output_y < INT16_MIN )
    {
        output_y = (q31_t)INT16_MIN;
    }

    PLATFORM_Position_XY_Set( (q15_t)output_x, (q15_t)output_y );
}

void BALANCE_PID_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    static uint8_t dv_data[42];

    platform_xy_t platform_xy = PLATFORM_Position_XY_Get();
    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    dv_data[0] = 0x03;

    dv_data[1] = (uint8_t)'P';

    dv_data[2] = (uint8_t)ball_detected;

    dv_data[3] = (uint8_t)target_x;
    dv_data[4] = (uint8_t)(target_x >> 8);
    dv_data[5] = (uint8_t)target_y;
    dv_data[6] = (uint8_t)(target_y >> 8);

    dv_data[7] = (uint8_t)ball_x;
    dv_data[8] = (uint8_t)(ball_x >> 8);
    dv_data[9] = (uint8_t)ball_y;
    dv_data[10] = (uint8_t)(ball_y >> 8);

    dv_data[11] = (uint8_t)pid_x.error;
    dv_data[12] = (uint8_t)(pid_x.error >> 8);
    dv_data[13] = (uint8_t)pid_y.error;
    dv_data[14] = (uint8_t)(pid_y.error >> 8);

    dv_data[15] = (uint8_t)pid_x.error_sum;
    dv_data[16] = (uint8_t)(pid_x.error_sum >> 8);
    dv_data[17] = (uint8_t)(pid_x.error_sum >> 16);
    dv_data[18] = (uint8_t)(pid_x.error_sum >> 24);
    dv_data[19] = (uint8_t)pid_y.error_sum;
    dv_data[20] = (uint8_t)(pid_y.error_sum >> 8);
    dv_data[21] = (uint8_t)(pid_y.error_sum >> 16);
    dv_data[22] = (uint8_t)(pid_y.error_sum >> 24);

    dv_data[23] = (uint8_t)pid_x.error_delta;
    dv_data[24] = (uint8_t)(pid_x.error_delta >> 8);
    dv_data[25] = (uint8_t)pid_y.error_delta;
    dv_data[26] = (uint8_t)(pid_y.error_delta >> 8);

    dv_data[27] = (uint8_t)platform_xy.x;
    dv_data[28] = (uint8_t)(platform_xy.x >> 8);
    dv_data[29] = (uint8_t)(platform_xy.x >> 16);
    dv_data[30] = (uint8_t)(platform_xy.x >> 24);
    dv_data[31] = (uint8_t)platform_xy.y;
    dv_data[32] = (uint8_t)(platform_xy.y >> 8);
    dv_data[33] = (uint8_t)(platform_xy.y >> 16);
    dv_data[34] = (uint8_t)(platform_xy.y >> 24);

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

static void BALANCE_PID_Initialize_Instance( pid_q15_t *pid )
{
    pid->Kp = BALANCE_PID_CONSTANT_Kp_DEFAULT;
    pid->Ki = BALANCE_PID_CONSTANT_Ki_DEFAULT;
    pid->Kd = BALANCE_PID_CONSTANT_Kd_DEFAULT;

    pid->output_scale_factor = BALANCE_PID_OUTPUT_SCALE_FACTOR_DEFAULT;
    pid->delta_filter_size = BALANCE_PID_DELTA_FILTER_SIZE_DEFAULT;

    BALANCE_PID_Reset_Instance( pid );
}

static void BALANCE_PID_Reset_Instance( pid_q15_t *pid )
{
    pid->error_history_index = 0;
    pid->error_sum = 0;
    pid->target = 0;
    pid->actual = 0;
    pid->integral_enabled = false;
    pid->output = 0;

    for( size_t i = 0; i < BALANCE_PID_HISTORY_DEPTH; i++ )
    {
        pid->error_history[i] = 0;
    }
}

static void BALANCE_PID_Run_Instance( pid_q15_t *pid, q15_t target, q15_t actual )
{
    int16_t error_delta_index;

    // Update target and actual positions for integral anti-windup logic
    pid->target = target;
    pid->actual = actual;

    // calculate the error
    pid->error = ((q31_t)pid->target - pid->actual);

    // calculate the index to use for error delta calculation
    error_delta_index = pid->error_history_index - pid->delta_filter_size;
    if( error_delta_index < 0 )
    {
        error_delta_index += BALANCE_PID_HISTORY_DEPTH;
    }

    // calculate the error delta
    pid->error_delta = pid->error - pid->error_history[error_delta_index];

    // calculate the proportional term
    pid->p_term = pid->error * pid->Kp;

    // Integral anti-windup logic: only update integral when ball is near target and moving slow
    bool near_target = (abs((int32_t)pid->error) < BALANCE_PID_NEAR_TARGET_THRESHOLD);
    bool moving_slow = (abs((int32_t)pid->error_delta) < BALANCE_PID_SLOW_MOVEMENT_THRESHOLD);
    pid->integral_enabled = near_target && moving_slow;
    
    // calculate the integral term (only update when conditions are met)
    if( pid->integral_enabled )
    {
        pid->error_sum += pid->error;
    }
    pid->i_term = pid->error_sum * pid->Ki;

    // calculate the derivative term
    pid->d_term = pid->error_delta * pid->Kd;

    // update the error history
    pid->error_history[pid->error_history_index] = pid->error;
    pid->error_history_index++;

    // wrap the error history index
    if( pid->error_history_index >= BALANCE_PID_HISTORY_DEPTH )
    {
        pid->error_history_index = 0;
    }

    // calculate the output
    pid->output = (pid->p_term + pid->i_term + pid->d_term) / pid->output_scale_factor;
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_PID_CMD_Print_State( void )
{
    q15_t ex = pid_x.error_history[pid_x.error_history_index];
    q15_t ey = pid_y.error_history[pid_y.error_history_index];
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

static void BALANCE_PID_CMD_Print_Constants( void )
{
    CMD_PrintString( "Kp: 0x", true );
    CMD_PrintHex_U16( (uint16_t)pid_x.Kp, true );
    CMD_PrintString( " Ki: 0x", true );
    CMD_PrintHex_U16( (uint16_t)pid_x.Ki, true );
    CMD_PrintString( " Kd: 0x", true );
    CMD_PrintHex_U16( (uint16_t)pid_x.Kd, true );
    CMD_PrintString( " Output Scale: 0x", true );
    CMD_PrintHex_U16( (uint16_t)pid_x.output_scale_factor, true );
    CMD_PrintString( " D Filter Size: 0x", true );
    CMD_PrintHex_U16( (uint16_t)pid_x.delta_filter_size, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_PID_CMD_Set_Kp( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        uint16_t kp;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        kp = (uint16_t)atoi( arg_buffer );

        pid_x.Kp = kp;
        pid_y.Kp = kp;
    }

    BALANCE_PID_CMD_Print_Constants();
}

static void BALANCE_PID_CMD_Set_Ki( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        uint16_t ki;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        ki = (uint16_t)atoi( arg_buffer );

        pid_x.Ki = ki;
        pid_y.Ki = ki;
    }

    BALANCE_PID_CMD_Print_Constants();
}

static void BALANCE_PID_CMD_Set_Kd( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        uint16_t kd;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        kd = (uint16_t)atoi( arg_buffer );

        pid_x.Kd = kd;
        pid_y.Kd = kd;
    }

    BALANCE_PID_CMD_Print_Constants();
}

static void BALANCE_PID_CMD_Set_OutputScaleFactor( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        uint16_t output_scale_factor;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        output_scale_factor = (uint16_t)atoi( arg_buffer );

        pid_x.output_scale_factor = output_scale_factor;
        pid_y.output_scale_factor = output_scale_factor;
    }

    BALANCE_PID_CMD_Print_Constants();
}

static void BALANCE_PID_CMD_Set_DeltaFilterSize( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        size_t delta_filter_size;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        delta_filter_size = (size_t)atoi( arg_buffer );

        if( delta_filter_size > BALANCE_PID_HISTORY_DEPTH )
        {
            delta_filter_size = BALANCE_PID_HISTORY_DEPTH;
        }

        pid_x.delta_filter_size = delta_filter_size;
        pid_y.delta_filter_size = delta_filter_size;

        BALANCE_PID_Reset();
    }

    BALANCE_PID_CMD_Print_Constants();
}

static void BALANCE_PID_CMD_Print_IntegralStatus( void )
{
    CMD_PrintString( "X - Target: ", true );
    CMD_PrintHex_U16( (uint16_t)pid_x.target, true );
    CMD_PrintString( " Actual: ", true );
    CMD_PrintHex_U16( (uint16_t)pid_x.actual, true );
    CMD_PrintString( " Integral: ", true );
    CMD_PrintString( pid_x.integral_enabled ? "ON" : "OFF", true );
    CMD_PrintString( "\r\n", true );
    
    CMD_PrintString( "Y - Target: ", true );
    CMD_PrintHex_U16( (uint16_t)pid_y.target, true );
    CMD_PrintString( " Actual: ", true );
    CMD_PrintHex_U16( (uint16_t)pid_y.actual, true );
    CMD_PrintString( " Integral: ", true );
    CMD_PrintString( pid_y.integral_enabled ? "ON" : "OFF", true );
    CMD_PrintString( "\r\n", true );
}
