/**
  Command Processor Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    command.h

  @Summary:
    Command Processor interface definition.

  @Description:
    This header file contains the interface definition for the Command Processor.
    It provides a way to interact with the Command Processor subsystem to manage
    the ASCII command requests from the user.
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


#ifndef COMMAND_H_
#define COMMAND_H_


// ******************************************************************
// Section: Included Files
// ******************************************************************

#include <stdbool.h>
#include <stdint.h>

#include "command_config.h"


#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

/**
  Summary:
    COMMAND Function type definition

  Description:
    Command Process Function API. This function type definition identifies
    the interface structure of the command process function API.
    
  Comments:
*/
typedef void (*cmd_function_t)( void );


// ******************************************************************
//  Section: External Interface Definitions
// ******************************************************************

/**
  Summary:
    Register a command with the Command Processor

  Description:
    This function will register a command string / command function pair with
      the Command Processor.

  Preconditions:
    None.

  Parameters:
    const char* string      - null-terminated string to be output by command processor
    cmd_function_t function - function to be called in response to command string

  Returns:
    bool - true on success. otherwise false (command list full or invalid parameter)

  Comments:
    Command string may not contain the space character as it is used as a delimter by
      the Command Processor.
    There is no check for duplicate strings in the command list. Only the first
      registered instance of a command string will be recognized.
*/
bool CMD_RegisterCommand( const char* string, cmd_function_t function );


/**
  Summary:
    Outputs a null-terminated string. 

  Description:
    This function request the command processor to output a null-terminated string.

  Preconditions:
    None.

  Parameters:
    const char* string - null-terminated string to be output by command processor
    bool block         - FALSE, function will not block, returning as soon as output
                            buffer is full.
                         TRUE, function will block until entire string has been queued
                            for output

  Returns:
    char* - If non-blocking and output buffer full, pointer to remainder
                     of string not yet queued for output.
                  Otherwise, NULL

  Comments:
    This function will do nothing when command processor is password locked.
*/
char* CMD_PrintString( const char* string, bool block );


/**
  Summary:
    Outputs an array of bytes

  Description:
    This function request the command processor to output array of bytes.

  Preconditions:
    None.

  Parameters:
    const uint8_t* bytes - array of bytes to be output by command processor
    length - number of bytes in byte array
    bool block - FALSE, function will not block, returning as soon as output
                        buffer is full.
                 TRUE, function will block until entire string has been queued
                       for output

  Returns:
    uint8_t* - If non-blocking and output buffer full, pointer to remainder
                     of array not yet queued for output.
                  Otherwise, NULL

  Comments:
    This function will do nothing when command processor is password locked.
*/
uint8_t* CMD_PrintByteArray( const uint8_t* bytes, uint16_t length, bool block );


void CMD_PrintHex_U8( uint8_t value, bool block );
void CMD_PrintHex_U16( uint16_t value, bool block );
void CMD_PrintHex_U32( uint32_t value, bool block );

void CMD_PrintDecimal_U32( uint32_t value, bool zero_blank, uint8_t width, bool block );

void CMD_PrintDecimal_S32( int32_t value, bool zero_blank, uint8_t width, bool block );

void CMD_PrintFixedPoint_U32( uint32_t value, uint8_t fractional_bits,
							  bool zero_blank, uint8_t width, uint8_t precision, bool block );

void CMD_PrintFixedPoint_S32( int32_t value, uint8_t fractional_bits,
							  bool zero_blank, uint8_t width,uint8_t precision, bool block );

void CMD_PrintFloat( float value, uint8_t precision, bool block );


/**
  Summary:
    Gets the argument count (argc) of processed command.

  Description:
    This function returns the argument count of the processed command.  This count
    includes the command string (i.e argc will always be >= 1).

  Preconditions:
    None.

  Parameters:
    None

  Returns
    count of arguments

  Comments:
    This routine should only be called by a command function (CMD_FNC)
      when being executed from Command Processor
*/
#if (CMD_ENABLE_ARGS == 1)
uint8_t CMD_GetArgc( void );
#endif


/**
  Summary:
    Gets the requested argument value 

  Description:
    This function copies the requested argument value into a user supplied buffer.
    argv_index = 0     : will always return the command itself.
    argv_index >= argc : will return with empty string in buffer.

  Preconditions:
    None

  Parameters:
    argv_index  - index of argument to retrieve
    buffer      - char buffer to hold copy of requested argument
    buffer_size - size of argument copy buffer

  Returns:
    string length of retrieved argument

  Comments:
    This routine should only be called by a command function (CMD_FNC)
      when being executed from Command Processor
*/
#if (CMD_ENABLE_ARGS == 1)
uint8_t CMD_GetArgv( uint8_t argv_index, char* buffer, uint8_t buffer_size );
#endif


bool CMD_CheckEscape(void);


// ******************************************************************
// Section: Operation Routines
// ******************************************************************

/**
  Summary:
    Initializes data for the instance of the Command Processor module.

  Description:
    This function initializes the Command Processor module.
    It also initializes any internal data structures.

  Preconditions:
    None.

  Parameters:
    None

  Returns:
    None

  Comments:
    This routine should only be called once during system initialization.
*/
void CMD_Initialize( void );


/**
  Summary:
    Maintains the Command Processor internal state machine.

  Description:
    This function is used to maintain the Command Processor internal state machine.

  Preconditions:
    CMD_Initialize was successfully run once.

  Parameters:
    None.

  Comments:
    CMD_Tasks is non-blocking except when executing a command. It is up to the
      application defined commands to ensure that they do not overly interfere
      with timing of other processes.
*/
void CMD_Task( void );


#ifdef __cplusplus  // Provide C++ Compatibility
}
#endif

#endif
