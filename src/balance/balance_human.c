// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_human.h"

#include "arm_math_types.h"
#include "ball/ball.h"
#include "command/command.h"

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

static q15_t human_target_x;
static q15_t human_target_y;
static q15_t human_platform_command_x;
static q15_t human_platform_command_y;


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
    nunchuk_data_t nunchukData = NUNCHUK_Data_Get();

    human_target_x = target_x;
    human_target_y = target_y;
    
    human_platform_command_x = (q15_t)nunchukData.joystick_x * 75;
    human_platform_command_y = (q15_t)nunchukData.joystick_y * 75;

    PLATFORM_Position_XY_Set( human_platform_command_x, human_platform_command_y );
}

void BALANCE_HUMAN_DataVisualizer( void )
{
    static uint8_t dv_data[21];

    ball_data_t ball = BALL_Position_Get();;
    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    dv_data[0] = 'H';

    dv_data[1] = (uint8_t)ball.detected;

    dv_data[2] = (uint8_t)human_target_x;
    dv_data[3] = (uint8_t)(human_target_x >> 8);
    dv_data[4] = (uint8_t)human_target_y;
    dv_data[5] = (uint8_t)(human_target_y >> 8);

    dv_data[6] = (uint8_t)ball.x;
    dv_data[7] = (uint8_t)(ball.x >> 8);
    dv_data[8] = (uint8_t)ball.y;
    dv_data[9] = (uint8_t)(ball.y >> 8);

    dv_data[10] = (uint8_t)human_platform_command_x;
    dv_data[11] = (uint8_t)(human_platform_command_x >> 8);
    dv_data[12] = (uint8_t)human_platform_command_y;
    dv_data[13] = (uint8_t)(human_platform_command_y >> 8);

    dv_data[14] = (uint8_t)platform_abc.a;
    dv_data[15] = (uint8_t)(platform_abc.a >> 8);
    dv_data[16] = (uint8_t)platform_abc.b;
    dv_data[17] = (uint8_t)(platform_abc.b >> 8);
    dv_data[18] = (uint8_t)platform_abc.c;
    dv_data[19] = (uint8_t)(platform_abc.c >> 8);

    dv_data[20] = ~'H';

    CMD_PrintByteArray( dv_data, sizeof(dv_data), false );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************
