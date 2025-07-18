// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance.h"

#include "FreeRTOS.h"
#include "task.h"

#include "peripheral/port/plib_port.h"

#include "ball/ball.h"
#include "nunchuk/nunchuk.h"
#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define BALANCE_RTOS_PRIORITY       (2)
#define BALANCE_RTOS_STACK_SIZE     (configMINIMAL_STACK_SIZE)

#define BALANCE_POWER_UP_DELAY_mS   (1000)
#define BALANCE_TASK_RATE_HZ        (100)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static TaskHandle_t balance_taskHandle = NULL;
static StaticTask_t balance_taskBuffer;
static StackType_t  balance_taskStack[ BALANCE_RTOS_STACK_SIZE ];

static TickType_t balance_taskLastWakeTime;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void BALANCE_RTOS_Task( void * pvParameters );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_Initialize( void )
{
    BALL_Initialize();

    NUNCHUK_Initialize();

    PLATFORM_Initialize();
    
    balance_taskHandle = xTaskCreateStatic(
        BALANCE_RTOS_Task,        /* Function that implements the task. */
        "Balance",                /* Text name for the task. */
        BALANCE_RTOS_STACK_SIZE,  /* Number of indexes in the stack. */
        NULL,                     /* Parameter passed into the task. */
        BALANCE_RTOS_PRIORITY,    /* Priority at which the task is created. */
        balance_taskStack,         /* Array to use as the task's stack. */
        &balance_taskBuffer        /* Variable to hold the task's data structure. */
    );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void BALANCE_RTOS_Task( void * pvParameters )
{
    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(BALANCE_POWER_UP_DELAY_mS) );

    NUNCHUK_Zero_Set();

    balance_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
        nunchuk_data_t nunchukData;
        q15_t x;
        q15_t y;
        
        nunchukData = NUNCHUK_Data_Get();
        
        if( nunchukData.button_z )
        {
            PLATFORM_Enable();

            x = (q15_t)nunchukData.joystick_x * 25;
            y = (q15_t)nunchukData.joystick_y * 25;

            PLATFORM_Position_XY_Set( x, y );
        }
        else
        {
            PLATFORM_Disable();
        }            

        vTaskDelayUntil( &balance_taskLastWakeTime, configTICK_RATE_HZ / BALANCE_TASK_RATE_HZ );    
    }
}
