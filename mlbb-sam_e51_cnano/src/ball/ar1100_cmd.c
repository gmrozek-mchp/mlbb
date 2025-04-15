#include "ar1100.h"

#include "command/command.h"


// **************************************************************
//  Private function declarations
// **************************************************************


// **************************************************************
//  Public functions
// **************************************************************

void AR1100_CMD_Print_TouchData( void )
{
    ar1100_touchdata_t touch_data = AR1100_TouchData_Get();
    
    CMD_PrintString( "T: ", true );
    CMD_PrintDecimal_U32( (uint32_t)touch_data.down, 0, 0, true );
    CMD_PrintString( " X: 0x", true );
    CMD_PrintHex_U16( touch_data.x, true );
    CMD_PrintString( " Y: 0x", true );
    CMD_PrintHex_U16( touch_data.y, true );
    CMD_PrintString( "\r\n", true );
}


// **************************************************************
//  Private functions
// **************************************************************
