// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "platform.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "arm_math_types.h"

#include "FreeRTOS.h"
#include "task.h"

#include "command/command.h"

#include "servo/servo.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

#define PLATFORM_RTOS_PRIORITY       (3)
#define PLATFORM_RTOS_STACK_SIZE     (configMINIMAL_STACK_SIZE)

#define PLATFORM_POWER_UP_DELAY_mS   (100)
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

static bool platform_enabled = false;

static platform_abc_t platform_position_command_abc;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void PLATFORM_RTOS_Task( void * pvParameters );


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void PLATFORM_CMD_Position_XY( void );
static void PLATFORM_CMD_Position_ABC( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void PLATFORM_Initialize( void )
{
    platform_position_command_abc.a = 0;
    platform_position_command_abc.b = 0;
    platform_position_command_abc.c = 0;
    
    CMD_RegisterCommand( "xy", PLATFORM_CMD_Position_XY );
    CMD_RegisterCommand( "abc", PLATFORM_CMD_Position_ABC );

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

void PLATFORM_Disable( void )
{
    if( platform_enabled )
    {
        // Stagger servo movement because all 3 moving identical could trip power supply on deceleration
        SERVO_Position_Command_Set_q15angle( SERVO_ID_A, angle_0deg );
        vTaskDelay(100);
        SERVO_Position_Command_Set_q15angle( SERVO_ID_B, angle_0deg );
        vTaskDelay(100);
        SERVO_Position_Command_Set_q15angle( SERVO_ID_C, angle_0deg );
        vTaskDelay(500);

        SERVO_Disable();

        platform_enabled = false;
    }
}

void PLATFORM_Enable( void )
{
    if( !platform_enabled )
    {
        SERVO_Enable();

        // Stagger servo movement because all 3 moving identical could trip power supply on deceleration
        SERVO_Position_Command_Set_q15angle( SERVO_ID_A, angle_0deg );
        vTaskDelay(100);
        SERVO_Position_Command_Set_q15angle( SERVO_ID_B, angle_0deg );
        vTaskDelay(100);
        SERVO_Position_Command_Set_q15angle( SERVO_ID_C, angle_0deg );
        vTaskDelay(500);

        platform_enabled = true;
    }
}

platform_xy_t PLATFORM_Position_XY_Get( void )
{
    platform_xy_t xy;
    
    
    // x = (c + a*cos(60)) / sin(60)
    // calculate in int32_t to prevent overflow and allow for fractional integer multiplication
    int32_t x = platform_position_command_abc.c + (platform_position_command_abc.a / 2);
    x = x * 37838 >> 15; // Divide by sin(60deg)

    xy.x = (q15_t)x;

    // y = a
    xy.y = platform_position_command_abc.a;
    
    return xy;
}

void PLATFORM_Position_XY_Set( q15_t x, q15_t y )
{
    // calculate ABC command assuming linear actuators
    q15_t a = y;
    q15_t b = xsin60(-x) + xcos60(-y);
    q15_t c = xsin60(x) + xcos60(-y);
    
    // TODO: compensate ABC based on non-linear motion of arms
    
    PLATFORM_Position_ABC_Set( a, b, c );
}

platform_abc_t PLATFORM_Position_ABC_Get( void )
{
    return platform_position_command_abc;
}

void PLATFORM_Position_ABC_Set( q15_t a, q15_t b, q15_t c )
{
//    q31_t offset;
    
    // offset ABC command such that platform center remains at fixed height
//    offset = (a + b + c) / 3;    
//    a += offset;
//    b += offset;
//    c += offset;
    
    // TODO: Need controlled access to command. Disable interrupts?
    platform_position_command_abc.a = a;
    platform_position_command_abc.b = b;
    platform_position_command_abc.c = c;
    
    SERVO_Position_Command_Set_q15angle( SERVO_ID_A, a );
    SERVO_Position_Command_Set_q15angle( SERVO_ID_B, b );
    SERVO_Position_Command_Set_q15angle( SERVO_ID_C, c );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void PLATFORM_RTOS_Task( void * pvParameters )
{
    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(PLATFORM_POWER_UP_DELAY_mS) );
    
    platform_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {        
        vTaskDelayUntil( &platform_taskLastWakeTime, configTICK_RATE_HZ / PLATFORM_TASK_RATE_HZ );    
    }
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void PLATFORM_CMD_Position_XY( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 3 )
    {
        q15_t platform_x;
        q15_t platform_y;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        platform_x = (q15_t)atoi( arg_buffer );

        CMD_GetArgv( 2, arg_buffer, sizeof(arg_buffer) );
        platform_y = (q15_t)atoi( arg_buffer );

        PLATFORM_Position_XY_Set( platform_x, platform_y );
    }

    platform_xy_t platform_xy = PLATFORM_Position_XY_Get();

    CMD_PrintString( "X: 0x", true );
    CMD_PrintHex_U16( platform_xy.x, true );
    CMD_PrintString( " Y: 0x", true );
    CMD_PrintHex_U16( platform_xy.y, true );
    CMD_PrintString( "\r\n", true );
}

static void PLATFORM_CMD_Position_ABC( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 4 )
    {
        q15_t platform_a;
        q15_t platform_b;
        q15_t platform_c;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        platform_a = (q15_t)atoi( arg_buffer );

        CMD_GetArgv( 2, arg_buffer, sizeof(arg_buffer) );
        platform_b = (q15_t)atoi( arg_buffer );

        CMD_GetArgv( 3, arg_buffer, sizeof(arg_buffer) );
        platform_c = (q15_t)atoi( arg_buffer );

        PLATFORM_Position_ABC_Set( platform_a, platform_b, platform_c );
    }

    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    CMD_PrintString( "A: 0x", true );
    CMD_PrintHex_U16( platform_abc.a, true );
    CMD_PrintString( " B: 0x", true );
    CMD_PrintHex_U16( platform_abc.b, true );
    CMD_PrintString( " C: 0x", true );
    CMD_PrintHex_U16( platform_abc.c, true );
    CMD_PrintString( "\r\n", true );
}
