// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance.h"

#include <stdlib.h>

#include "arm_math_types.h"

#include "FreeRTOS.h"
#include "nunchuk/nunchuk.h"
#include "platform/platform.h"
#include "task.h"

#include "peripheral/port/plib_port.h"

#include "command/command.h"

#include "nunchuk/nunchuk.h"

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
#define BALANCE_TARGET_CIRCLE_INCREMENT (50)

#define BALANCE_NUNCHUK_DEBOUNCE_COUNT  (10)


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

static balance_mode_t machine_balance_mode = BALANCE_MODE_PID;

static balance_target_t balance_target;
static size_t balance_target_cycle_index = 0;

static bool balance_dv_active = false;

static void BALANCE_OFF_Reset( void );
static const balance_interface_t balancer_off = {
    .init = NULL,
    .reset = BALANCE_OFF_Reset,
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
    { .x = 0x7E0, .y = 0x810, .led_target_pin = LED_TARGET_CENTER_PIN },
    { .x = 0x4C8, .y = 0xBC0, .led_target_pin = LED_TARGET_TOP_RIGHT_PIN },
    { .x = 0xB20, .y = 0xBB8, .led_target_pin = LED_TARGET_TOP_LEFT_PIN },
    { .x = 0xB18, .y = 0x470, .led_target_pin = LED_TARGET_BOTTOM_LEFT_PIN },
    { .x = 0x4D0, .y = 0x468, .led_target_pin = LED_TARGET_BOTTOM_RIGHT_PIN }
};

static q15_t balance_target_circle_degrees = 0;


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
    return machine_balance_mode;
}

void BALANCE_MODE_Set( balance_mode_t mode )
{
    if( mode >= BALANCE_MODE_NUM_MODES )
    {
        return;
    }

    machine_balance_mode = mode;
}

void BALANCE_MODE_Next( void )
{
    if( machine_balance_mode >= BALANCE_MODE_NUM_MODES - 1 )
    {
        machine_balance_mode = 0;
    }
    else
    {
        machine_balance_mode++;
    }
}

void BALANCE_MODE_Previous( void )
{
    if( machine_balance_mode == 0 )
    {
        machine_balance_mode = BALANCE_MODE_NUM_MODES - 1;
    }
    else
    {
        machine_balance_mode--;
    }
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void BALANCE_RTOS_Task( void * pvParameters )
{
    balance_mode_t active_balance_mode = BALANCE_MODE_INVALID;
    balance_mode_t pending_balance_mode = BALANCE_MODE_INVALID;

    uint32_t balance_target_timer = 0;

    bool balance_nunchuk_state_c = false;
    uint32_t balance_nunchuk_debounce_c = 0;

    bool balance_nunchuk_state_z = false;
    uint32_t balance_nunchuk_debounce_z = 0;

    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(BALANCE_POWER_UP_DELAY_mS) );

    balance_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
        nunchuk_data_t nunchuk = NUNCHUK_Data_Get();

        if( nunchuk.button_c != balance_nunchuk_state_c )
        {
            balance_nunchuk_debounce_c++;
            if( balance_nunchuk_debounce_c >= BALANCE_NUNCHUK_DEBOUNCE_COUNT )
            {
                balance_nunchuk_state_c = nunchuk.button_c;

                if( balance_nunchuk_state_c )
                {
                    switch( machine_balance_mode )
                    {
                        case BALANCE_MODE_OFF:
                        {
                            machine_balance_mode = BALANCE_MODE_PID;
                            break;
                        }
                        case BALANCE_MODE_PID:
                        {
                            machine_balance_mode = BALANCE_MODE_OFF;
                            break;
                        }
                        case BALANCE_MODE_NN:
                        {
                            machine_balance_mode = BALANCE_MODE_PID;
                            break;
                        }
                        default:
                        {
                            machine_balance_mode = BALANCE_MODE_OFF;
                        }                        
                    }
                }
            }
        }
        else 
        {
            balance_nunchuk_debounce_c = 0;
        }

        if( nunchuk.button_z != balance_nunchuk_state_z )
        {
            balance_nunchuk_debounce_z++;
            if( balance_nunchuk_debounce_z >= BALANCE_NUNCHUK_DEBOUNCE_COUNT )
            {
                balance_nunchuk_state_z = nunchuk.button_z;
            }
        }
        else 
        {
            balance_nunchuk_debounce_z = 0;
        }

        if( balance_nunchuk_state_z )
        {
            pending_balance_mode = BALANCE_MODE_HUMAN;
        }
        else
        {
            pending_balance_mode = machine_balance_mode;
        }

        // Check if balance mode has changed.
        if( active_balance_mode != pending_balance_mode )
        {
            balance_mode_t balancer_index;

            active_balance_mode = pending_balance_mode;

            // Turn off all balance mode indicators.
            for( balancer_index = 0; balancer_index < BALANCE_MODE_NUM_MODES; balancer_index++ )
            {
                if( balancers[balancer_index].led_mode_pin != PORT_PIN_NONE )
                {
                    PORT_PinClear( balancers[balancer_index].led_mode_pin );
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
            balancers[active_balance_mode].run( balance_target.x, balance_target.y );
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

static void BALANCE_OFF_Reset( void )
{
    PLATFORM_Position_XY_Set( 0, 0 );
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
