/**
  Command List for Command Processor

  @Company
    Microchip Technology Inc.

  @File Name
    command_list.c

  @Summary
    This is the implementation file for Application defined commands to be
    supported by the Command Processor subsystem.

  @Description:
    This file contains an example list of application defined commands to be
      supported by the Command Processor.  Note that the cmd_command_list[] and 
      cmd_command_list_size variables are the only pieces required by the
      Command Processor subsystem.  Where these variables are defined is
      irrelevant, so long as they are available to the Command Processor modules
      at link time.  All of the other functions in this file are examples of
      application defined commands.  
    Note that all of these pieces are contained in one file only for purpose
      of example.  A real application will likely have these command 
      implementations separated into multiple application modules as appropriate.
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

#include "command.h"

#include "system/reset/sys_reset.h"

#include "bsp/bsp.h"

#include "ball/ar1100.h"


// ******************************************************************
// COMMAND FUNCTION DECLARATIONS
// ******************************************************************

void ForceReset(void);
void SetLED(void);


// ******************************************************************
// COMMAND DESCRIPTORS
// ******************************************************************

static const cmd_descriptor_t reset_command = 
{ 
    "reset",
    ForceReset
};

static const cmd_descriptor_t led_command = 
{ 
    "led",
    SetLED
};

static const cmd_descriptor_t touch_command = 
{ 
    "touch",
    AR1100_CMD_Print_TouchData
};


// ******************************************************************
// COMMAND LIST
// ******************************************************************

const cmd_descriptor_t* const cmd_command_list[] =
{
    &touch_command,
    &led_command,
    &reset_command,    
};
const uint8_t cmd_command_list_size = (sizeof(cmd_command_list) / sizeof(cmd_command_list[0]));


// ******************************************************************
// COMMAND FUNCTION DEFINITIONS
// ******************************************************************

void SetLED( void )
{
    if( CMD_GetArgc() >= 2 )
    {
        char state;
        CMD_GetArgv( 1, &state, 1);
        
        if( state == '0' )
        {
            BSP_LED_Off();
        }
        else
        {
            BSP_LED_On();
        }
    }
}

void ForceReset( void )
{
    SYS_RESET_SoftwareReset();
}
