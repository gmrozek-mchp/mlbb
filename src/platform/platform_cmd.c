#include "platform.h"

#include "command/command.h"


// **************************************************************
//  Private function declarations
// **************************************************************


// **************************************************************
//  Public functions
// **************************************************************

void PLATFORM_CMD_Position_XY( void )
{
    
}

void PLATFORM_CMD_Position_ABC( void )
{
    platform_xy_t xy = {0};
    
    PLATFORM_Position_XY_Set( xy );
}


// **************************************************************
//  Private functions
// **************************************************************
