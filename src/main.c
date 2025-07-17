/**
 * @file main.c
 * @author greg
 * @date 2025-04-23
 * @brief Main function
 */

#include "bsp.h"

#include "FreeRTOS.h"   // IWYU pragma: keep - FreeRTOS.h must be included before other FreeRTOS headers
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
