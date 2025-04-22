#include "nunchuk.h"

#include "command/command.h"


// **************************************************************
//  Private function declarations
// **************************************************************


// **************************************************************
//  Public functions
// **************************************************************

void NUNCHUK_CMD_Print_Data( void )
{
    nunchuk_data_t data = NUNCHUK_Data_Get();
    
    CMD_PrintString( "C: ", true );
    CMD_PrintDecimal_U32( (uint32_t)data.button_c, true, 1, true );
    CMD_PrintString( " Z: ", true );
    CMD_PrintDecimal_U32( (uint32_t)data.button_z, true, 1, true );
    CMD_PrintString( " X: ", true );
    CMD_PrintDecimal_U32( data.joystick_x, true, 4, true );
    CMD_PrintString( " Y: ", true );
    CMD_PrintDecimal_U32( data.joystick_y, true, 4, true );
    CMD_PrintString( "\r\n", true );
}


// **************************************************************
//  Private functions
// **************************************************************
