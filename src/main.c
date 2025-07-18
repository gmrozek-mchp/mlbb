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
#include "ball/ball.h"
#include "nunchuk/nunchuk.h"
#include "platform/platform.h"


int main()
{
    BSP_Initialize();

    CMD_Initialize();
    
    BALANCE_Initialize();
    BALL_Initialize();
    NUNCHUK_Initialize();
    PLATFORM_Initialize();

    vTaskStartScheduler();

    return 0;
}
