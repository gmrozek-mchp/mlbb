// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "platform.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arm_math.h"

#include "FreeRTOS.h"
#include "task.h"

#include "servo/servo.h"

#include "nunchuk/nunchuk.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define PLATFORM_RTOS_PRIORITY       (3)
#define PLATFORM_RTOS_STACK_SIZE     (configMINIMAL_STACK_SIZE)

#define PLATFORM_POWER_UP_DELAY_mS   (500)
#define PLATFORM_TASK_RATE_HZ        (100)


#define angle_0deg      ((q15_t)0x0000)
#define angle_30deg     ((q15_t)0x0AAA)
#define angle_45deg     ((q15_t)0x1000)
#define angle_60deg     ((q15_t)0x1555)
#define angle_90deg     ((q15_t)0x2000)

#define xcos60(x)   ((x) / 2)
#define xsin60(x)   (q15_t)(((int32_t)x * 0xDDB3) >> 16)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static TaskHandle_t platform_taskHandle = NULL;
static StaticTask_t platform_taskBuffer;
static StackType_t  platform_taskStack[ PLATFORM_RTOS_STACK_SIZE ];

static TickType_t platform_taskLastWakeTime;

static platform_abc_t platform_position_command_abc;
static platform_abc_t platform_position_actual_abc;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void PLATFORM_RTOS_Task( void * pvParameters );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void PLATFORM_Initialize( void )
{
    platform_position_command_abc.a = 0;
    platform_position_command_abc.b = 0;
    platform_position_command_abc.c = 0;
    
    SERVO_Initialize();

    platform_taskHandle = xTaskCreateStatic(
        PLATFORM_RTOS_Task,        /* Function that implements the task. */
        "Servo",                /* Text name for the task. */
        PLATFORM_RTOS_STACK_SIZE,  /* Number of indexes in the stack. */
        NULL,                     /* Parameter passed into the task. */
        PLATFORM_RTOS_PRIORITY,    /* Priority at which the task is created. */
        platform_taskStack,         /* Array to use as the task's stack. */
        &platform_taskBuffer        /* Variable to hold the task's data structure. */
    );
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
    abc.b = xsin60(-xy.x) + xcos60(-xy.y);
    abc.c = xsin60(xy.x) + xcos60(-xy.y);
    
    // TODO: compensate ABC based on non-linear motion of arms
    
    PLATFORM_Position_ABC_Set(abc);
}

platform_abc_t PLATFORM_Position_ABC_Get( void )
{
    return platform_position_actual_abc;
}

void PLATFORM_Position_ABC_Set( platform_abc_t abc )
{
//    q31_t offset;
    
    // offset ABC command such that platform center remains at fixed height
//    offset = (abc.a + abc.b + abc.c) / 3;    
//    abc.a += offset;
//    abc.b += offset;
//    abc.c += offset;
    
    // TODO: Need controlled access to command. Disable interrupts?
    platform_position_command_abc = abc;
    
    SERVO_Position_Command_Set_q15angle( SERVO_ID_A, abc.a );
    SERVO_Position_Command_Set_q15angle( SERVO_ID_B, abc.b );
    SERVO_Position_Command_Set_q15angle( SERVO_ID_C, abc.c );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void PLATFORM_RTOS_Task( void * pvParameters )
{
    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(PLATFORM_POWER_UP_DELAY_mS) );

    SERVO_Position_Command_Set_q15angle( SERVO_ID_A, 0xB00 );
    SERVO_Position_Command_Set_q15angle( SERVO_ID_B, 0xB00 );
    SERVO_Position_Command_Set_q15angle( SERVO_ID_C, 0xB00 );
    
    vTaskDelay( pdMS_TO_TICKS(500) );

    SERVO_Position_Zero_Set( SERVO_ID_A );
    SERVO_Position_Zero_Set( SERVO_ID_B );
    SERVO_Position_Zero_Set( SERVO_ID_C );
    
    platform_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
        nunchuk_data_t nunchukData = NUNCHUK_Data_Get();
        platform_xy_t platformXY;
        
        platformXY.x = ((q15_t)nunchukData.joystick_x - 127) * 25;
        platformXY.y = ((q15_t)nunchukData.joystick_y - 127) * 25;

        PLATFORM_Position_XY_Set( platformXY );
        
        vTaskDelayUntil( &platform_taskLastWakeTime, configTICK_RATE_HZ / PLATFORM_TASK_RATE_HZ );    
    }
}
