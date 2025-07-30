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
#define BALANCE_PID_CONSTANT_Ki_DEFAULT (0)
#define BALANCE_PID_CONSTANT_Kd_DEFAULT (8000)

#define BALANCE_PID_OUTPUT_SCALE_FACTOR_DEFAULT     (256)

#define BALANCE_PID_DELTA_FILTER_SIZE_DEFAULT  (5)

#define BALANCE_PID_HISTORY_DEPTH (10)


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

        q31_t error_sum;
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
static q15_t BALANCE_PID_Run_Instance( pid_q15_t *pid, q15_t target, q15_t actual );


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_PID_CMD_Print_State( void );
static void BALANCE_PID_CMD_Print_Constants( void );
static void BALANCE_PID_CMD_Set_Kp( void );
static void BALANCE_PID_CMD_Set_Ki( void );
static void BALANCE_PID_CMD_Set_Kd( void );


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
        q15_t platform_x = BALANCE_PID_Run_Instance( &pid_x, target_x, ball_x );
        q15_t platform_y = BALANCE_PID_Run_Instance( &pid_y, target_y, ball_y );

        PLATFORM_Position_XY_Set( platform_x, platform_y );
    }
    else
    {
        BALANCE_PID_Reset();
        PLATFORM_Position_XY_Set( 0, 0 );
    }   
}

void BALANCE_PID_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    static uint8_t dv_data[22];

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

    dv_data[11] = (uint8_t)platform_xy.x;
    dv_data[12] = (uint8_t)(platform_xy.x >> 8);
    dv_data[13] = (uint8_t)platform_xy.y;
    dv_data[14] = (uint8_t)(platform_xy.y >> 8);

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

    for( size_t i = 0; i < BALANCE_PID_HISTORY_DEPTH; i++ )
    {
        pid->error_history[i] = 0;
    }
}

static q15_t BALANCE_PID_Run_Instance( pid_q15_t *pid, q15_t target, q15_t actual )
{
    q31_t error;
    int16_t error_delta_index;
    q31_t error_delta;
    q31_t p_term;
    q31_t i_term;
    q31_t d_term;

    // calculate the error
    error = ((q31_t)target - actual);

    // calculate the index to use for error delta calculation
    error_delta_index = pid->error_history_index - pid->delta_filter_size;
    if( error_delta_index < 0 )
    {
        error_delta_index += BALANCE_PID_HISTORY_DEPTH;
    }

    // calculate the error delta
    error_delta = error - pid->error_history[error_delta_index];

    // calculate the proportional term
    p_term = error * pid->Kp;

    // calculate the integral term
    pid->error_sum += error;
    i_term = pid->error_sum * pid->Ki;

    // calculate the derivative term
    d_term = error_delta * pid->Kd;

    // update the error history
    pid->error_history[pid->error_history_index] = error;
    pid->error_history_index++;

    // wrap the error history index
    if( pid->error_history_index >= BALANCE_PID_HISTORY_DEPTH )
    {
        pid->error_history_index = 0;
    }

    // calculate the output
    q31_t output = (p_term + i_term + d_term) / pid->output_scale_factor;

    if( output > INT16_MAX )
    {
        output = INT16_MAX;
    }
    else if( output < INT16_MIN )
    {
        output = INT16_MIN;
    }

    return (q15_t)output;
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
    CMD_PrintString( "Kp: ", true );
    CMD_PrintHex_U32( (uint32_t)pid_x.Kp, true );
    CMD_PrintString( " Ki: ", true );
    CMD_PrintHex_U32( (uint32_t)pid_x.Ki, true );
    CMD_PrintString( " Kd: ", true );
    CMD_PrintHex_U32( (uint32_t)pid_x.Kd, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_PID_CMD_Set_Kp( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        q31_t kp;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        kp = (q31_t)atoi( arg_buffer );

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
        q31_t ki;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        ki = (q31_t)atoi( arg_buffer );

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
        q31_t kd;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        kd = (q31_t)atoi( arg_buffer );

        pid_x.Kd = kd;
        pid_y.Kd = kd;
    }

    BALANCE_PID_CMD_Print_Constants();
}
