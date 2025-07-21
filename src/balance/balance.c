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

#include "balance/balance_human.h"
#include "balance/balance_nn.h"
#include "balance/balance_pid.h"


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

static balance_mode_t balance_mode = BALANCE_MODE_HUMAN;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void BALANCE_RTOS_Task( void * pvParameters );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_Initialize( void )
{
    BALANCE_HUMAN_Initialize();
    BALANCE_NN_Initialize();
    BALANCE_PID_Initialize();
    
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

balance_mode_t BALANCE_MODE_Get( void )
{
    return balance_mode;
}

void BALANCE_MODE_Set( balance_mode_t mode )
{
    if( mode >= BALANCE_MODE_NUM_MODES )
    {
        return;
    }

    balance_mode = mode;
}

void BALANCE_MODE_Next( void )
{
    if( balance_mode >= BALANCE_MODE_NUM_MODES - 1 )
    {
        balance_mode = 0;
    }
    else
    {
        balance_mode++;
    }
}

void BALANCE_MODE_Previous( void )
{
    if( balance_mode == 0 )
    {
        balance_mode = BALANCE_MODE_NUM_MODES - 1;
    }
    else
    {
        balance_mode--;
    }
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void BALANCE_RTOS_Task( void * pvParameters )
{
    balance_mode_t active_balance_mode = BALANCE_MODE_INVALID;

    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(BALANCE_POWER_UP_DELAY_mS) );

    balance_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
        if( active_balance_mode != balance_mode )
        {
            active_balance_mode = balance_mode;

            LED_MODE_HUMAN_Clear();
            LED_MODE_PID_Clear();
            LED_MODE_NEURAL_NETWORK_Clear();

            switch( active_balance_mode )
            {
                case BALANCE_MODE_OFF:
                {
                    PLATFORM_Disable();
                    break;
                }

                case BALANCE_MODE_HUMAN:
                {
                    LED_MODE_HUMAN_Set();
                    BALANCE_HUMAN_Reset();
                    PLATFORM_Enable();
                    break;
                }

                case BALANCE_MODE_PID:
                {
                    LED_MODE_PID_Set();
                    BALANCE_PID_Reset();
                    PLATFORM_Enable();
                    break;
                }

                case BALANCE_MODE_NN:
                {
                    LED_MODE_NEURAL_NETWORK_Set();
                    BALANCE_NN_Reset();
                    PLATFORM_Enable();
                    break;
                }

                default:
                {
                    break;
                }
            }

        }

        switch( active_balance_mode )
        {
            case BALANCE_MODE_HUMAN:
            {
                BALANCE_HUMAN_Run();
                break;
            }

            case BALANCE_MODE_PID:
            {
                BALANCE_PID_Run();
                break;
            }

            case BALANCE_MODE_NN:
            {
                BALANCE_NN_Run();
                break;
            }

            default:
            {
                break;
            }
        }

        vTaskDelayUntil( &balance_taskLastWakeTime, configTICK_RATE_HZ / BALANCE_TASK_RATE_HZ );    
    }
}

