/**
  Command Processor Hardware Abstraction Layer implementation file

  @Company:
    Microchip Technology Inc.

  @File Name:
    command_hal.c

  @Summary:
    This is the implementation file for Command Processor Hardware Abstraction Layer

  @Description:
    This file contains the implementation for the Command Processor Hardware Abstraction Layer.
*/

/*
    (c) 2017 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/


// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "peripheral/sercom/usart/plib_sercom5_usart.h"

#include "command.h"
#include "command_hal.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define CMD_HAL_TIMER_RESOLUTION_mS   portTICK_PERIOD_MS

#define CMD_HAL_RTOS_PRIORITY       1
#define CMD_HAL_RTOS_STACK_SIZE     200


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static TaskHandle_t cmdTaskHandle = NULL;
static StaticTask_t cmdTaskBuffer;
static StackType_t cmdTaskStack[ CMD_HAL_RTOS_STACK_SIZE ];

static TimerHandle_t cmdTimerHandle = NULL;
static StaticTimer_t cmdTimerBuffer;
static bool cmdTimerExpired = false;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void CMD_HAL_RTOS_Task( void * pvParameters );
static void CMD_HAL_TIMER_Callback( TimerHandle_t xTimer );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void CMD_HAL_Initialize( void )
{
    cmdTaskHandle = xTaskCreateStatic(
        CMD_HAL_RTOS_Task,        /* Function that implements the task. */
        "Command",                /* Text name for the task. */
        CMD_HAL_RTOS_STACK_SIZE,  /* Number of indexes in the stack. */
        NULL,                     /* Parameter passed into the task. */
        CMD_HAL_RTOS_PRIORITY,    /* Priority at which the task is created. */
        cmdTaskStack,             /* Array to use as the task's stack. */
        &cmdTaskBuffer            /* Variable to hold the task's data structure. */
    );

    cmdTimerHandle = xTimerCreateStatic(
        "Command",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        CMD_HAL_TIMER_Callback,
        &cmdTimerBuffer
    );
}

void CMD_HAL_Tasks( void )
{
}


bool CMD_HAL_IO_RxBufferEmpty(void)
{
    return (SERCOM5_USART_ReadCountGet() == 0);
}

bool CMD_HAL_IO_TxBufferFull(void)
{
    return (SERCOM5_USART_WriteFreeBufferCountGet() == 0);
}

uint8_t CMD_HAL_IO_Read(void)
{
    uint8_t byte;

    (void)SERCOM5_USART_Read( &byte, 1 );

    return byte;
}

void CMD_HAL_IO_Write(uint8_t txData)
{
    (void)SERCOM5_USART_Write( &txData, 1 );
}


void CMD_HAL_TIMER_Start( uint16_t period_ms )
{
    (void)xTimerChangePeriod( cmdTimerHandle, pdMS_TO_TICKS(period_ms), portMAX_DELAY );
    (void)xTimerStart( cmdTimerHandle, portMAX_DELAY );
}

void CMD_HAL_TIMER_Stop( void )
{
    (void)xTimerStop( cmdTimerHandle, portMAX_DELAY );
}

bool CMD_HAL_TIMER_IsTimerExpired( void )
{
    bool expired = cmdTimerExpired;

    cmdTimerExpired = false;
    
    return expired;
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void CMD_HAL_RTOS_Task( void * pvParameters )
{
    (void)pvParameters;
    
    while(1)
    {
        CMD_Task();
        
        vTaskDelay(10);
    }
}

static void CMD_HAL_TIMER_Callback( TimerHandle_t xTimer )
{
    (void)xTimer;
    
    cmdTimerExpired = true;
}

/**
  End of File
*/
