// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_pid.h"

#include <stdlib.h>

#include "arm_math.h"

#include "FreeRTOS.h"
#include "task.h"

#include "command/command.h"

#include "ball/ball.h"
#include "dsp/controller_functions.h"
#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

#define BALANCE_PID_CONSTANT_Kp (0x1)
#define BALANCE_PID_CONSTANT_Ki (0x1)
#define BALANCE_PID_CONSTANT_Kd (0x0)

#define BALANCE_PID_TARGET_X    (0x4000)
#define BALANCE_PID_TARGET_Y    (0x4000)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************

arm_pid_instance_q15 balance_pid_x;
arm_pid_instance_q15 balance_pid_y;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_PID_CMD_Print_Constants( void );
static void BALANCE_PID_CMD_Set_Kp( void );
static void BALANCE_PID_CMD_Set_Ki( void );
static void BALANCE_PID_CMD_Set_Kd( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_PID_Initialize( void )
{
    balance_pid_x.Kp = 0x100;
    balance_pid_x.Ki = 0x100;
    balance_pid_x.Kd = 0x100;
    arm_pid_init_q15( &balance_pid_x, 1 );

    balance_pid_y.Kp = 0x100;
    balance_pid_y.Ki = 0x100;
    balance_pid_y.Kd = 0x100;
    arm_pid_init_q15( &balance_pid_y, 1 );

    CMD_RegisterCommand( "pid", BALANCE_PID_CMD_Print_Constants );
    CMD_RegisterCommand( "kp", BALANCE_PID_CMD_Set_Kp );
    CMD_RegisterCommand( "ki", BALANCE_PID_CMD_Set_Ki );
    CMD_RegisterCommand( "kd", BALANCE_PID_CMD_Set_Kd );
}

void BALANCE_PID_Reset( void )
{
    arm_pid_reset_q15( &balance_pid_x );
    arm_pid_reset_q15( &balance_pid_y );
}

void BALANCE_PID_Run( void )
{
    ball_data_t ball_data = BALL_Position_Get();

    if( ball_data.detected )
    {
        q15_t error_x = BALANCE_PID_TARGET_X - ball_data.x;
        q15_t error_y = BALANCE_PID_TARGET_Y - ball_data.y;

        q15_t command_x = arm_pid_q15( &balance_pid_x, error_x );
        q15_t command_y = arm_pid_q15( &balance_pid_y, error_y );

        PLATFORM_Position_XY_Set( command_x, command_y );
    }
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_PID_CMD_Print_Constants( void )
{
    CMD_PrintString( "Kp: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_x.Kp, true );
    CMD_PrintString( " Ki: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_x.Ki, true );
    CMD_PrintString( " Kd: ", true );
    CMD_PrintHex_U16( (uint16_t)balance_pid_x.Kd, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_PID_CMD_Set_Kp( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        q15_t kp;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        kp = (q15_t)atoi( arg_buffer );

        balance_pid_x.Kp = kp;
        balance_pid_y.Kp = kp;

        taskENTER_CRITICAL();

        arm_pid_init_q15( &balance_pid_x, 0 );
        arm_pid_init_q15( &balance_pid_y, 0 );

        taskEXIT_CRITICAL();
    }

    BALANCE_PID_CMD_Print_Constants();
}

static void BALANCE_PID_CMD_Set_Ki( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        q15_t ki;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        ki = (q15_t)atoi( arg_buffer );

        balance_pid_x.Ki = ki;
        balance_pid_y.Ki = ki;

        taskENTER_CRITICAL();

        arm_pid_init_q15( &balance_pid_x, 0 );
        arm_pid_init_q15( &balance_pid_y, 0 );

        taskEXIT_CRITICAL();
    }

    BALANCE_PID_CMD_Print_Constants();
}

static void BALANCE_PID_CMD_Set_Kd( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        q15_t kd;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        kd = (q15_t)atoi( arg_buffer );

        balance_pid_x.Kd = kd;
        balance_pid_y.Kd = kd;

        taskENTER_CRITICAL();

        arm_pid_init_q15( &balance_pid_x, 0 );
        arm_pid_init_q15( &balance_pid_y, 0 );

        taskEXIT_CRITICAL();
    }

    BALANCE_PID_CMD_Print_Constants();
}
