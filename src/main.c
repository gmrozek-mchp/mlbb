/**
 * @file main.c
 * @author greg
 * @date 2025-04-23
 * @brief Main function
 */

#include "bsp.h"
#include "peripheral/port/plib_port.h"
#include "system/reset/sys_reset.h"

#include "FreeRTOS.h"
#include "task.h"

#include "command/command.h"

#include "balance/balance.h"


int main()
{
    BSP_Initialize();

    CMD_Initialize();

    BALANCE_Initialize();

    vTaskStartScheduler();

    return 0;
}
