/**
  Command Processor Hardware Abstraction Layer Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    command_hal.h

  @Summary:
    Command Processor hardware abstraction layer interface definition.

  @Description:
    This header file contains the interface definition for hardware used by the Command Processor.
    This header is intended only for internal use by the Command Processor module
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


#ifndef COMMAND_HAL_H_
#define COMMAND_HAL_H_


// ******************************************************************
// Section: Included Files
// ******************************************************************

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************


// ******************************************************************
//  Section: External Interface Definitions
// ******************************************************************

/**
  Summary:
    Check if data is ready to be read from Command Processor input source

  Description:
    This routine will check if data is ready to be read from 
      Command Processor input source.
    This function should be called to check if the receiver 
      is not empty before calling CMD_HAL_IO_Read().

  Preconditions:
    CMD_HAL_Initialize() function should have been called before calling this function.
    
  Parameters:
    None

  Returns:
    True if input buffer is empty, otherwise false

  Comments:
*/
bool CMD_HAL_IO_RxBufferEmpty( void );

/**
  Summary:
    Check if data output buffer is full
    
  Description:
    This routine will check if the output buffer is full.
    This function should be called to check if the transmitter is not
      full before calling CMD_HAL_IO_Write().
    
  Preconditions:
    CMD_HAL_Initialize() function should have been called before calling this function.

  Parameters:
    None

  Returns:
    True if output buffer is full, otherwise false

  Comments:
*/
bool CMD_HAL_IO_TxBufferFull( void );

/**
  Summary:
    Read a byte of data from the Command Processor input source.

  Description:
    This routine reads a byte of data from the Command Processor input source.

  Preconditions:
    CMD_HAL_Initialize() function should have been called before calling this function.
    This function will block on empty receive buffer.  For non-blocking operation,
      CMD_HAL_IO_RxBufferEmpty() can be used to check if any data is ready to be read.

  Parameters:
    None

  Returns:
    A data byte received by the driver.

  Comments:
*/
uint8_t CMD_HAL_IO_Read( void );

 /**
  Summary:
    Writes a byte of data to the Command Processor output.

  Description:
    This routine writes a byte of data to the Command Processor output.

  Preconditions:
    CMD_HAL_Initialize() function should have been called before calling this function.
    This function will block on empty receive buffer. For non-blocking operation, 
      CMD_HAL_IO_TxBufferFull() can be used to check if any data is ready to be read.

  Parameters:
    txData  - Data byte to write to the output

  Returns:
    None

  Comments:
*/
void CMD_HAL_IO_Write( uint8_t txData );


 /**
  Summary
    Start the Command Processor Timer
    
  Description
    This routine starts the Command Processor Timer
    
  Preconditions:
    CMD_HAL_Initialize() function should have been called before calling this function.

  Parameters:
    period_mS - timer period in milliseconds

  Returns
    None

  Comments:
    This function only needs implementation if CMD_ENABLE_STREAM is enabled
*/
void CMD_HAL_TIMER_Start( uint16_t period_ms );

 /**
  Summary
    Stop the Command Processor Timer
    
  Description
    This routine stops the Command Processor Timer
    
  Preconditions:
    CMD_HAL_Initialize() function should have been called before calling this function.

  Parameters:
    None

  Returns
    None

  Comments:
    This function only needs implementation if CMD_ENABLE_STREAM is enabled
*/
void CMD_HAL_TIMER_Stop( void );

 /**
  Summary
    Check the status of the Command Processor Timer
    
  Description
    This routine checks to see if one (or more) period of the Command Processor Timer has elapsed.
    
  Preconditions:
    CMD_HAL_Initialize() function should have been called before calling this function.

  Parameters:
    None

  Returns
    true if timer is expired, otherwise false
    
  Comments:
    This function will clear internal expired status (if set).
    This function only needs implementation if CMD_ENABLE_STREAM is enabled
*/
bool CMD_HAL_TIMER_IsTimerExpired( void );


// ******************************************************************
// Section: Operation Routines
// ******************************************************************

/**
  Summary:
    Initialization routine for Command Processor Hardware Abstraction Layer.

  Description:
    This routine initializes the Command Processor Hardware Abstraction Layer.
    This routine must be called before any other CMD_HAL routine is called.

  Preconditions:
    None.

  Parameters:
    None

  Comments:
    This routine should only be called once during system initialization.
*/
void CMD_HAL_Initialize( void );

/**
  Summary:
    Maintains the Command Processor Hardware Abstraction Layer.

  Description:
    This function is used to maintain the Command Processor hardware abstraction layer.

  Preconditions:
    CMD_HAL_Initialize was successfully run once.

  Parameters:
    None.

  Comments:
    CMD_HAL_Tasks is non-blocking.
*/
void CMD_HAL_Tasks( void );


#ifdef __cplusplus  // Provide C++ Compatibility
}
#endif

#endif
