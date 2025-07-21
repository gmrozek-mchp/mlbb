// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance.h"

#include <stdlib.h>

#include "arm_math_types.h"

#include "FreeRTOS.h"
#include "task.h"

#include "peripheral/port/plib_port.h"

#include "command/command.h"

#include "balance/balance_human.h"
#include "balance/balance_nn.h"
#include "balance/balance_pid.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define BALANCE_RTOS_PRIORITY       (2)
#define BALANCE_RTOS_STACK_SIZE     (2*configMINIMAL_STACK_SIZE)

#define BALANCE_POWER_UP_DELAY_mS   (1000)
#define BALANCE_TASK_RATE_HZ        (100)

#define BALANCE_TARGET_CYCLE_INTERVAL   (500)

// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************
typedef struct {
    void (*init)( void );
    void (*reset)( void );
    void (*run)( q15_t target_x, q15_t target_y );
    void (*dv)( void );
    PORT_PIN led_mode_pin;
} balance_interface_t;

typedef struct {
    q15_t x;
    q15_t y;
    PORT_PIN led_target_pin;
} balance_target_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static TaskHandle_t balance_taskHandle = NULL;
static StaticTask_t balance_taskBuffer;
static StackType_t  balance_taskStack[ BALANCE_RTOS_STACK_SIZE ];

static TickType_t balance_taskLastWakeTime;

static balance_mode_t balance_mode = BALANCE_MODE_PID;

static balance_target_t balance_target;
static bool balance_dv_active = false;

static const balance_interface_t balancer_off = {
    .init = NULL,
    .reset = NULL,
    .run = NULL,
    .dv = NULL,
    .led_mode_pin = PORT_PIN_NONE
};
static const balance_interface_t balancer_human = { 
    .init = BALANCE_HUMAN_Initialize,
    .reset = BALANCE_HUMAN_Reset,
    .run = BALANCE_HUMAN_Run,
    .dv = BALANCE_HUMAN_DataVisualizer,
    .led_mode_pin = LED_MODE_HUMAN_PIN
};
static const balance_interface_t balancer_pid = {
    .init = BALANCE_PID_Initialize,
    .reset = BALANCE_PID_Reset,
    .run = BALANCE_PID_Run,
    .dv = BALANCE_PID_DataVisualizer,
    .led_mode_pin = LED_MODE_PID_PIN
};
static const balance_interface_t balancer_nn = {
    .init = BALANCE_NN_Initialize,
    .reset = BALANCE_NN_Reset,
    .run = BALANCE_NN_Run,
    .dv = BALANCE_NN_DataVisualizer,
    .led_mode_pin = LED_MODE_NEURAL_NETWORK_PIN
};

static balance_interface_t balancers[] = {
    balancer_off,
    balancer_human,
    balancer_pid,
    balancer_nn
};

static balance_target_t balance_targets[] = {
    { .x = 0x7F0, .y = 0x810, .led_target_pin = LED_TARGET_CENTER_PIN },
    { .x = 0x4C8, .y = 0xBC0, .led_target_pin = LED_TARGET_TOP_RIGHT_PIN },
    { .x = 0xB18, .y = 0xBB8, .led_target_pin = LED_TARGET_TOP_LEFT_PIN },
    { .x = 0xB18, .y = 0x470, .led_target_pin = LED_TARGET_BOTTOM_LEFT_PIN },
    { .x = 0x4C8, .y = 0x470, .led_target_pin = LED_TARGET_BOTTOM_RIGHT_PIN }
};


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void BALANCE_RTOS_Task( void * pvParameters );

static void BALANCE_CMD_DataVisualizer( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_Initialize( void )
{
    balance_mode_t balancer_index;

    for( balancer_index = 0; balancer_index < BALANCE_MODE_NUM_MODES; balancer_index++ )
    {
        if( balancers[balancer_index].init != NULL )
        {
            balancers[balancer_index].init();
        }
    }

    balance_target = balance_targets[balance_target_cycle_index];
    PORT_PinSet( balance_targets[balance_target_cycle_index].led_target_pin );

    CMD_RegisterCommand( "dvbalance", BALANCE_CMD_DataVisualizer );

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
    uint32_t balance_target_timer = 0;

    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(BALANCE_POWER_UP_DELAY_mS) );

    balance_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
        // Check if balance mode has changed.
        if( active_balance_mode != balance_mode )
        {
            balance_mode_t balancer_index;

            active_balance_mode = balance_mode;
            balance_dv_active = false;

            // Turn off all balance mode indicators.
            for( balancer_index = 0; balancer_index < BALANCE_MODE_NUM_MODES; balancer_index++ )
            {
                if( balancers[active_balance_mode].led_mode_pin != PORT_PIN_NONE )
                {
                    PORT_PinClear( balancers[active_balance_mode].led_mode_pin );
                }
            }

            // Turn on active balance mode indicator.
            if( balancers[active_balance_mode].led_mode_pin != PORT_PIN_NONE )
            {
                PORT_PinSet( balancers[active_balance_mode].led_mode_pin );
            }

            // Reset the active balancer.
            if( balancers[active_balance_mode].reset != NULL )
            {
                balancers[active_balance_mode].reset();
            }
        }

        // Run the active balancer
        if( balancers[active_balance_mode].run != NULL )
        {
            balancers[active_balance_mode].run();
        }

        // Stream data visualizer if active
        if( balance_dv_active && (balancers[active_balance_mode].dv != NULL) )
        {
            balancers[active_balance_mode].dv();
        }

        balance_target_timer++;
        if( balance_target_timer >= BALANCE_TARGET_CYCLE_INTERVAL )
        {
            size_t new_index;

            balance_target_timer = 0;

            PORT_PinClear( balance_targets[balance_target_cycle_index].led_target_pin );

            new_index = (size_t)rand() % sizeof(balance_targets)/sizeof(balance_target_t);
            while( new_index == balance_target_cycle_index )
            {
                new_index = (size_t)rand() % sizeof(balance_targets)/sizeof(balance_target_t);
            }
            balance_target_cycle_index = new_index;

            balance_target.x = balance_targets[balance_target_cycle_index].x;
            balance_target.y = balance_targets[balance_target_cycle_index].y;

            PORT_PinSet( balance_targets[balance_target_cycle_index].led_target_pin );
        }

        vTaskDelayUntil( &balance_taskLastWakeTime, configTICK_RATE_HZ / BALANCE_TASK_RATE_HZ );    
    }
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_CMD_DataVisualizer( void )
{
    balance_dv_active = true;

    while( !CMD_CheckEscape() && balance_dv_active )
    {
        vTaskDelay( 10 );
    }

    balance_dv_active = false;
}
