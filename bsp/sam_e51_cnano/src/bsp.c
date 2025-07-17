#include "bsp.h"

#include "definitions.h"
#include "driver/driver_i2c.h"

#include "command/command.h"


void SetLED( void )
{
    if( CMD_GetArgc() >= 2 )
    {
        char state[2];
        CMD_GetArgv( 1, state, 2);
        
        if( state[0] == '0' )
        {
            LED_CNANO_Set();
        }
        else
        {
            LED_CNANO_Clear();
        }
    }
}

void ForceReset( void )
{
    SYS_RESET_SoftwareReset();
}


void BSP_Initialize( void )
{
    SYS_Initialize ( NULL );

    DRIVER_I2C_Initialize();

    CMD_RegisterCommand( "led", SetLED );
    CMD_RegisterCommand( "reset", ForceReset );    
}
