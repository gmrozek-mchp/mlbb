/**
 * @file main.c
 * @author greg
 * @date 2025-04-23
 * @brief Main function
 */

#include "bsp.h"
#include "peripheral/port/plib_port.h"
#include "system/reset/sys_reset.h"

#include "FreeRTOS.h"
#include "task.h"

#include "command/command.h"
#include "platform/platform.h"
 

void SetLED( void )
{
    if( CMD_GetArgc() >= 2 )
    {
        char state[2];
        CMD_GetArgv( 1, state, 2);
        
        if( state[0] == '0' )
        {
            LED_Set();
        }
        else
        {
            LED_Clear();
        }
    }
}

void ForceReset( void )
{
    SYS_RESET_SoftwareReset();
}


int main()
{
    BSP_Initialize();

    CMD_Initialize();

    PLATFORM_Initialize();

    CMD_RegisterCommand( "led", SetLED );
    CMD_RegisterCommand( "reset", ForceReset );

    vTaskStartScheduler();

    return 0;
}
