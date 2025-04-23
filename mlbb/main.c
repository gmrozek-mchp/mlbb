/**
 * @file main.c
 * @author greg
 * @date 2025-04-23
 * @brief Main function
 */

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "command/command.h"
 
int main()
{
    BSP_Initialize();

    CMD_Initialize();

    vTaskStartScheduler();

    return 0;
}
