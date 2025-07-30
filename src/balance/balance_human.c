// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_human.h"

#include "arm_math_types.h"
#include "command/command.h"

#include "nunchuk/nunchuk.h"
#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

// Full joystick range
#define JOYSTICK_LINEAR_RANGE   (25)    // Range around the center where the joystick is linear with minimum gain
#define JOYSTICK_FULL_RANGE     (100)   // Full joystick range

// Scaling factor for softened joystick
#define JOYSTICK_GAIN_MIN       (20)
#define JOYSTICK_GAIN_MAX       (75)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static q15_t SoftenJoystickValue(int16_t raw_value);


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

void BALANCE_HUMAN_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    nunchuk_data_t nunchukData = NUNCHUK_Data_Get();

    q15_t platform_command_x = SoftenJoystickValue(nunchukData.joystick_x);
    q15_t platform_command_y = SoftenJoystickValue(nunchukData.joystick_y);

    PLATFORM_Position_XY_Set( platform_command_x, platform_command_y );
}

void BALANCE_HUMAN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    static uint8_t dv_data[22];

    platform_xy_t platform_xy = PLATFORM_Position_XY_Get();
    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    dv_data[0] = 0x03;

    dv_data[1] = (uint8_t)'H';

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

/**
 * @brief Softens joystick input around the center by applying non-linear mapping
 * @param raw_value Raw joystick value (-128 to 127)
 * @return Softened joystick value scaled for platform control
 */
static q15_t SoftenJoystickValue(int16_t raw_value)
{
    int16_t abs_value;
    int16_t sign;
    int16_t gain;
    
    // Handle sign and get absolute value
    if (raw_value < 0)
    {
        sign = -1;
        abs_value = -raw_value;
    }
    else
    {
        sign = 1;
        abs_value = raw_value;
    }
    
    gain = JOYSTICK_GAIN_MIN;
    if( abs_value > JOYSTICK_LINEAR_RANGE )
    {
        gain += (abs_value - JOYSTICK_LINEAR_RANGE) * (JOYSTICK_GAIN_MAX - JOYSTICK_GAIN_MIN) / (JOYSTICK_FULL_RANGE - JOYSTICK_LINEAR_RANGE);
    }

    // Apply sign and gain for platform
    return (q15_t)(sign * abs_value * gain);
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************
