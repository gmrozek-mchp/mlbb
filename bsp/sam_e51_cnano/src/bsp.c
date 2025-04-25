#include "bsp.h"

#include "definitions.h"
#include "driver/driver_i2c.h"

void BSP_Initialize( void )
{
    SYS_Initialize ( NULL );

    DRIVER_I2C_Initialize();
}
