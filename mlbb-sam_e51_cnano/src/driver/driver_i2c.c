// ******************************************************************
// Section: Included Files
// ******************************************************************
#include "driver_i2c.h"

#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "peripheral/sercom/i2c_master/plib_sercom2_i2c_master.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define DRIVER_SERCOM_TIMEOUT_mS    (100)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static SemaphoreHandle_t DRIVER_I2C_SERCOM_Mutex = NULL;
static StaticSemaphore_t DRIVER_I2C_SERCOM_MutexBuffer;

static TaskHandle_t DRIVER_I2C_TaskToNotify = NULL;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void DRIVER_SERCOM_I2C_Callback( uintptr_t context );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void DRIVER_I2C_Initialize( void )
{
    DRIVER_I2C_SERCOM_Mutex = xSemaphoreCreateMutexStatic( &DRIVER_I2C_SERCOM_MutexBuffer );

    SERCOM2_I2C_CallbackRegister( DRIVER_SERCOM_I2C_Callback, (uintptr_t)NULL );
}

void DRIVER_I2C_Write(uint16_t address, uint8_t* wrData, uint32_t wrLength)
{
    while( xSemaphoreTake(DRIVER_I2C_SERCOM_Mutex, pdMS_TO_TICKS(DRIVER_SERCOM_TIMEOUT_mS)) != pdTRUE )
    {
        // Unable to obtain mutex
    }
    
    SERCOM2_I2C_Write( address, wrData, wrLength );
    
    DRIVER_I2C_TaskToNotify = xTaskGetCurrentTaskHandle();
    
    (void)ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(DRIVER_SERCOM_TIMEOUT_mS) );        
    
    xSemaphoreGive( DRIVER_I2C_SERCOM_Mutex );    
}

void DRIVER_I2C_Read(uint16_t address, uint8_t* rdData, uint32_t rdLength)
{
    while( xSemaphoreTake(DRIVER_I2C_SERCOM_Mutex, pdMS_TO_TICKS(DRIVER_SERCOM_TIMEOUT_mS)) != pdTRUE )
    {
        // Unable to obtain mutex
    }
    
    SERCOM2_I2C_Read( address, rdData, rdLength );
    
    DRIVER_I2C_TaskToNotify = xTaskGetCurrentTaskHandle();
    
    (void)ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(DRIVER_SERCOM_TIMEOUT_mS) );
    
    xSemaphoreGive( DRIVER_I2C_SERCOM_Mutex );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void DRIVER_SERCOM_I2C_Callback( uintptr_t context )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    (void)context;
    
    if( DRIVER_I2C_TaskToNotify != NULL )
    {
        vTaskNotifyGiveFromISR( DRIVER_I2C_TaskToNotify, &xHigherPriorityTaskWoken );
    }
    
    DRIVER_I2C_TaskToNotify = NULL;
 
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
