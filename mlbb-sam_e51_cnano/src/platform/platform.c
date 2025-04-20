#include "platform.h"

#include <xc.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arm_math.h"


#define angle_0deg      ((q15_t)0x0000)
#define angle_30deg     ((q15_t)0x0AAA)
#define angle_45deg     ((q15_t)0x1000)
#define angle_60deg     ((q15_t)0x1555)
#define angle_90deg     ((q15_t)0x2000)

#define xcos60(x)   ((x) / 2)
#define xsin60(x)   (q15_t)(((int32_t)x * 0xDDB3) >> 16)


static platform_abc_t position_command_abc;
static platform_abc_t position_actual_abc;


void PLATFORM_Initialize( void )
{
    position_command_abc.a = 0;
    position_command_abc.b = 0;
    position_command_abc.c = 0;
}

platform_xy_t PLATFORM_Position_XY_Get( void )
{
    platform_xy_t xy = {0};
    
    return xy;
}

void PLATFORM_Position_XY_Set( platform_xy_t xy )
{
    platform_abc_t abc;

    // calculate ABC command assuming linear actuators
    abc.a = xy.y;
    abc.b = (xy.x * arm_sin_q15(angle_60deg)) + xcos60(-xy.y);
    abc.c = xsin60(-xy.x) + xcos60(-xy.y);
    
    // TODO: compensate ABC based on non-linear motion of arms
    
    PLATFORM_Position_ABC_Set(abc);
}

platform_abc_t PLATFORM_Position_ABC_Get( void )
{
    return position_actual_abc;
}

void PLATFORM_Position_ABC_Set( platform_abc_t abc )
{
    q31_t offset;
    
    // offset ABC command such that platform center remains at fixed height
    offset = (abc.a + abc.b + abc.c) / 3;    
    abc.a += offset;
    abc.b += offset;
    abc.c += offset;
    
    // TODO: Need controlled access to command. Disable interrupts?
    position_command_abc = abc;
}
