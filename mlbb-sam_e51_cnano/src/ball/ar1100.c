#include "ar1100.h"

#include <stdbool.h>
#include <stdint.h>

#include "peripheral/sercom/usart/plib_sercom0_usart.h"


#define AR1100_READ_BUFFER_SIZE     (10)

static uint8_t ar1100_readBuffer[AR1100_READ_BUFFER_SIZE];
static size_t ar1100_readBufferPosition;

static ar1100_touchdata_t ar1100_touchData;

static void ar1100_ReadCallback( uintptr_t context );
static void ar1100_processReadBuffer( void );


void AR1100_Initialize( void )
{
    ar1100_touchData.down = false;
    ar1100_touchData.x = 0;
    ar1100_touchData.y = 0;
    
    SERCOM0_USART_ReadCallbackRegister( ar1100_ReadCallback, (uintptr_t)NULL );
    
    ar1100_readBufferPosition = 0;
    SERCOM0_USART_Read( &ar1100_readBuffer[ar1100_readBufferPosition], 1 );
}

ar1100_touchdata_t AR1100_TouchData_Get( void )
{
    return ar1100_touchData;
}

bool AR1100_Touch_Down_Get( void )
{    
    return AR1100_TouchData_Get().down;
}

uint16_t AR1100_Touch_X_Get( void )
{
    return AR1100_TouchData_Get().x;
}

uint16_t AR1100_Touch_Y_Get( void )
{    
    return AR1100_TouchData_Get().y;
}


static void ar1100_ReadCallback( uintptr_t context )
{
    (void)context;
    
    switch( ar1100_readBufferPosition )
    {
        case 0:
        {
            if( ar1100_readBuffer[ar1100_readBufferPosition] == 0x55 )
            {
                ar1100_readBufferPosition++;
            }
            break;
        }
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        {
            ar1100_readBufferPosition++;
            break;
        }
        case 6:
        {
            ar1100_processReadBuffer();
            ar1100_readBufferPosition = 0;
            break;            
        }
        default:
        {
            ar1100_readBufferPosition = 0;
            break;
        }
    }
    
    SERCOM0_USART_Read( &ar1100_readBuffer[ar1100_readBufferPosition], 1 );
}

static void ar1100_processReadBuffer( void )
{
    if( (ar1100_readBufferPosition == 6) &&
        (ar1100_readBuffer[0] == 0x55) &&
        (ar1100_readBuffer[2] & 0x80) )
    {
        ar1100_touchData.down = ((ar1100_readBuffer[2] & 0x01) != 0);
        ar1100_touchData.x = ((ar1100_readBuffer[4] & 0x1f) << 7) | (ar1100_readBuffer[3] & 0x7f);
        ar1100_touchData.y = ((ar1100_readBuffer[6] & 0x1f) << 7) | (ar1100_readBuffer[5] & 0x7f);
    }
}
