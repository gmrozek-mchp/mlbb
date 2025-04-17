#include "nunchuk.h"

#include <xc.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "peripheral/sercom/i2c_master/plib_sercom2_i2c_master.h"


#define NUNCHUK_I2C_ADDRESS          (0x52)
#define NUNCHUK_READ_BUFFER_SIZE     (6)

static uint8_t nunchuk_readBuffer[NUNCHUK_READ_BUFFER_SIZE];

// Set up ping-pong buffer to store most recent touch data. Using buffer
// will ensure consistent data for NUNCHUK_Data_Get(), even if it is
// interrupted with reception of a fresh data packet.
static volatile nunchuk_data_t nunchuk_data[2];
static volatile uint8_t nunchuk_data_readIndex = 0;

static nunchuk_data_callback_t nunchuk_dataCallback = NULL;

static const uint8_t nunchuk_cmd_init1[] = { 0xF0, 0x55 };
static const uint8_t nunchuk_cmd_init2[] = { 0xFB, 0x00 };
static const uint8_t nunchuk_cmd_read[] = { 0x00 };


static void NUNCHUK_I2C_Callback( uintptr_t context );


void NUNCHUK_Initialize( void )
{
    nunchuk_data[0].button_c = false;
    nunchuk_data[0].button_z = false;
    nunchuk_data[0].joystick_x = 0;
    nunchuk_data[0].joystick_y = 0;
    
    nunchuk_data[1].button_c = false;
    nunchuk_data[1].button_z = false;
    nunchuk_data[1].joystick_x = 0;
    nunchuk_data[1].joystick_y = 0;
    
    SERCOM2_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_init1, sizeof(nunchuk_cmd_init1) );
    while( SERCOM2_I2C_IsBusy() );

    for(uint32_t i=0; i<10000; i++)
    {
        Nop();
    }
    
    SERCOM2_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_init2, sizeof(nunchuk_cmd_init2) );
    while( SERCOM2_I2C_IsBusy() );

    for(uint32_t i=0; i<10000; i++)
    {
        Nop();
    }
    
    nunchuk_data_readIndex = 0;    
    SERCOM2_I2C_CallbackRegister( NUNCHUK_I2C_Callback, (uintptr_t)NULL );

    SERCOM2_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_read, sizeof(nunchuk_cmd_read) );
}

void NUNCHUK_DataCallback_Register( nunchuk_data_callback_t callback )
{
    nunchuk_dataCallback = callback;
}


nunchuk_data_t NUNCHUK_Data_Get( void )
{
    return nunchuk_data[nunchuk_data_readIndex];
}


static void NUNCHUK_I2C_Callback( uintptr_t context )
{
    static bool read_nwrite = true;
    
    (void)context;
    
    if( read_nwrite )
    {
        for(uint32_t i=0; i<10000; i++)
        {
            Nop();
        }

        SERCOM2_I2C_Read( NUNCHUK_I2C_ADDRESS, nunchuk_readBuffer, sizeof(nunchuk_readBuffer) );
        
        read_nwrite = false;
    }
    else
    {
        uint8_t writeIndex = (nunchuk_data_readIndex + 1) % 2;

        nunchuk_data[writeIndex].button_c = ((nunchuk_readBuffer[5] & 0x02) == 0);
        nunchuk_data[writeIndex].button_z = ((nunchuk_readBuffer[5] & 0x01) == 0);
        nunchuk_data[writeIndex].joystick_x = nunchuk_readBuffer[0];
        nunchuk_data[writeIndex].joystick_y = nunchuk_readBuffer[1];

        if( nunchuk_dataCallback != NULL )
        {
            nunchuk_dataCallback( nunchuk_data[writeIndex] );
        }

        nunchuk_data_readIndex = writeIndex;

        for(uint32_t i=0; i<10000; i++)
        {
            Nop();
        }

        SERCOM2_I2C_Write( NUNCHUK_I2C_ADDRESS, (uint8_t*)nunchuk_cmd_read, sizeof(nunchuk_cmd_read) );
        
        read_nwrite = true;
    }
}
