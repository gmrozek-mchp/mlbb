// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_human.h"

#include "nunchuk/nunchuk.h"
#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_HUMAN_Initialize( void )
{
}

void BALANCE_HUMAN_Reset( void )
{
    NUNCHUK_Zero_Set();
}

void BALANCE_HUMAN_Run( q15_t target_x, q15_t target_y )
{
    nunchuk_data_t nunchukData;
    q15_t x;
    q15_t y;
    
    nunchukData = NUNCHUK_Data_Get();
    
    // if( nunchukData.button_z )
    // {
    //    PLATFORM_Enable();

        x = (q15_t)nunchukData.joystick_x * 75;
        y = (q15_t)nunchukData.joystick_y * 75;

        PLATFORM_Position_XY_Set( x, y );
    // }
    // else
    // {
    //     PLATFORM_Disable();
    // }            
}

void BALANCE_HUMAN_DataVisualizer( void )
{

}


// ******************************************************************
// Section: Private Functions
// ******************************************************************


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************
