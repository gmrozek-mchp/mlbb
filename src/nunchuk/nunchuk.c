// ******************************************************************
// Section: Included Files
// ******************************************************************
#include "nunchuk.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "driver/driver_i2c.h"

#include "command/command.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define NUNCHUK_RTOS_PRIORITY       (2)
#define NUNCHUK_RTOS_STACK_SIZE     (configMINIMAL_STACK_SIZE)

#define NUNCHUK_I2C_ADDRESS         (0x52)
#define NUNCHUK_READ_BUFFER_SIZE    (6)

#define NUNCHUK_POWER_UP_DELAY_mS   (100)
#define NUNCHUK_SCAN_RATE_HZ        (200)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static TaskHandle_t nunchuk_taskHandle = NULL;
static StaticTask_t nunchuk_taskBuffer;
static StackType_t  nunchuk_taskStack[ NUNCHUK_RTOS_STACK_SIZE ];

static TickType_t nunchuk_taskLastWakeTime;

static uint8_t nunchuk_readBuffer[NUNCHUK_READ_BUFFER_SIZE];

static nunchuk_data_t nunchuk_data_raw;
static int16_t nunchuk_joystick_x_zero;
static int16_t nunchuk_joystick_y_zero;

static nunchuk_data_callback_t nunchuk_dataCallback = NULL;

static const uint8_t nunchuk_cmd_init1[] = { 0xF0, 0x55 };
static const uint8_t nunchuk_cmd_init2[] = { 0xFB, 0x00 };
static const uint8_t nunchuk_cmd_read[] = { 0x00 };


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void NUNCHUK_RTOS_Task( void * pvParameters );

static void NUNCHUK_SendConfigSequence( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void NUNCHUK_Initialize( void )
{
    nunchuk_data_raw.button_c = false;
    nunchuk_data_raw.button_z = false;
    nunchuk_data_raw.joystick_x = 0;
    nunchuk_data_raw.joystick_y = 0;

    nunchuk_joystick_x_zero = 0;
    nunchuk_joystick_y_zero = 0;
    
    CMD_RegisterCommand( "nunchuk", NUNCHUK_CMD_Print_Data );

    nunchuk_taskHandle = xTaskCreateStatic(
        NUNCHUK_RTOS_Task,        /* Function that implements the task. */
        "Nunchuk",                /* Text name for the task. */
        NUNCHUK_RTOS_STACK_SIZE,  /* Number of indexes in the stack. */
        NULL,                     /* Parameter passed into the task. */
        NUNCHUK_RTOS_PRIORITY,    /* Priority at which the task is created. */
        nunchuk_taskStack,         /* Array to use as the task's stack. */
        &nunchuk_taskBuffer        /* Variable to hold the task's data structure. */
    );
}

void NUNCHUK_DataCallback_Register( nunchuk_data_callback_t callback )
{
    nunchuk_dataCallback = callback;
}

nunchuk_data_t NUNCHUK_Data_Get( void )
{
    nunchuk_data_t returnValue;
    
    taskENTER_CRITICAL();
    
    returnValue.button_c = nunchuk_data_raw.button_c;
    returnValue.button_z = nunchuk_data_raw.button_z;
    returnValue.joystick_x = nunchuk_data_raw.joystick_x - nunchuk_joystick_x_zero;
    returnValue.joystick_y = nunchuk_data_raw.joystick_y - nunchuk_joystick_y_zero;
    
    taskEXIT_CRITICAL();

    return returnValue;
}

void NUNCHUK_Zero_Set( void )
{
    taskENTER_CRITICAL();
    
    nunchuk_joystick_x_zero = nunchuk_data_raw.joystick_x;
    nunchuk_joystick_x_zero = nunchuk_data_raw.joystick_y;

    taskEXIT_CRITICAL();
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void NUNCHUK_RTOS_Task( void * pvParameters )
{
    (void)pvParameters;

    // Nunchuk needs some time to power up
    vTaskDelay( pdMS_TO_TICKS(NUNCHUK_POWER_UP_DELAY_mS) );
    
    NUNCHUK_SendConfigSequence();
    
    nunchuk_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
        DRIVER_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_read, sizeof(nunchuk_cmd_read) );

        vTaskDelay(1);  // Nunchuk communications need delay between commands    

        DRIVER_I2C_Read( NUNCHUK_I2C_ADDRESS, nunchuk_readBuffer, sizeof(nunchuk_readBuffer) );

        taskENTER_CRITICAL();

        nunchuk_data_raw.button_c = ((nunchuk_readBuffer[5] & 0x02) == 0);
        nunchuk_data_raw.button_z = ((nunchuk_readBuffer[5] & 0x01) == 0);
        nunchuk_data_raw.joystick_x = nunchuk_readBuffer[0];
        nunchuk_data_raw.joystick_y = nunchuk_readBuffer[1];

        taskEXIT_CRITICAL();

        if( nunchuk_dataCallback != NULL )
        {
            nunchuk_data_t nunchuk_data;

            nunchuk_data.button_c = nunchuk_data_raw.button_c;
            nunchuk_data.button_z = nunchuk_data_raw.button_z;
            nunchuk_data.joystick_x = nunchuk_data_raw.joystick_x - nunchuk_joystick_x_zero;
            nunchuk_data.joystick_y = nunchuk_data_raw.joystick_y - nunchuk_joystick_y_zero;
        
            nunchuk_dataCallback( nunchuk_data );
        }

        vTaskDelayUntil( &nunchuk_taskLastWakeTime, configTICK_RATE_HZ / NUNCHUK_SCAN_RATE_HZ );    

        if( (nunchuk_readBuffer[0] == 0xFF) && (nunchuk_readBuffer[1] == 0xFF) )
        {
            // Bad data. Try to configure nunchuk again. 
            NUNCHUK_SendConfigSequence();
        }
    }
}

static void NUNCHUK_SendConfigSequence( void )
{
//    vTaskDelay(100);  // Nunchuk needs time to start up.

    DRIVER_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_init1, sizeof(nunchuk_cmd_init1) );

    vTaskDelay(1);  // Nunchuk communications need delay between commands
    
    DRIVER_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_init2, sizeof(nunchuk_cmd_init2) );
    
    vTaskDelay(1);  // Nunchuk communications need delay between commands    
}
