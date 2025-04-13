/**
  Command Processor Configuration Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    command_config.h

  @Summary:
    Command Processor configuration settings.

  @Description:
    This header file contains application specific configuration settings used
    by the Command Processor subsystem.
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


#ifndef COMMAND_CONFIG_H_
#define COMMAND_CONFIG_H_


// ******************************************************************
// Section: COMMAND Processor Functionality Enable flags
// ******************************************************************
#define CMD_ENABLE_ARGS                 1
#define CMD_ENABLE_HELP                 1
#define CMD_ENABLE_HISTORY              1
#define CMD_ENABLE_STREAM               0
#define CMD_ENABLE_PASSWORD             0


// ******************************************************************
// Section: COMMAND Processor Data Types and Definitions
// ******************************************************************

/**
  Summary:
    Command Processor Buffer Maximum Length definition.

  Description:
    This macro defines the maximum length of the command buffer.

  Comments:
    CMD_BUFFER_SIZE cannot exceed 256.
*/
#define CMD_BUFFER_SIZE  64


/**
  Summary:
    Command strings for built-in command support

  Description:
    These command strings are used by the Command Processor for support of
    built-in commands.

  Comments:
    Availability of these commands may depend on enabled functionality.
*/
#define CMD_HELP_COMMAND_STR        "?"
#define CMD_UNLOCK_COMMAND_STR      "unlock"
#define CMD_LOCK_COMMAND_STR        "lock"
#define CMD_STREAM_COMMAND_STR      "stream"


/**
  Summary:
    Interface strings used by Command Processor .

  Description:
    These definitions are the terminal messages used by Command Processor.

  Comments:
    Not all interface strings may be necessary depending on enabled functionality.
    Unwanted interface strings may be safely defined as NULL
*/
#define CMD_INITIALIZE_STR          "\r\n" \
                                    "\r\n" \
                                    "Microchip Lightweight Command Processor\r\n"

#define CMD_COMMAND_PROMPT_STR      "> "

#define CMD_COMMAND_NOT_FOUND_STR   "Enter '?' for list of commands\r\n"

#define CMD_NO_STREAM_MESSAGE_STR   "Invalid previous command\r\n"
#define CMD_NO_TICK_MESSAGE_STR     "Command requires repeat interval\r\n"

#define CMD_LOCK_MESSAGE_STR        "LOCKED\r\n"

#define CMD_LINE_TERMINATOR_STR     "\r\n"


#define CMD_DECIMAL_SEPERATOR_CHAR	'.'


#endif
