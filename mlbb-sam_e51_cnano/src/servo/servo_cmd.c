#include "servo.h"

#include <stdlib.h>

#include "command/command.h"


// **************************************************************
//  Private function declarations
// **************************************************************


// **************************************************************
//  Public functions
// **************************************************************

void SERVO_CMD_Position_GetSet_q15angle( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        servo_id_t servo_id;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        servo_id = (servo_id_t)atoi( arg_buffer );
        
        if (CMD_GetArgc() == 2 )
        {
            CMD_PrintString( "ANGLE: 0x", true );
            CMD_PrintHex_U16( SERVO_Position_Get_q15angle((servo_id_t)atoi( arg_buffer )), true );
            CMD_PrintString( "\r\n", true );
        }
        else
        {
            q15_t angle;
            
            CMD_GetArgv( 2, arg_buffer, sizeof(arg_buffer) );
            angle = (q15_t)atoi( arg_buffer );

            SERVO_Position_Command_Set_q15angle( servo_id, angle );
        }
    }
}

void SERVO_CMD_Position_GetSet_steps( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        servo_id_t servo_id;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        servo_id = (servo_id_t)atoi( arg_buffer );
        
        if (CMD_GetArgc() == 2 )
        {
            CMD_PrintString( "STEPS: 0x", true );
            CMD_PrintHex_U16( SERVO_Position_Get_steps(servo_id), true );
            CMD_PrintString( "\r\n", true );
        }
        else
        {
            int16_t steps;
            
            CMD_GetArgv( 2, arg_buffer, sizeof(arg_buffer) );
            steps = (int16_t)atoi( arg_buffer );

            SERVO_Position_Command_Set_steps( servo_id, steps);
        }
    }
}

void SERVO_CMD_Position_Zero( void )
{
    char arg_buffer[10];
    
    if( CMD_GetArgc() >= 2 )
    {
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        SERVO_Position_Zero_Set(  (servo_id_t)atoi( arg_buffer ) );
    }
}


// **************************************************************
//  Private functions
// **************************************************************
