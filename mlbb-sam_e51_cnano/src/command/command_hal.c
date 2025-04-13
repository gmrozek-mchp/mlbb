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

#include "peripheral/sercom/usart/plib_sercom5_usart.h"
//#include "peripheral/tc/plib_tc0.h"

#include "command_hal.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define CMD_HAL_TIMER_RESOLUTION_mS   1

// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
// Section: Private Variables
// ******************************************************************
//static uint16_t timerPeriod_mS = 0;
//static uint16_t remainingTime_mS = 0;
//static bool timerExpired = false;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
//static void CMD_HAL_TIMER_InterruptHandler(TC_TIMER_STATUS status, uintptr_t context);


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void CMD_HAL_Initialize( void )
{
//    TC0_TimerStop();
//    TC0_TimerCallbackRegister( CMD_HAL_TIMER_InterruptHandler, (uintptr_t)NULL );
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


//void CMD_HAL_TIMER_Start( uint16_t period_ms )
//{
//    timerPeriod_mS = period_ms;
//    remainingTime_mS = period_ms;
//    timerExpired = false;
//    TC0_Timer16bitCounterSet(0);
//    TC0_TimerStart();
//}
//
//void CMD_HAL_TIMER_Stop( void )
//{
//    TC0_TimerStop();
//}
//
//bool CMD_HAL_TIMER_IsTimerExpired( void )
//{
//    bool returnValue = timerExpired;
//    
//    if( returnValue )
//    {
//        timerExpired = false;
//    }
//    
//    return returnValue;
//}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

//static void CMD_HAL_TIMER_InterruptHandler(TC_TIMER_STATUS status, uintptr_t context)
//{
//    if( remainingTime_mS >= CMD_HAL_TIMER_RESOLUTION_mS )
//    {
//        remainingTime_mS -= CMD_HAL_TIMER_RESOLUTION_mS;
//    }
//    else
//    {
//        remainingTime_mS += timerPeriod_mS;
//        timerExpired = true;
//    }
//}


/**
  End of File
*/
