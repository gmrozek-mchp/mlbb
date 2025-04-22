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


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define NUNCHUK_RTOS_PRIORITY       (2)
#define NUNCHUK_RTOS_STACK_SIZE     (configMINIMAL_STACK_SIZE)

#define NUNCHUK_I2C_ADDRESS         (0x52)
#define NUNCHUK_READ_BUFFER_SIZE    (6)

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

static nunchuk_data_t nunchuk_data;

static nunchuk_data_callback_t nunchuk_dataCallback = NULL;

static const uint8_t nunchuk_cmd_init1[] = { 0xF0, 0x55 };
static const uint8_t nunchuk_cmd_init2[] = { 0xFB, 0x00 };
static const uint8_t nunchuk_cmd_read[] = { 0x00 };


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void NUNCHUK_RTOS_Task( void * pvParameters );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void NUNCHUK_Initialize( void )
{
    nunchuk_data.button_c = false;
    nunchuk_data.button_z = false;
    nunchuk_data.joystick_x = 0;
    nunchuk_data.joystick_y = 0;
    
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
    
    returnValue = nunchuk_data;
    
    taskEXIT_CRITICAL();

    return returnValue;
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void NUNCHUK_RTOS_Task( void * pvParameters )
{       
    DRIVER_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_init1, sizeof(nunchuk_cmd_init1) );

    vTaskDelay(1);  // Nunchuk communications need delay between commands
    
    DRIVER_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_init2, sizeof(nunchuk_cmd_init2) );
    
    vTaskDelay(1);  // Nunchuk communications need delay between commands

    nunchuk_taskLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
        DRIVER_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_read, sizeof(nunchuk_cmd_read) );

        vTaskDelay(1);  // Nunchuk communications need delay between commands    

        DRIVER_I2C_Read( NUNCHUK_I2C_ADDRESS, nunchuk_readBuffer, sizeof(nunchuk_readBuffer) );

        taskENTER_CRITICAL();
                
        nunchuk_data.button_c = ((nunchuk_readBuffer[5] & 0x02) == 0);
        nunchuk_data.button_z = ((nunchuk_readBuffer[5] & 0x01) == 0);
        nunchuk_data.joystick_x = nunchuk_readBuffer[0];
        nunchuk_data.joystick_y = nunchuk_readBuffer[1];

        taskEXIT_CRITICAL();
        
        if( nunchuk_dataCallback != NULL )
        {
            nunchuk_dataCallback( nunchuk_data );
        }

        vTaskDelayUntil( &nunchuk_taskLastWakeTime, configTICK_RATE_HZ / NUNCHUK_SCAN_RATE_HZ );    
    }
}
