/*******************************************************************************
  Board Support Package Header File.

  Company:
    Microchip Technology Inc.

  File Name:
    bsp.h

  Summary:
    Board Support Package Header File 

  Description:
    This file contains constants, macros, type definitions and function
    declarations 
*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2023 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

#ifndef BSP_H
#define BSP_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "device.h"
#include "peripheral/port/plib_port.h"

// *****************************************************************************
// *****************************************************************************
// Section: BSP Macros
// *****************************************************************************
// *****************************************************************************
#define SAME51_CURIOSITY_NANO_BASE
#define BOARD_NAME    "SAME51-CURIOSITY-NANO-BASE"

/*** Macros for MIKROBUS1_DIR output pin ***/ 
#define BSP_MIKROBUS1_DIR_PIN        PORT_PIN_PA2
#define BSP_MIKROBUS1_DIR_Get()      ((PORT_REGS->GROUP[0].PORT_IN >> 2U) & 0x01U)
#define BSP_MIKROBUS1_DIR_Set()      (PORT_REGS->GROUP[0].PORT_OUTSET = ((uint32_t)1U << 2U))
#define BSP_MIKROBUS1_DIR_Clear()    (PORT_REGS->GROUP[0].PORT_OUTCLR = ((uint32_t)1U << 2U))
#define BSP_MIKROBUS1_DIR_Toggle()   (PORT_REGS->GROUP[0].PORT_OUTTGL = ((uint32_t)1U << 2U))
#define BSP_MIKROBUS1_DIR_On()       BSP_MIKROBUS1_DIR_Set()
#define BSP_MIKROBUS1_DIR_Off()      BSP_MIKROBUS1_DIR_Clear() 

/*** Macros for MIKROBUS3_DIR output pin ***/ 
#define BSP_MIKROBUS3_DIR_PIN        PORT_PIN_PB8
#define BSP_MIKROBUS3_DIR_Get()      ((PORT_REGS->GROUP[1].PORT_IN >> 8U) & 0x01U)
#define BSP_MIKROBUS3_DIR_Set()      (PORT_REGS->GROUP[1].PORT_OUTSET = ((uint32_t)1U << 8U))
#define BSP_MIKROBUS3_DIR_Clear()    (PORT_REGS->GROUP[1].PORT_OUTCLR = ((uint32_t)1U << 8U))
#define BSP_MIKROBUS3_DIR_Toggle()   (PORT_REGS->GROUP[1].PORT_OUTTGL = ((uint32_t)1U << 8U))
#define BSP_MIKROBUS3_DIR_On()       BSP_MIKROBUS3_DIR_Set()
#define BSP_MIKROBUS3_DIR_Off()      BSP_MIKROBUS3_DIR_Clear() 

/*** Macros for LED output pin ***/ 
#define BSP_LED_PIN        PORT_PIN_PA14
#define BSP_LED_Get()      ((PORT_REGS->GROUP[0].PORT_IN >> 14U) & 0x01U)
#define BSP_LED_Set()      (PORT_REGS->GROUP[0].PORT_OUTSET = ((uint32_t)1U << 14U))
#define BSP_LED_Clear()    (PORT_REGS->GROUP[0].PORT_OUTCLR = ((uint32_t)1U << 14U))
#define BSP_LED_Toggle()   (PORT_REGS->GROUP[0].PORT_OUTTGL = ((uint32_t)1U << 14U))
#define BSP_LED_On()       BSP_LED_Clear()
#define BSP_LED_Off()      BSP_LED_Set() 

/*** Macros for MIKROBUS2_DIR output pin ***/ 
#define BSP_MIKROBUS2_DIR_PIN        PORT_PIN_PB3
#define BSP_MIKROBUS2_DIR_Get()      ((PORT_REGS->GROUP[1].PORT_IN >> 3U) & 0x01U)
#define BSP_MIKROBUS2_DIR_Set()      (PORT_REGS->GROUP[1].PORT_OUTSET = ((uint32_t)1U << 3U))
#define BSP_MIKROBUS2_DIR_Clear()    (PORT_REGS->GROUP[1].PORT_OUTCLR = ((uint32_t)1U << 3U))
#define BSP_MIKROBUS2_DIR_Toggle()   (PORT_REGS->GROUP[1].PORT_OUTTGL = ((uint32_t)1U << 3U))
#define BSP_MIKROBUS2_DIR_On()       BSP_MIKROBUS2_DIR_Set()
#define BSP_MIKROBUS2_DIR_Off()      BSP_MIKROBUS2_DIR_Clear() 


/*** Macros for BUTTON input pin ***/ 
#define BSP_BUTTON_PIN                    PORT_PIN_PA15
#define BSP_BUTTON_Get()                  ((PORT_REGS->GROUP[0].PORT_IN >> 15U) & 0x01U)
#define BSP_BUTTON_STATE_PRESSED          1
#define BSP_BUTTON_STATE_RELEASED         0


// *****************************************************************************
// *****************************************************************************
// Section: Interface Routines
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Function:
    void BSP_Initialize(void)

  Summary:
    Performs the necessary actions to initialize a board

  Description:
    This function initializes the LED and Switch ports on the board.  This
    function must be called by the user before using any APIs present on this
    BSP.

  Precondition:
    None.

  Parameters:
    None

  Returns:
    None.

  Example:
    <code>
    BSP_Initialize();
    </code>

  Remarks:
    None
*/

void BSP_Initialize(void);

#endif // BSP_H

/*******************************************************************************
 End of File
*/