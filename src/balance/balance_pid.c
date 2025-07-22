// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_pid.h"

#include <stdlib.h>

#include "arm_math_types.h"

#include "FreeRTOS.h"
#include "task.h"

#include "command/command.h"

#include "ball/ball.h"
#include "dsp/controller_functions.h"
#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

#define BALANCE_PID_CONSTANT_Kp (6600)
#define BALANCE_PID_CONSTANT_Ki (24)
#define BALANCE_PID_CONSTANT_Kd (1000000)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************

arm_pid_instance_q31 balance_pid_x;
arm_pid_instance_q31 balance_pid_y;

q15_t pid_target_x;
q15_t pid_target_y;

q15_t pid_platform_command_x;
q15_t pid_platform_command_y;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************


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
    balance_pid_x.Kp = BALANCE_PID_CONSTANT_Kp;
    balance_pid_x.Ki = BALANCE_PID_CONSTANT_Ki;
    balance_pid_x.Kd = BALANCE_PID_CONSTANT_Kd;
    arm_pid_init_q31( &balance_pid_x, 1 );

    balance_pid_y.Kp = BALANCE_PID_CONSTANT_Kp;
    balance_pid_y.Ki = BALANCE_PID_CONSTANT_Ki;
    balance_pid_y.Kd = BALANCE_PID_CONSTANT_Kd;
    arm_pid_init_q31( &balance_pid_y, 1 );

    CMD_RegisterCommand( "pid", BALANCE_PID_CMD_Print_State );
    CMD_RegisterCommand( "pidk", BALANCE_PID_CMD_Print_Constants );
    CMD_RegisterCommand( "kp", BALANCE_PID_CMD_Set_Kp );
    CMD_RegisterCommand( "ki", BALANCE_PID_CMD_Set_Ki );
    CMD_RegisterCommand( "kd", BALANCE_PID_CMD_Set_Kd );
}

void BALANCE_PID_Reset( void )
{
    arm_pid_reset_q31( &balance_pid_x );
    arm_pid_reset_q31( &balance_pid_y );
}

void BALANCE_PID_Run( q15_t target_x, q15_t target_y )
{
    ball_data_t ball_data;
    static uint16_t debounce_count = 10;
    
    ball_data = BALL_Position_Get();

    if( ball_data.detected )
    {
        q31_t error_x = -((q31_t)target_x - ball_data.x) << 19;
        q31_t error_y = ((q31_t)target_y - ball_data.y) << 19;

        q15_t command_x = arm_pid_q31( &balance_pid_x, error_x );
        q15_t command_y = arm_pid_q31( &balance_pid_y, error_y );

        PLATFORM_Position_XY_Set( command_x, command_y );

        debounce_count = 0;
    }
    else
    {
        debounce_count++;
        if( debounce_count >= 10 )
        {
            debounce_count = 10;

            arm_pid_reset_q31( &balance_pid_x );
            arm_pid_reset_q31( &balance_pid_y );
            PLATFORM_Position_XY_Set( 0, 0 );
        }
        else
        {
            q31_t error_x = -((q31_t)target_x - ball_data.x) << 19;
            q31_t error_y = ((q31_t)target_y - ball_data.y) << 19;

            q15_t command_x = arm_pid_q31( &balance_pid_x, error_x );
            q15_t command_y = arm_pid_q31( &balance_pid_y, error_y );

            PLATFORM_Position_XY_Set( command_x, command_y );
        }
    }
}

void BALANCE_PID_DataVisualizer( void )
{
    static uint8_t dv_data[21];

    ball_data_t ball = BALL_Position_Get();;
    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    dv_data[0] = 'P';

    dv_data[1] = (uint8_t)ball.detected;

    dv_data[2] = (uint8_t)pid_target_x;
    dv_data[3] = (uint8_t)(pid_target_y >> 8);
    dv_data[4] = (uint8_t)pid_target_y;
    dv_data[5] = (uint8_t)(pid_target_x >> 8);

    dv_data[6] = (uint8_t)ball.x;
    dv_data[7] = (uint8_t)(ball.x >> 8);
    dv_data[8] = (uint8_t)ball.y;
    dv_data[9] = (uint8_t)(ball.y >> 8);

    dv_data[10] = (uint8_t)pid_platform_command_x;
    dv_data[11] = (uint8_t)(pid_platform_command_x >> 8);
    dv_data[12] = (uint8_t)pid_platform_command_y;
    dv_data[13] = (uint8_t)(pid_platform_command_y >> 8);

    dv_data[14] = (uint8_t)platform_abc.a;
    dv_data[15] = (uint8_t)(platform_abc.a >> 8);
    dv_data[16] = (uint8_t)platform_abc.b;
    dv_data[17] = (uint8_t)(platform_abc.b >> 8);
    dv_data[18] = (uint8_t)platform_abc.c;
    dv_data[19] = (uint8_t)(platform_abc.c >> 8);

    dv_data[20] = ~'P';

    CMD_PrintByteArray( dv_data, sizeof(dv_data), false );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_PID_CMD_Print_State( void )
{
    CMD_PrintString( "xS0: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_x.state[0], true );
    CMD_PrintString( " xS1: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_x.state[1], true );
    CMD_PrintString( " xS2: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_x.state[2], true );
    CMD_PrintString( " yS0: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_y.state[0], true );
    CMD_PrintString( " yS1: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_y.state[1], true );
    CMD_PrintString( " yS2: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_y.state[2], true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_PID_CMD_Print_Constants( void )
{
    CMD_PrintString( "Kp: ", true );
    CMD_PrintHex_U32( (uint32_t)balance_pid_x.Kp, true );
    CMD_PrintString( " Ki: ", true );
    CMD_PrintHex_U32( (uint32_t)balance_pid_x.Ki, true );
    CMD_PrintString( " Kd: ", true );
    CMD_PrintHex_U32( (uint32_t)balance_pid_x.Kd, true );
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

        balance_pid_x.Kp = kp;
        balance_pid_y.Kp = kp;

        taskENTER_CRITICAL();

        arm_pid_init_q31( &balance_pid_x, 0 );
        arm_pid_init_q31( &balance_pid_y, 0 );

        taskEXIT_CRITICAL();
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

        balance_pid_x.Ki = ki;
        balance_pid_y.Ki = ki;

        taskENTER_CRITICAL();

        arm_pid_init_q31( &balance_pid_x, 0 );
        arm_pid_init_q31( &balance_pid_y, 0 );

        taskEXIT_CRITICAL();
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

        balance_pid_x.Kd = kd;
        balance_pid_y.Kd = kd;

        taskENTER_CRITICAL();

        arm_pid_init_q31( &balance_pid_x, 0 );
        arm_pid_init_q31( &balance_pid_y, 0 );

        taskEXIT_CRITICAL();
    }

    BALANCE_PID_CMD_Print_Constants();
}
